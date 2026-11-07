#include "bot.h"

static unsigned long g_next_allowed_spawn_tick = 0;

static void safe_create_unit(t_obj *core, const char *name, unsigned long cost,
	unsigned long post_spawn_cd,
	char *c1, char *c2, char *c3, char *c4, char *c5, char *c6)
{
	if (!core || core->s_core.spawn_cooldown > 0)
		return;
	if (game.elapsed_ticks < g_next_allowed_spawn_tick)
		return;
	if (core->s_core.gems < cost)
		return;
	if (!has_free_spawn_tile(core->pos))
		return;

	g_next_allowed_spawn_tick = game.elapsed_ticks + post_spawn_cd;
	core_action_createUnit(name, c1, c2, c3, c4, c5, c6, NULL);
}

static void spawn_brawler_or_hunter(t_obj *core, unsigned long gems, unsigned int n_hunters,
									unsigned int enemy_combat, unsigned long enemy_max_hp,
									bool emergency)
{
	(void)enemy_combat;
	(void)enemy_max_hp;
	(void)n_hunters;

	if (gems >= COST_BRAWLER_HEAVY)
	{
		safe_create_unit(core, "Hunter", COST_BRAWLER_HEAVY, 53, "combat", "health", "health", "health", "speed", "speed");
		return;
	}

	// Only settle for lighter units if Core is under immediate emergency attack!
	if (emergency)
	{
		if (gems >= COST_BRAWLER_MED)
			safe_create_unit(core, "Hunter", COST_BRAWLER_MED, 42, "combat", "combat", "health", "health", "speed", NULL);
		else if (gems >= COST_HUNTER_SPEED)
			safe_create_unit(core, "Hunter", COST_HUNTER_SPEED, 36, "combat", "combat", "speed", NULL, NULL, NULL);
		else if (gems >= COST_HUNTER_TANK)
			safe_create_unit(core, "Hunter", COST_HUNTER_TANK, 27, "combat", "combat", "health", NULL, NULL, NULL);
	}
}

void handle_spawner(t_obj *core)
{
	if (!core || core->s_core.spawn_cooldown > 0)
		return;
	if (!has_free_spawn_tile(core->pos))
		return;

	// STRICT ZERO ACTION FAILURE GUARANTEE: only spend confirmed Core gems!
	unsigned long gems = core->s_core.gems;
	unsigned int n_carriers = core_get_units_byName_count("Carrier");
	unsigned int n_miners = core_get_units_byName_count("Miner");
	unsigned int n_defenders = core_get_units_byName_count("Defender");
	unsigned int n_hunters = core_get_units_byName_count("Hunter");
	unsigned int n_strikers = core_get_units_byName_count("Striker");
	unsigned int safe_res = ft_count_safe_resources(core->pos);
	unsigned int enemy_combat = ft_count_enemy_combat_units();
	unsigned long enemy_max_hp = ft_get_enemy_max_hp();
	bool threatened = ft_is_core_threatened(core, CORE_THREAT_RADIUS);
	unsigned long loose = ft_runner_opportunity(core);
	bool enemy_cleared = ft_enemy_units_cleared();

	// 1. Emergency Core Defense: if enemy is near our Core and we don't outnumber them
	if (threatened && (n_hunters + n_defenders <= enemy_combat))
	{
		spawn_brawler_or_hunter(core, gems, n_hunters, enemy_combat, enemy_max_hp, true);
		return;
	}

	// 2. Dynamic Opening Logic:
	// If corridors connect to safe loose gem piles, save for the Carrier/Runner at tick 32 (232 gems)
	if (n_carriers < 1 && n_miners < 1 && loose > 0 && game.elapsed_ticks <= 350 && !threatened)
	{
		if (gems >= COST_RUNNER)
			safe_create_unit(core, "Carrier", COST_RUNNER, 44, "max_gems", "gem_speed", "speed", "speed", NULL, NULL);
		return;
	}

	// If corridors are blocked (loose == 0), do NOT wait 32 ticks!
	// Immediately spawn a Miner at tick 0 (gems == 200 >= 187)!
	if (n_miners < 1 && !threatened)
	{
		if (gems >= COST_MINER)
			safe_create_unit(core, "Miner", COST_MINER, 44, "demolition", "speed", "speed", NULL, NULL, NULL);
		return;
	}

	// If miner was built first during blocked opening, build Carrier as soon as affordable
	if (n_carriers < 1 && !threatened)
	{
		if (gems >= COST_RUNNER)
			safe_create_unit(core, "Carrier", COST_RUNNER, 44, "max_gems", "gem_speed", "speed", "speed", NULL, NULL);
		return;
	}

	// 3. First-Wave Combat Force: deploy at least 2 Heavy Brawlers before any economy expansion
	if (n_hunters < 2)
	{
		spawn_brawler_or_hunter(core, gems, n_hunters, enemy_combat, enemy_max_hp, threatened);
		return;
	}

	// 4. Second High-Velocity Digger if resources are abundant and base is secure
	if (n_hunters >= 2 && n_miners < 2 && safe_res >= 3 && !threatened)
	{
		if (gems >= COST_MINER)
		{
			safe_create_unit(core, "Miner", COST_MINER, 44, "demolition", "speed", "speed", NULL, NULL, NULL);
			return;
		}
	}

	// 5. Decisive Core Finisher: once enemy units are cleared and we have hunters guarding
	if (n_hunters >= 2 && enemy_cleared)
	{
		if (n_strikers < 1)
		{
			if (gems >= COST_FINISHER)
				safe_create_unit(core, "Striker", COST_FINISHER, 31, "siege", "siege", "demolition", "health", NULL, NULL);
			return;
		}
	}

	// 6. If enemy units are still alive: NEVER build 0-unit-damage Strikers!
	// Pump 22 HP Heavy Brawlers (3cd, 3 dmg) to overwhelm enemy forces!
	if (!enemy_cleared)
	{
		spawn_brawler_or_hunter(core, gems, n_hunters, enemy_combat, enemy_max_hp, threatened);
		return;
	}

	// 7. If field is cleared and Finisher is already out, continue army support
	if (n_strikers < 2 && gems >= COST_FINISHER)
	{
		safe_create_unit(core, "Striker", COST_FINISHER, 31, "siege", "siege", "demolition", "health", NULL, NULL);
		return;
	}
	else
	{
		spawn_brawler_or_hunter(core, gems, n_hunters, enemy_combat, enemy_max_hp, false);
	}
}
