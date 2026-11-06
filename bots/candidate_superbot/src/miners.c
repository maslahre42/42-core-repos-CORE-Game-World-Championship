#include "bot.h"

void handle_miners(t_obj *core, t_obj **units)
{
	if (!core)
		return;

	unsigned int n_carriers = core_get_units_byName_count("Carrier");

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, "Miner") != 0)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		unsigned long carried = u->s_unit.gems;

		// 1. Handshake with Carrier on-site:
		// If Carrier is adjacent and has space, transfer gems to Carrier!
		t_obj *adj_carrier = NULL;
		for (int j = 0; units && units[j]; j++)
		{
			t_obj *c = units[j];
			if (c->s_unit.name && strcmp(c->s_unit.name, "Carrier") == 0)
			{
				if (ft_is_adjacent(u->pos, c->pos))
				{
					adj_carrier = c;
					break;
				}
			}
		}

		if (adj_carrier && carried > 0)
		{
			unsigned long max_g = adj_carrier->s_unit.properties.max_gems;
			unsigned long curr_g = adj_carrier->s_unit.gems;
			if (curr_g < max_g)
			{
				unsigned long space = max_g - curr_g;
				unsigned long amount = (carried < space) ? carried : space;
				core_action_transferGems(u, adj_carrier->pos, amount);
				core_debug_addObjectInfo(u, "Relayed %lu gems to Carrier #%lu on-site", amount, adj_carrier->id);
				continue; // Cooldown resets after transfer: terminate turn immediately!
			}
		}

		// 2. Fallback if no Carrier is active and Miner is holding substantial gems
		if (n_carriers == 0 && carried >= 75)
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, carried);
				core_debug_addObjectInfo(u, "Self-unloaded %lu gems to Core", carried);
				continue;
			}
			safe_travel(u, core->pos);
			core_debug_addObjectInfo(u, "Walking back to Core with %lu gems", carried);
			continue;
		}

		// 3. Mining on-site: attack adjacent GemPile or Deposit
		t_obj *pile = ft_get_gems_nearest(u->pos);
		if (pile && ft_is_adjacent(u->pos, pile->pos))
		{
			core_action_attack(u, pile);
			core_debug_addObjectInfo(u, "Mining GemPile at (%u, %u)", pile->pos.x, pile->pos.y);
			continue;
		}

		t_obj *dep = ft_get_deposit_nearest(u->pos);
		if (dep && ft_is_adjacent(u->pos, dep->pos))
		{
			core_action_attack(u, dep);
			core_debug_addObjectInfo(u, "Demolishing Deposit at (%u, %u)", dep->pos.x, dep->pos.y);
			continue;
		}

		// 4. Move to assigned deconflicted resource
		t_obj *target = ft_get_resource_for_miner(u->pos, core->pos, u->id);
		if (target)
		{
			if (ft_is_adjacent(u->pos, target->pos))
				core_action_attack(u, target);
			else
				safe_travel(u, target->pos);
			core_debug_addObjectInfo(u, "Moving to resource at (%u, %u)", target->pos.x, target->pos.y);
			continue;
		}

		// 5. If carrying residual gems and nothing else to mine, return to Core
		if (carried > 0)
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, carried);
				continue;
			}
			safe_travel(u, core->pos);
		}
	}
}
