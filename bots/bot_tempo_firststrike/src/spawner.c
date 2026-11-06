#include "bot.h"

static void spawn_brawler_or_hunter(unsigned long gems, unsigned int n_hunters,
									unsigned int enemy_combat, unsigned long enemy_max_hp,
									bool emergency)
{
	(void)enemy_combat;
	(void)enemy_max_hp;
	(void)n_hunters;

	if (gems >= COST_BRAWLER_HEAVY)
	{
		core_action_createUnit("Hunter", "combat", "health", "health", "health", "speed", "speed", NULL);
		return;
	}

	// Only settle for lighter units if Core is under immediate emergency attack!
	if (emergency)
	{
		if (gems >= COST_BRAWLER_MED)
			core_action_createUnit("Hunter", "combat", "combat", "health", "health", "speed", NULL);
		else if (gems >= COST_HUNTER_SPEED)
			core_action_createUnit("Hunter", "combat", "combat", "speed", NULL);
		else if (gems >= COST_HUNTER_TANK)
			core_action_createUnit("Hunter", "combat", "combat", "health", NULL);
	}
}

static void spawn_striker_variant(unsigned long gems)
{
	unsigned int walls = ft_count_walls();

	if (walls > 0)
	{
		if (gems >= COST_BREACH_SIEGER)
			core_action_createUnit("Striker", "siege", "demolition", "speed", "speed", "health", NULL);
		else if (gems >= COST_STRIKER)
			core_action_createUnit("Striker", "combat", "siege", "demolition", "health", NULL);
		return;
	}

	if (gems >= COST_SPEED_SIEGER)
		core_action_createUnit("Striker", "siege", "speed", "speed", "health", NULL);
	else if (gems >= COST_STRIKER_SPEED)
		core_action_createUnit("Striker", "combat", "siege", "speed", NULL);
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
	unsigned int safe_res = ft_count_safe_resources(core->pos);
	unsigned int enemy_combat = ft_count_enemy_combat_units();
	unsigned long enemy_max_hp = ft_get_enemy_max_hp();
	bool threatened = ft_is_core_threatened(core, CORE_THREAT_RADIUS);

	// 1. Emergency Core Defense: if enemy is near our Core and we don't outnumber them
	if (threatened && (n_hunters + n_defenders <= enemy_combat))
	{
		spawn_brawler_or_hunter(gems, n_hunters, enemy_combat, enemy_max_hp, true);
		return;
	}

	// 2. Opening Economy: Hyper-Speed Gem Runner at tick 32 (232 gems)
	if (n_carriers < 1 && safe_res > 0 && !threatened)
	{
		if (gems >= COST_RUNNER)
			core_action_createUnit("Carrier", "max_gems", "gem_speed", "speed", "speed", NULL);
		return;
	}

	if (n_miners < 1 && safe_res > 0 && !ft_is_core_threatened(core, 6))
	{
		if (gems >= COST_MINER)
			core_action_createUnit("Miner", "demolition", "max_gems", NULL);
		return;
	}

	// 3. Get at least 2 Heavy Brawlers before any economy greed
	if (n_hunters < 2)
	{
		spawn_brawler_or_hunter(gems, n_hunters, enemy_combat, enemy_max_hp, threatened);
		return;
	}

	// 4. Second Miner & Second Carrier (only if safe resources are abundant and we aren't outnumbered!)
	if (n_hunters + n_defenders >= enemy_combat && !threatened)
	{
		if (n_miners < TARGET_MINERS && safe_res >= 3)
		{
			if (gems >= COST_MINER)
				core_action_createUnit("Miner", "demolition", "max_gems", NULL);
			return;
		}
		if (n_carriers < MAX_CARRIERS && safe_res >= 5 && n_miners >= 2)
		{
			if (gems >= COST_CARRIER)
				core_action_createUnit("Carrier", "speed", "max_gems", NULL);
			return;
		}
	}

	// 5. CRITICAL LESSON FROM MONIR_BOT:
	// Never waste gems on 0-unit-damage Strikers while enemy combat units are alive
	// unless our Brawlers/Hunters outnumber enemy combat units by at least +2!
	if (enemy_combat > 0 && n_hunters < enemy_combat + 2)
	{
		spawn_brawler_or_hunter(gems, n_hunters, enemy_combat, enemy_max_hp, threatened);
		return;
	}

	// 6. Once we have combat superiority (or enemy has 0 combat units), spawn a Speed Sieger!
	if (n_strikers < 2 && (enemy_combat == 0 || n_hunters >= enemy_combat + 2))
	{
		spawn_striker_variant(gems);
		return;
	}

	// 7. Continue pumping Heavy Brawlers & Siegers
	if (n_strikers * 2 <= n_hunters)
		spawn_striker_variant(gems);
	else
		spawn_brawler_or_hunter(gems, n_hunters, enemy_combat, enemy_max_hp, false);
}
