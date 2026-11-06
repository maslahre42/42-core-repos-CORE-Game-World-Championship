#include "bot.h"

void handle_spawner(t_obj *core)
{
	if (!core || core->s_core.spawn_cooldown > 0)
		return;

	// Include gems transferred to Core earlier in this tick!
	unsigned long gems = core->s_core.gems + get_pending_core_gems();
	unsigned int n_carriers = core_get_units_byName_count("Carrier");
	unsigned int n_miners = core_get_units_byName_count("Miner");
	unsigned int n_defenders = core_get_units_byName_count("Defender");
	unsigned int n_hunters = core_get_units_byName_count("Hunter");
	unsigned int active_res = ft_count_active_resources();
	unsigned int enemy_combat = ft_count_enemy_combat_units();
	bool threatened = ft_is_core_threatened(core, CORE_THREAT_RADIUS);

	// 1. Emergency Defense: if Core is under immediate threat or we are outnumbered in combat
	if (threatened && (n_defenders + n_hunters < enemy_combat + 1))
	{
		if (gems >= COST_HUNTER)
		{
			core_action_createUnit("Hunter", "combat", "combat", "health", NULL);
			return;
		}
	}

	// 2. High-speed Carrier (only if resources exist on map)
	if (n_carriers < MAX_CARRIERS && active_res > 0)
	{
		if (gems >= COST_CARRIER)
			core_action_createUnit("Carrier", "speed", "max_gems", NULL);
		return;
	}

	// 3. First Miner
	if (n_miners < 1 && active_res > 0)
	{
		if (gems >= COST_MINER)
			core_action_createUnit("Miner", "demolition", "max_gems", NULL);
		return;
	}

	// 4. Early Hunter/Defender if enemy has already spawned combat units
	if (enemy_combat > 0 && (n_hunters + n_defenders == 0))
	{
		if (gems >= COST_HUNTER)
		{
			core_action_createUnit("Hunter", "combat", "combat", "health", NULL);
			return;
		}
	}

	// 5. Secondary Miner (only if enough deposits exist)
	if (n_miners < TARGET_MINERS && active_res >= 2)
	{
		if (gems >= COST_MINER)
		{
			core_action_createUnit("Miner", "demolition", "max_gems", NULL);
			return;
		}
	}

	// 6. Core Guardian / Defender
	if (n_defenders < 1)
	{
		if (gems >= COST_DEFENDER)
		{
			core_action_createUnit("Defender", "combat", "health", "armor", NULL);
			return;
		}
	}

	// 7. Roaming Unit Assassin (scales if enemy spams light combat units)
	unsigned int desired_hunters = (enemy_combat >= 3) ? 2 : 1;
	if (n_hunters < desired_hunters)
	{
		if (gems >= COST_HUNTER)
		{
			core_action_createUnit("Hunter", "combat", "combat", "health", NULL);
			return;
		}
	}

	// 8. Optional 3rd Miner if map is still rich in deposits
	if (n_miners < MAX_MINERS && active_res >= 6)
	{
		if (gems >= COST_MINER)
		{
			core_action_createUnit("Miner", "demolition", "max_gems", NULL);
			return;
		}
	}

	// 9. Core Siege Army: Wall-Breaching Strikers
	if (gems >= COST_HEAVY_STRIKER)
		core_action_createUnit("Striker", "combat", "siege", "demolition", "health", "armor", NULL);
	else if (gems >= COST_STRIKER)
		core_action_createUnit("Striker", "combat", "siege", "demolition", "health", NULL);
}
