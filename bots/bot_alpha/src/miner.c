#include "bot.h"

void handle_miners(t_obj *core, t_obj **units)
{
	if (!core)
		return;

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, UNIT_NAME_MINER) != 0)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		// 1. When Carrier is adjacent, transfers gems to Carrier!
		t_obj *carrier = ft_get_adjacent_carrier(u->pos);
		if (carrier && u->s_unit.gems > 0)
		{
			core_action_transferGems(u, carrier->pos, u->s_unit.gems);
			core_debug_addObjectInfo(u, "Miner transfer %lu gems to Carrier #%lu",
									 u->s_unit.gems, carrier->id);
			g_reserved_tiles[u->pos.y][u->pos.x] = true;
			continue; // IMMEDIATELY follow transferGems with continue to avoid cooldown reset failures!
		}

		// 2. Mines deposits on-site and stays there!
		t_obj *deposit = ft_get_deposit_for_miner(u, core);
		if (deposit)
		{
			if (ft_is_adjacent(u->pos, deposit->pos))
			{
				core_action_attack(u, deposit);
				core_debug_addObjectInfo(u, "Miner demolition strike on Deposit (%u, %u) [held: %lu]",
										 deposit->pos.x, deposit->pos.y, u->s_unit.gems);
				g_reserved_tiles[u->pos.y][u->pos.x] = true;
				continue;
			}
			else
			{
				nav_move_adjacent_to(u, deposit->pos, false);
				core_debug_addObjectInfo(u, "Miner marching to assigned Deposit (%u, %u)",
										 deposit->pos.x, deposit->pos.y);
				continue;
			}
		}

		// 3. If assigned deposit is gone, check for an adjacent Gem Pile
		t_obj *adj_gem_pile = ft_get_nearest_gem_pile(u->pos);
		if (adj_gem_pile && ft_is_adjacent(u->pos, adj_gem_pile->pos))
		{
			core_action_attack(u, adj_gem_pile);
			core_debug_addObjectInfo(u, "Miner collecting adjacent Gem Pile (%u, %u)",
									 adj_gem_pile->pos.x, adj_gem_pile->pos.y);
			g_reserved_tiles[u->pos.y][u->pos.x] = true;
			continue;
		}

		// 4. Emergency fallback: if full and no friendly Carriers exist, return gems to Core
		if (u->s_unit.gems >= 150 && core_get_units_byName_count(UNIT_NAME_CARRIER) == 0)
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, u->s_unit.gems);
				g_reserved_tiles[u->pos.y][u->pos.x] = true;
				continue;
			}
			else
			{
				nav_move_adjacent_to(u, core->pos, false);
				continue;
			}
		}

		// Idle in place on-site
		g_reserved_tiles[u->pos.y][u->pos.x] = true;
	}
}
