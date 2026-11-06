#include "bot.h"

void handle_spawner(t_obj *core)
{
	if (!core || core->s_core.spawn_cooldown > 0)
		return;

	unsigned long gems = core->s_core.gems;
	unsigned int n_defenders = count_owned_units_by_role(ROLE_DEFENDER);
	unsigned int n_couriers  = count_owned_units_by_role(ROLE_COURIER);
	unsigned int n_miners    = count_owned_units_by_role(ROLE_MINER);
	bool core_threatened     = ft_is_core_threatened(7);

	// 1. Emergency Defense: If enemy is near our core, reinforce chokes immediately
	if (core_threatened && (n_defenders < 2 || ft_is_core_threatened(4)))
	{
		if (gems >= COST_DEFENDER)
		{
			core_action_createUnit(ROLE_DEFENDER, "combat", "health", "armor", NULL);
			return;
		}
	}

	// 2. Foundation: High-Speed Courier to gather early gem piles
	if (n_couriers == 0)
	{
		if (gems >= COST_COURIER)
		{
			core_action_createUnit(ROLE_COURIER, "speed", "max_gems", NULL);
			return;
		}
		return; // Wait for Courier before other units
	}

	// 3. Economy: First Stationary Miner to drill deposits
	if (n_miners == 0)
	{
		if (gems >= COST_MINER)
		{
			core_action_createUnit(ROLE_MINER, "demolition", "max_gems", NULL);
			return;
		}
		return;
	}

	// 4. Citadel Security: Deploy First Choke Defender
	if (n_defenders == 0)
	{
		if (gems >= COST_DEFENDER)
		{
			core_action_createUnit(ROLE_DEFENDER, "combat", "health", "armor", NULL);
			return;
		}
		return;
	}

	// 5. Citadel Security: Deploy Second Choke Defender to cork both corridors
	if (n_defenders < 2)
	{
		if (gems >= COST_DEFENDER)
		{
			core_action_createUnit(ROLE_DEFENDER, "combat", "health", "armor", NULL);
			return;
		}
		return;
	}

	// 6. Economic Expansion: Second Stationary Miner
	if (n_miners < 2)
	{
		if (gems >= COST_MINER)
		{
			core_action_createUnit(ROLE_MINER, "demolition", "max_gems", NULL);
			return;
		}
		return;
	}

	// 7. Military Hegemony: Produce Armored Phalanxes continuously
	if (gems >= COST_PHALANX)
	{
		core_action_createUnit(ROLE_PHALANX, "combat", "siege", "health", "armor", NULL);
		return;
	}
}
