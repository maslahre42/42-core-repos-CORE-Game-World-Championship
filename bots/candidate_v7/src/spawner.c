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
	// 1. Primary Apex Fighter: 22 HP, 3 CD (307g)
	if (gems >= COST_BRAWLER_HEAVY)
	{
		safe_create_unit(core, "Hunter", COST_BRAWLER_HEAVY, "combat", "health",
			"health", "health", "speed", "speed");
		return;
	}
	// 2. High-Grade Fighter: 16 HP, 3 CD (267g) in emergency or after tick 600
	if (emergency || game.elapsed_ticks > 600)
	{
		if (gems >= 267)
		{
			safe_create_unit(core, "Hunter", 267, "combat", "health",
				"health", "speed", "speed", NULL);
			return;
		}
	}
	// 3. Emergency Fast Guard: 10 HP, 3 CD (227g) under critical emergency
	if (emergency && gems >= 227)
	{
		safe_create_unit(core, "Hunter", 227, "combat", "health",
			"speed", "speed", NULL, NULL);
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
	bool critical_threat = ft_is_core_threatened(core, 3);
	unsigned long loose = ft_runner_opportunity(core);
	bool enemy_cleared = ft_enemy_units_cleared();
	t_obj *opp_core = ft_get_core_opponent();
	bool open_highway = (opp_core != NULL) ? nav_is_path_to_core_open(core->pos, opp_core->pos) : true;

	// 0A. CRITICAL BASE DEFENSE: If enemy units are within 3 tiles of our Core,
	// spawn an immediate 3-CD combat defender right next to them to save our Core!
	if (critical_threat)
	{
		if (gems >= COST_BRAWLER_HEAVY)
		{
			safe_create_unit(core, "Hunter", COST_BRAWLER_HEAVY, "combat", "health",
				"health", "health", "speed", "speed");
			return;
		}
		else if (gems >= 267)
		{
			safe_create_unit(core, "Hunter", 267, "combat", "health",
				"health", "speed", "speed", NULL);
			return;
		}
		else if (gems >= 227)
		{
			safe_create_unit(core, "Hunter", 227, "combat", "health",
				"speed", "speed", NULL, NULL);
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
		else if (gems >= 227)
		{
			safe_create_unit(core, "Hunter", 227, "combat", "health",
				"speed", "speed", NULL, NULL);
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

	// 1. Opening Hyper-Runner when corridors connect to loose gems
	if (!g_had_economy && n_carriers == 0 && n_miners == 0 && loose > 0 &&
		game.elapsed_ticks <= 350 && !threatened && !warrior_rush_incoming)
	{
		if (gems >= COST_RUNNER)
			safe_create_unit(core, "Carrier", COST_RUNNER, "max_gems",
				"gem_speed", "speed", "speed", NULL, NULL);
		return;
	}

	// 2. Emergency Core Defense: if threatened and we have no fighters yet
	if (threatened && (n_hunters + n_defenders == 0))
	{
		if (gems >= COST_BRAWLER_HEAVY)
			spawn_brawler_or_hunter(core, gems, n_hunters, true);
		return;
	}

	// 3. Ensure at least 1 Carrier is active whenever loose gems exist
	if (n_carriers < 1 && loose > 0 && !(warrior_rush_incoming && (n_hunters + n_defenders == 0)))
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
	if (n_miners < 1 && safe_res > 0 && !(warrior_rush_incoming && (n_hunters + n_defenders == 0)))
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
	// or in late-game stalemate (>= 1200 ticks) when n_miners < 1
	if (((n_miners == 1 && n_carriers > 0 && g_attacked && safe_res >= 2) ||
		 (n_miners < 1 && game.elapsed_ticks >= 1200)) &&
		!ft_is_core_threatened(core, 10))
	{
		if (gems >= COST_DIGGER)
			safe_create_unit(core, "Miner", COST_DIGGER, "demolition", "speed",
				"speed", NULL, NULL, NULL);
		return;
	}

	// 5b. Sapper-Harasser (speed + demolition + combat = 167g): Limit 1 unit to assassinate enemy miners/carriers
	unsigned int n_harassers = core_get_units_byName_count("Harasser");
	if (n_harassers < 1 && n_hunters >= 1 && gems >= COST_HARASSER &&
		(game.elapsed_ticks >= 350 || ft_count_enemy_combat_units() > 1))
	{
		safe_create_unit(core, "Harasser", COST_HARASSER, "speed", "demolition",
			"combat", NULL, NULL, NULL);
		return;
	}

	// 6. Core Finisher once enemy units are cleared, OR in late-game stalemate (>= 1200 ticks)
	// to force decisive Core destruction rather than idling into a 10,000-tick tiebreak!
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

	bool force_finisher = (game.elapsed_ticks >= 1000 && n_hunters >= 2 && n_strikers < 1);
	if ((n_hunters >= 1 && enemy_cleared && core_perimeter_secured) || force_finisher)
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
			else if (open_highway && gems >= 222)
			{
				safe_create_unit(core, "Striker", 222, "siege", "siege",
					"speed", "speed", NULL, NULL);
				return;
			}
			else if (open_highway && gems >= COST_CORE_ATTACKER)
			{
				safe_create_unit(core, "Striker", COST_CORE_ATTACKER, "speed", "siege",
					"combat", "health", NULL, NULL);
				return;
			}
			else if (gems >= COST_FINISHER && !open_highway)
			{
				safe_create_unit(core, "Striker", COST_FINISHER, "siege", "siege",
					"demolition", "health", NULL, NULL);
				return;
			}
			else if (gems >= 187)
			{
				safe_create_unit(core, "Striker", 187, "siege", "speed",
					"speed", NULL, NULL, NULL);
				return;
			}
			if (force_finisher)
				return;
		}
	}

	// 7. Continuous Heavy Brawler production
	spawn_brawler_or_hunter(core, gems, n_hunters, threatened);
}
