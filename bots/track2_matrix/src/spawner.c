#include "bot.h"

static unsigned long g_last_spawn_tick = ULONG_MAX;
static bool g_had_economy = false;
static bool g_attacked = false;

static void safe_create_unit(t_obj *core, const char *name, unsigned long cost,
	char *c1, char *c2, char *c3, char *c4, char *c5, char *c6)
{
	if (!core || core->s_core.spawn_cooldown > 0)
		return;
	if (game.elapsed_ticks == g_last_spawn_tick)
		return;
	if (core->s_core.gems + get_pending_core_gems() < cost)
		return;
	if (!has_free_spawn_tile(core->pos))
		return;

	g_last_spawn_tick = game.elapsed_ticks;
	core_action_createUnit(name, c1, c2, c3, c4, c5, c6, NULL);
}

static void spawn_core_action(t_obj *core, int action, unsigned long gems,
	bool open_highway, unsigned int n_carriers)
{
	switch (action)
	{
		case CORE_ACTION_RUNNER:
			if (gems >= COST_RUNNER)
				safe_create_unit(core, "Carrier", COST_RUNNER, "max_gems",
					"gem_speed", "speed", "speed", NULL, NULL);
			else if (game.elapsed_ticks > 350 && gems >= 152)
				safe_create_unit(core, "Carrier", 152, "speed", "speed", NULL, NULL, NULL, NULL);
			break;
		case CORE_ACTION_DIGGER:
			if (n_carriers > 0 && gems >= COST_DIGGER)
				safe_create_unit(core, "Miner", COST_DIGGER, "demolition", "speed",
					"speed", NULL, NULL, NULL);
			else if (n_carriers == 0 && gems >= COST_MINER_SOLO)
				safe_create_unit(core, "Miner", COST_MINER_SOLO, "demolition",
					"max_gems", NULL, NULL, NULL, NULL);
			else if (game.elapsed_ticks > 350 && gems >= 77)
				safe_create_unit(core, "Miner", 77, "demolition", NULL, NULL, NULL, NULL, NULL);
			break;
		case CORE_ACTION_BRAWLER:
			if (gems >= COST_BRAWLER_HEAVY)
				safe_create_unit(core, "Hunter", COST_BRAWLER_HEAVY, "combat", "health",
					"health", "health", "speed", "speed");
			break;
		case CORE_ACTION_SKIRMISHER:
			if (gems >= COST_SKIRMISHER)
				safe_create_unit(core, "Hunter", COST_SKIRMISHER, "combat", "health",
					"speed", "speed", NULL, NULL);
			break;
		case CORE_ACTION_FINISHER:
			if (open_highway && gems >= 262)
				safe_create_unit(core, "Striker", 262, "siege", "siege",
					"speed", "speed", "health", NULL);
			else if (gems >= COST_FINISHER)
				safe_create_unit(core, "Striker", COST_FINISHER, "siege", "siege",
					"demolition", "health", NULL, NULL);
			break;
		default:
			break;
	}
}

