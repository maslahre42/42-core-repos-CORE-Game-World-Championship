#include "bot.h"

void handle_carriers(t_obj *core, t_obj **units)
{
	if (!core)
		return;

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, UNIT_NAME_CARRIER) != 0)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		unsigned long carried = u->s_unit.gems;
		unsigned long capacity = u->s_unit.properties.max_gems;

		// 1. If holding profitable haul (>= 50 gems or nearly full capacity):
		// Sprint back to Core to unload!
		if (carried >= 50 || (carried > 0 && carried >= capacity - 16))
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, carried);
				core_debug_addObjectInfo(u, "Carrier transferring %lu gems to Core", carried);
				g_reserved_tiles[u->pos.y][u->pos.x] = true;
				continue; // Mandatory continue after transferGems
			}
			else
			{
				nav_move_adjacent_to(u, core->pos, false);
				core_debug_addObjectInfo(u, "Carrier returning %lu gems to Core", carried);
				continue;
			}
		}

		// 2. Early game: Prioritize nearby natural gem piles (OBJ_GEM_PILE)
		t_obj *gem_pile = ft_get_nearest_gem_pile(u->pos);
		if (gem_pile && ft_distance(u->pos, gem_pile->pos) <= 8)
		{
			if (ft_is_adjacent(u->pos, gem_pile->pos))
			{
				core_action_attack(u, gem_pile);
				core_debug_addObjectInfo(u, "Carrier harvesting Gem Pile at (%u, %u)",
										 gem_pile->pos.x, gem_pile->pos.y);
				g_reserved_tiles[u->pos.y][u->pos.x] = true;
				continue;
			}
			else
			{
				nav_move_adjacent_to(u, gem_pile->pos, false);
				core_debug_addObjectInfo(u, "Carrier sprinting to Gem Pile at (%u, %u)",
										 gem_pile->pos.x, gem_pile->pos.y);
				continue;
			}
		}

		// 3. Mid game: Meet miners on-site to receive gems and relay them to Core
		t_obj *target_miner = ft_get_miner_needing_carrier(u->pos);
		if (target_miner)
		{
			if (ft_is_adjacent(u->pos, target_miner->pos))
			{
				// Adjacent to miner: if carrier already has gems and miner is empty, return to Core
				if (carried > 0 && target_miner->s_unit.gems == 0)
				{
					if (ft_is_adjacent(u->pos, core->pos))
					{
						core_action_transferGems(u, core->pos, carried);
						core_debug_addObjectInfo(u, "Carrier transferring %lu gems to Core", carried);
						g_reserved_tiles[u->pos.y][u->pos.x] = true;
						continue;
					}
					else
					{
						nav_move_adjacent_to(u, core->pos, false);
						core_debug_addObjectInfo(u, "Carrier heading home with %lu gems", carried);
						continue;
					}
				}
				// Otherwise, hold position adjacent to miner to receive transfer
				g_reserved_tiles[u->pos.y][u->pos.x] = true;
				core_debug_addObjectInfo(u, "Carrier on-site with Miner #%lu (Miner gems: %lu)",
										 target_miner->id, target_miner->s_unit.gems);
				continue;
			}
			else
			{
				nav_move_adjacent_to(u, target_miner->pos, false);
				core_debug_addObjectInfo(u, "Carrier meeting Miner #%lu on-site", target_miner->id);
				continue;
			}
		}

		// 4. Any remaining natural gem pile on the board
		if (gem_pile)
		{
			if (ft_is_adjacent(u->pos, gem_pile->pos))
			{
				core_action_attack(u, gem_pile);
				core_debug_addObjectInfo(u, "Carrier harvesting Gem Pile at (%u, %u)",
										 gem_pile->pos.x, gem_pile->pos.y);
				g_reserved_tiles[u->pos.y][u->pos.x] = true;
				continue;
			}
			else
			{
				nav_move_adjacent_to(u, gem_pile->pos, false);
				continue;
			}
		}

		// 5. Fallback: If carrying any gems, return them to Core
		if (carried > 0)
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, carried);
				core_debug_addObjectInfo(u, "Carrier transferring %lu gems to Core", carried);
				g_reserved_tiles[u->pos.y][u->pos.x] = true;
				continue;
			}
			else
			{
				nav_move_adjacent_to(u, core->pos, false);
				continue;
			}
		}

		// Idle in place
		g_reserved_tiles[u->pos.y][u->pos.x] = true;
	}
}
