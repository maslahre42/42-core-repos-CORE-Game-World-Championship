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

static void spawn_brawler_or_hunter(t_obj *core, unsigned long gems, unsigned int n_hunters,
	bool emergency)
{
	(void)n_hunters;
	(void)emergency;
	// Primary Meta Fighter: 22 HP, 3-tick cooldown, 3 DMG (307 gems)
	// Never waste gems on inferior fighters that feed the opponent's army
	if (gems >= COST_BRAWLER_HEAVY)
	{
		safe_create_unit(core, "Hunter", COST_BRAWLER_HEAVY, "combat", "health",
			"health", "health", "speed", "speed");
		return;
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

	// 1. Opening Hyper-Runner when corridors connect to loose gems
	if (!g_had_economy && n_carriers == 0 && n_miners == 0 && loose > 0 &&
		game.elapsed_ticks <= 350 && !threatened)
	{
		if (gems >= COST_RUNNER)
			safe_create_unit(core, "Carrier", COST_RUNNER, "max_gems",
				"gem_speed", "speed", "speed", NULL, NULL);
		return;
	}

	// 2. Emergency Core Defense: if threatened and we have no fighters yet
	if (threatened && (n_hunters + n_defenders == 0) && gems >= COST_BRAWLER_HEAVY)
	{
		spawn_brawler_or_hunter(core, gems, n_hunters, true);
		return;
	}

	// 3. Ensure at least 1 Carrier is active whenever loose gems exist
	if (n_carriers < 1 && loose > 0)
	{
		if (gems >= COST_RUNNER)
		{
			safe_create_unit(core, "Carrier", COST_RUNNER, "max_gems",
				"gem_speed", "speed", "speed", NULL, NULL);
			return;
		}
		if (game.elapsed_ticks > 350 && gems >= 152)
		{
			safe_create_unit(core, "Carrier", 152, "speed", "speed", NULL, NULL, NULL, NULL);
			return;
		}
		return;
	}

	// 4. First Fast Digger (or solo Miner if no Carrier exists)
	if (n_miners < 1 && safe_res > 0)
	{
		if (n_carriers > 0 && gems >= COST_DIGGER)
			safe_create_unit(core, "Miner", COST_DIGGER, "demolition", "speed",
				"speed", NULL, NULL, NULL);
		else if (n_carriers == 0 && gems >= COST_MINER_SOLO)
			safe_create_unit(core, "Miner", COST_MINER_SOLO, "demolition",
				"max_gems", NULL, NULL, NULL, NULL);
		else if (game.elapsed_ticks > 350 && gems >= 77)
			safe_create_unit(core, "Miner", 77, "demolition", NULL, NULL, NULL, NULL, NULL);
		return;
	}

	// 5. Second Fast Digger once we have attacked (at least 1 Hunter built) and 2+ safe deposits exist
	if (n_miners == 1 && n_carriers > 0 && g_attacked && safe_res >= 2 &&
		!ft_is_core_threatened(core, 10))
	{
		if (gems >= COST_DIGGER)
			safe_create_unit(core, "Miner", COST_DIGGER, "demolition", "speed",
				"speed", NULL, NULL, NULL);
		return;
	}

	// 6. Core Finisher once enemy units are cleared and we have Hunters guarding the enemy Core
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

	if (n_hunters >= 1 && enemy_cleared && core_perimeter_secured)
	{
		if (n_strikers < 1)
		{
			// High-Velocity Sieger: 6 Core DMG, 6-tick CD (1.0 Core DPS)!
			// 17 hits * 6 ticks = 102 ticks to destroy 99 HP Core!
			if (open_highway && gems >= 262)
			{
				safe_create_unit(core, "Striker", 262, "siege", "siege",
					"speed", "speed", "health", NULL);
				return;
			}
			else if (gems >= COST_FINISHER)
			{
				safe_create_unit(core, "Striker", COST_FINISHER, "siege", "siege",
					"demolition", "health", NULL, NULL);
				return;
			}
			return;
		}
	}

	// 7. Continuous Heavy Brawler production
	spawn_brawler_or_hunter(core, gems, n_hunters, threatened);
}
