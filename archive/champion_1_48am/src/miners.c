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
		if (u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
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

		// 2. Fallback if no Carrier is active: dynamic threshold prevents Core gem deadlock!
		unsigned long solo_thresh = MINER_SOLO_RETURN_GEMS;
		if (n_carriers == 0 && core->s_core.gems < COST_CARRIER)
		{
			unsigned long needed = COST_CARRIER - core->s_core.gems;
			if (needed < solo_thresh)
				solo_thresh = (needed < 15) ? 15 : needed;
		}

		if (n_carriers == 0 && carried >= solo_thresh)
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

		// 3. Mining on-site: prioritize adjacent Deposit first, or adjacent GemPile if cargo has room
		t_obj *dep = ft_get_deposit_nearest(u->pos);
		if (dep && ft_is_adjacent(u->pos, dep->pos))
		{
			core_action_attack(u, dep);
			core_debug_addObjectInfo(u, "Demolishing Deposit at (%u, %u)", dep->pos.x, dep->pos.y);
			continue;
		}

		t_obj *pile = ft_get_gems_nearest(u->pos);
		if (pile && ft_is_adjacent(u->pos, pile->pos) &&
			u->s_unit.gems < u->s_unit.properties.max_gems &&
			pile->s_deposit_gems_pile.gems > get_pending_pile_gems(pile->id) &&
			!ft_is_enemy_adjacent_to_pos(pile->pos))
		{
			core_action_attack(u, pile);
			core_debug_addObjectInfo(u, "Mining GemPile at (%u, %u)", pile->pos.x, pile->pos.y);
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

		// 5. If all deposits and gem piles are exhausted: Miners become Demolition / Sapper squads!
		if (!target)
		{
			t_obj *opp_core = ft_get_core_opponent();
			t_obj *wall = ft_get_wall_nearest(u->pos);

			if (wall)
			{
				if (ft_is_adjacent(u->pos, wall->pos))
					core_action_attack(u, wall);
				else
					safe_travel(u, wall->pos);
				continue;
			}
			else if (opp_core)
			{
				safe_travel(u, opp_core->pos);
				continue;
			}
		}

		// 6. If carrying residual gems and nothing else to mine, return to Core
		if (carried > 0)
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, carried);
				continue;
			}
			safe_travel(u, core->pos);
			continue;
		}

		// 7. Anti-Corking: If idle next to Core with no work, step away so Core can spawn units!
		if (ft_is_adjacent(u->pos, core->pos))
		{
			step_away_from_core(u, core);
			core_debug_addObjectInfo(u, "Miner stepping away from Core to prevent spawn corking");
		}
	}
}
