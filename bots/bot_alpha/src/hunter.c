#include "bot.h"

void handle_hunters(t_obj *core, t_obj *opp_core, t_obj **units)
{
	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, UNIT_NAME_HUNTER) != 0)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		// 1. Strike any adjacent enemy unit immediately (6 lethal damage!)
		t_obj *adj_enemy = ft_get_adjacent_enemy(u->pos);
		if (adj_enemy)
		{
			core_action_attack(u, adj_enemy);
			core_debug_addObjectInfo(u, "Hunter executing adjacent enemy #%lu (6 DMG)", adj_enemy->id);
			g_reserved_tiles[u->pos.y][u->pos.x] = true;
			continue;
		}

		// 2. Hunt down target (prioritizing unarmed carriers first!)
		t_obj *target = ft_get_best_hunter_target(u, core);
		if (target)
		{
			nav_move_adjacent_to(u, target->pos, false);
			core_debug_addObjectInfo(
				u, "Hunter hunting %s enemy #%lu at (%u, %u)",
				(target->s_unit.properties.damage_unit == 0 ? "unarmed carrier" : "threat"),
				target->id, target->pos.x, target->pos.y);
			continue;
		}

		// 3. If all enemy units eliminated, assist siege on opponent Core
		if (opp_core)
		{
			if (ft_is_adjacent(u->pos, opp_core->pos))
			{
				core_action_attack(u, opp_core);
				core_debug_addObjectInfo(u, "Hunter besieging opponent Core!");
				g_reserved_tiles[u->pos.y][u->pos.x] = true;
				continue;
			}
			else
			{
				nav_move_adjacent_to(u, opp_core->pos, false);
				core_debug_addObjectInfo(u, "Hunter advancing toward opponent Core");
				continue;
			}
		}

		g_reserved_tiles[u->pos.y][u->pos.x] = true;
	}
}
