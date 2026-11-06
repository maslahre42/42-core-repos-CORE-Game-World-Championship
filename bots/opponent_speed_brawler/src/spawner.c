#include "bot.h"

void handle_spawner(t_obj *core)
{
	if (!core || core->s_core.spawn_cooldown > 0)
		return;

	unsigned long gems = core->s_core.gems + get_pending_core_gems();
	unsigned int n_gatherers = core_get_units_byName_count("Gatherer");
	unsigned int n_miners = core_get_units_byName_count("Miner");
	unsigned int n_brawlers = core_get_units_byName_count("Brawler");

	// 1. First priority: High-speed Gatherer (Tick 32)
	// ['max_gems', 'gem_speed', 'speed', 'speed'] -> CD 0, moves every tick, collects 127 gems/step!
	if (n_gatherers < 1)
	{
		if (gems >= COST_GATHERER)
			core_action_createUnit("Gatherer", "max_gems", "gem_speed", "speed", "speed", NULL);
		return;
	}

	// 2. Second priority: High-speed Miner (Tick 107)
	// ['demolition', 'speed', 'speed'] -> CD 3, 3 object damage
	if (n_miners < 1)
	{
		if (gems >= COST_MINER)
			core_action_createUnit("Miner", "demolition", "speed", "speed", NULL);
		return;
	}

	// 3. Primary Strike Force: 3 Heavy High-Speed Brawlers
	// ['combat', 'health', 'health', 'health', 'speed', 'speed'] -> 22 HP, CD 3, 3 unit damage
	if (n_brawlers < MAX_BRAWLERS)
	{
		if (gems >= COST_BRAWLER)
			core_action_createUnit("Brawler", "combat", "health", "health", "health", "speed", "speed", NULL);
		return;
	}

	// 4. Secondary Miner (Tick 447)
	if (n_miners < 2)
	{
		if (gems >= COST_MINER)
		{
			core_action_createUnit("Miner", "demolition", "speed", "speed", NULL);
			return;
		}
	}

	// 5. Endgame Finisher: Rapid Sieger
	// ['siege', 'speed', 'speed', 'health'] -> 10 HP, CD 3, 3 core damage (melts Core in 96 ticks)
	if (gems >= COST_SIEGER)
	{
		core_action_createUnit("Sieger", "siege", "speed", "speed", "health", NULL);
	}
}
