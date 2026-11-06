#include "bot.h"

void handle_spawner(t_obj *core)
{
	if (!core || core->s_core.spawn_cooldown > 0)
		return;

	unsigned long gems = core->s_core.gems;
	unsigned int n_carriers = core_get_units_byName_count("Carrier");
	unsigned int n_miners = core_get_units_byName_count("Miner");
	unsigned int n_defenders = core_get_units_byName_count("Defender");
	unsigned int n_hunters = core_get_units_byName_count("Hunter");
	bool threatened = ft_is_core_threatened(core, 8);

	// 1. Tick 0 priority: Hyper-speed Carrier (137 gems, 2-tick cooldown)
	// Gathers natural gem piles immediately to fund the rest of the army!
	if (n_carriers < 1)
	{
		if (gems >= COST_CARRIER)
			core_action_createUnit("Carrier", "speed", "max_gems", NULL);
		return;
	}

	// 2. Emergency Defense: spawn Defender or Hunter if enemies threaten Core
	if (threatened && n_defenders + n_hunters == 0)
	{
		if (gems >= COST_HUNTER)
		{
			// Hunter: 2x combat + health -> 10 HP, 6 unit damage!
			core_action_createUnit("Hunter", "combat", "combat", "health", NULL);
			return;
		}
	}

	// 3. First Miner: breaks deposits on-site and relays gems to Carrier
	if (n_miners < 1)
	{
		if (gems >= COST_MINER)
			core_action_createUnit("Miner", "demolition", "max_gems", NULL);
		return;
	}

	// 4. Secondary Miner: expand economy
	if (n_miners < 2)
	{
		if (gems >= COST_MINER)
		{
			core_action_createUnit("Miner", "demolition", "max_gems", NULL);
			return;
		}
	}

	// 5. Core Guardian / Defender: 10 HP, 30% armor corks defensive choke
	if (n_defenders < 1)
	{
		if (gems >= COST_DEFENDER)
		{
			core_action_createUnit("Defender", "combat", "health", "armor", NULL);
			return;
		}
	}

	// 6. Roaming Unit Assassin: 6 unit damage
	if (n_hunters < 1)
	{
		if (gems >= COST_HUNTER)
		{
			core_action_createUnit("Hunter", "combat", "combat", "health", NULL);
			return;
		}
	}

	// 7. Core Siege Army: Wall-Breaching Strikers
	// (Demolishes walls, slaughters units, batters enemy Core)
	if (gems >= COST_HEAVY_STRIKER)
	{
		core_action_createUnit("Striker", "combat", "siege", "demolition", "health", "armor", NULL);
	}
	else if (gems >= COST_STRIKER)
	{
		core_action_createUnit("Striker", "combat", "siege", "demolition", "health", NULL);
	}
}