void handle_spawner(t_obj *core)
{
	if (!core || core->s_core.spawn_cooldown > 0)
		return;
	if (!has_free_spawn_tile(core->pos))
		return;

	unsigned long gems = core->s_core.gems + get_pending_core_gems();
	unsigned int n_carriers = core_get_units_byName_count("Carrier");
	unsigned int n_miners = core_get_units_byName_count("Miner");
	unsigned int n_defenders = core_get_units_byName_count("Defender");
	unsigned int n_hunters = core_get_units_byName_count("Hunter");
	unsigned int n_strikers = core_get_units_byName_count("Striker");

	if (n_carriers > 0 || n_miners > 0)
		g_had_economy = true;
	if (n_hunters > 0)
		g_attacked = true;

	unsigned int safe_res = ft_count_safe_resources(core->pos);
	bool threatened = ft_is_core_threatened(core, 6);
	unsigned long loose = ft_runner_opportunity(core);
	bool enemy_cleared = ft_enemy_units_cleared();
	t_obj *opp_core = ft_get_core_opponent();
	bool open_highway = (opp_core != NULL) ? nav_is_path_to_core_open(core->pos, opp_core->pos) : true;

	bool enemy_has_fighter = (ft_count_enemy_combat_units() > 0);
	bool warrior_rush_incoming = (open_highway && enemy_has_fighter && !g_had_economy && game.elapsed_ticks <= 250);

	// Warrior Rush Defense: If opponent spawned an early fighter on an open highway during opening,
	// immediately spawn a combat unit to intercept and defend our base!
	if (warrior_rush_incoming && (n_hunters + n_defenders == 0))
	{
		if (gems >= COST_BRAWLER_HEAVY)
		{
			safe_create_unit(core, "Hunter", COST_BRAWLER_HEAVY, "combat", "health",
				"health", "health", "speed", "speed");
			return;
		}
		else if (gems >= COST_FAST_SCOUT)
		{
			safe_create_unit(core, "Hunter", COST_FAST_SCOUT, "combat", "speed",
				"speed", NULL, NULL, NULL);
			return;
		}
		return;
	}

	bool core_perimeter_secured = false;
	if (opp_core)
	{
		t_obj **own = ft_get_units_own();
		for (int i = 0; own && own[i]; i++)
		{
			t_obj *u = own[i];
			if (u->s_unit.name && strcmp(u->s_unit.name, "Hunter") == 0 &&
				ft_distance(u->pos, opp_core->pos) <= 5)
			{
				core_perimeter_secured = true;
				break;
			}
		}
		if (own)
			free(own);
	}

	// Direct Finisher Spawn: When board is cleared and Core perimeter is secured,
	// spawn the Finisher immediately to end the game without being blocked by Brawler Q-values!
	if (n_hunters >= 1 && enemy_cleared && core_perimeter_secured && n_strikers < 1)
	{
		if (open_highway && gems >= 262)
		{
			safe_create_unit(core, "Striker", 262, "siege", "siege",
				"speed", "speed", "health", NULL);
			return;
		}
		else if (!open_highway && n_miners == 0 && gems >= COST_DIGGER)
		{
			safe_create_unit(core, "Miner", COST_DIGGER, "demolition", "speed",
				"speed", NULL, NULL, NULL);
			return;
		}
		else if (gems >= COST_FINISHER)
		{
			safe_create_unit(core, "Striker", COST_FINISHER, "siege", "siege",
				"demolition", "health", NULL, NULL);
			return;
		}
	}

	// 1. Determine baseline heuristic default
	int heuristic_default = -1;

	// Opening Hyper-Runner when corridors connect to loose gems
	if (!g_had_economy && n_carriers == 0 && n_miners == 0 && loose > 0 &&
		game.elapsed_ticks <= 350 && !threatened && !warrior_rush_incoming)
	{
		if (gems >= COST_RUNNER)
			heuristic_default = CORE_ACTION_RUNNER;
	}
	// Emergency Core Defense: if threatened and we have no fighters yet
	else if (threatened && (n_hunters + n_defenders == 0))
	{
		if (gems >= COST_BRAWLER_HEAVY)
			heuristic_default = CORE_ACTION_BRAWLER;
	}
	// Ensure at least 1 Carrier is active whenever loose gems exist
	else if (n_carriers < 1 && loose > 0 && !(warrior_rush_incoming && (n_hunters + n_defenders == 0)))
	{
		if (gems >= COST_RUNNER || (game.elapsed_ticks > 350 && gems >= 152))
			heuristic_default = CORE_ACTION_RUNNER;
	}
	// First Fast Digger (or solo Miner if no Carrier exists)
	else if (n_miners < 1 && safe_res > 0 && !(warrior_rush_incoming && (n_hunters + n_defenders == 0)))
	{
		if ((n_carriers > 0 && gems >= COST_DIGGER) ||
			(n_carriers == 0 && gems >= COST_MINER_SOLO) ||
			(game.elapsed_ticks > 350 && gems >= 77))
			heuristic_default = CORE_ACTION_DIGGER;
	}
	// Second Fast Digger once we have attacked, 2+ safe deposits exist, AND combat army leads!
	else if (n_miners == 1 && n_carriers > 0 && g_attacked && safe_res >= 2 &&
		!ft_is_core_threatened(core, 10) &&
		ft_count_own_combat_units() > ft_count_enemy_combat_units())
	{
		if (gems >= COST_DIGGER)
			heuristic_default = CORE_ACTION_DIGGER;
	}
	// Continuous Heavy Brawler production
	else if (gems >= COST_BRAWLER_HEAVY)
	{
		heuristic_default = CORE_ACTION_BRAWLER;
	}

	// 2. Build legal action mask based on strategic affordances
	bool legal_actions[5] = {false};

	// RUNNER: Only when no Carrier active and loose gems exist
	if (n_carriers < 1 && loose > 0 && !(warrior_rush_incoming && (n_hunters + n_defenders == 0)))
	{
		legal_actions[CORE_ACTION_RUNNER] = (gems >= COST_RUNNER) || (game.elapsed_ticks > 350 && gems >= 152);
	}

	// DIGGER: Max 1 miner opener; second miner only after at least 1 fighter produced and army leads
	if (n_miners == 0 && safe_res > 0 && !(warrior_rush_incoming && (n_hunters + n_defenders == 0)))
	{
		legal_actions[CORE_ACTION_DIGGER] = (n_carriers > 0 && gems >= COST_DIGGER) ||
		                                   (n_carriers == 0 && gems >= COST_MINER_SOLO) ||
		                                   (game.elapsed_ticks > 350 && gems >= 77);
	}
	else if (n_miners == 1 && n_carriers > 0 && g_attacked && safe_res >= 2 &&
		!ft_is_core_threatened(core, 10) &&
		ft_count_own_combat_units() > ft_count_enemy_combat_units())
	{
		legal_actions[CORE_ACTION_DIGGER] = (gems >= COST_DIGGER);
	}

	// BRAWLER: Primary combat powerhouse
	legal_actions[CORE_ACTION_BRAWLER] = (gems >= COST_BRAWLER_HEAVY);

	// SKIRMISHER: Fast combat auxiliary
	legal_actions[CORE_ACTION_SKIRMISHER] = (gems >= COST_SKIRMISHER && n_hunters >= 1);

	// FINISHER: Core demolition finisher
	if (n_hunters >= 1 && enemy_cleared && core_perimeter_secured && n_strikers < 1)
	{
		legal_actions[CORE_ACTION_FINISHER] = (open_highway && gems >= 262) || (gems >= COST_FINISHER);
	}

	// Emergency rush defense: never spawn unarmed workers when Core is actively threatened and we have 0 fighters
	if (threatened && n_hunters == 0)
	{
		legal_actions[CORE_ACTION_RUNNER] = false;
		legal_actions[CORE_ACTION_DIGGER] = false;
		legal_actions[CORE_ACTION_FINISHER] = false;
	}

	// 3. Matrix action selection
	int action = select_core_action(core, legal_actions, heuristic_default);

	// 4. Execution
	if (action >= 0)
		spawn_core_action(core, action, gems, open_highway, n_carriers);
}
