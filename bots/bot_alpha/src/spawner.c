#include "bot.h"

void handle_spawner(t_obj *core)
{
	if (!core || core->s_core.spawn_cooldown > 0)
		return;

	unsigned long gems = core->s_core.gems;
	unsigned int n_carriers = core_get_units_byName_count(UNIT_NAME_CARRIER);
	unsigned int n_miners = core_get_units_byName_count(UNIT_NAME_MINER);
	unsigned int n_hunters = core_get_units_byName_count(UNIT_NAME_HUNTER);
	unsigned int n_flank_breakers = core_get_units_byName_count(UNIT_NAME_FLANK_BREAKER);
	bool threatened = ft_is_core_threatened(core, 7);

	// 1. Emergency defense: if enemy is near our Core, spawn Hunter immediately!
	if (threatened && (n_hunters == 0 || ft_is_core_threatened(core, 4)))
	{
		if (gems >= COST_HUNTER)
		{
			// Hunter: 2x combat (70) + health (40) -> 10 HP, 6 unit damage
			core_action_createUnit(UNIT_NAME_HUNTER, "combat", "combat", "health", NULL);
			return;
		}
	}

	// 2. Early game economy:
	// 1st unit: Fast Carrier (speed + max_gems) to collect natural gem piles
	if (n_carriers == 0)
	{
		if (gems >= COST_CARRIER)
		{
			core_action_createUnit(UNIT_NAME_CARRIER, "speed", "max_gems", NULL);
			return;
		}
		return; // Wait for carrier first
	}

	// 1st Miner: Demolition on-site miner (demolition + max_gems)
	if (n_miners == 0)
	{
		if (gems >= COST_MINER)
		{
			core_action_createUnit(UNIT_NAME_MINER, "demolition", "max_gems", NULL);
			return;
		}
		return;
	}

	// 2nd Miner: Expand mining operations
	if (n_miners < 2)
	{
		if (gems >= COST_MINER)
		{
			core_action_createUnit(UNIT_NAME_MINER, "demolition", "max_gems", NULL);
			return;
		}
		return;
	}

	// 3. Early military defense: ensure at least 1 Hunter to protect our economy
	if (n_hunters == 0)
	{
		if (gems >= COST_HUNTER)
		{
			core_action_createUnit(UNIT_NAME_HUNTER, "combat", "combat", "health", NULL);
			return;
		}
		return;
	}

	// 4. Mid game expansion:
	// If we have excess gems, expand to 3rd Miner
	if (n_miners < 3 && n_flank_breakers >= 1 && gems >= (COST_MINER + 50))
	{
		core_action_createUnit(UNIT_NAME_MINER, "demolition", "max_gems", NULL);
		return;
	}

	// If we have 3 miners and strong economy, add a 2nd Carrier to relay gems faster
	if (n_carriers < 2 && n_miners >= 3 && gems >= (COST_CARRIER + 50))
	{
		core_action_createUnit(UNIT_NAME_CARRIER, "speed", "max_gems", NULL);
		return;
	}

	// 5. Military Offensive & Defense:
	// Alternate between Flank Breakers (Core siegers) and Hunters (Unit killers)
	if (n_flank_breakers <= n_hunters)
	{
		if (gems >= COST_FLANK_BREAKER)
		{
			// Flank Breaker: combat (35) + siege (35) + health (40) + armor (35) -> 187 gems
			core_action_createUnit(UNIT_NAME_FLANK_BREAKER, "combat", "siege", "health", "armor", NULL);
			return;
		}
	}
	else
	{
		if (gems >= COST_HUNTER)
		{
			// Hunter: 2x combat (70) + health (40) -> 152 gems
			core_action_createUnit(UNIT_NAME_HUNTER, "combat", "combat", "health", NULL);
			return;
		}
	}
}
