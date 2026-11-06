#include "bot.h"

void handle_flank_breakers(t_obj *core, t_obj *opp_core, t_obj **units)
{
	if (!opp_core)
		return;

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name)
			continue;
		if (strcmp(u->s_unit.name, UNIT_NAME_FLANK_BREAKER) != 0 &&
			strcmp(u->s_unit.name, "Striker") != 0)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		// 1. Demolish opponent Core if adjacent (Win Condition!)
		if (ft_is_adjacent(u->pos, opp_core->pos))
		{
			core_action_attack(u, opp_core);
			core_debug_addObjectInfo(u, "Flank Breaker battering opponent Core!");
			g_reserved_tiles[u->pos.y][u->pos.x] = true;
			continue;
		}

		// 2. Tactical retaliation against adjacent enemy units
		t_obj *adj_enemy = ft_get_adjacent_enemy(u->pos);
		if (adj_enemy)
		{
			core_action_attack(u, adj_enemy);
			core_debug_addObjectInfo(u, "Flank Breaker engaging adjacent enemy #%lu", adj_enemy->id);
			g_reserved_tiles[u->pos.y][u->pos.x] = true;
			continue;
		}

		// 3. Flank route calculation:
		// Hug outer border tiles (x=0,19 or y=0,19) to slip past central defenses
		t_pos waypoint;
		if (core)
		{
			if (u->id % 2 == 0)
				waypoint = (t_pos){opp_core->pos.x, core->pos.y}; // Flank Horizontal
			else
				waypoint = (t_pos){core->pos.x, opp_core->pos.y}; // Flank Vertical
		}
		else
		{
			waypoint = opp_core->pos;
		}

		if (ft_distance(u->pos, waypoint) <= 1)
		{
			// Corner reached: march along outer border directly to opponent Core!
			nav_move_adjacent_to(u, opp_core->pos, true);
			core_debug_addObjectInfo(u, "Flank Breaker advancing along flank to opponent Core (%u, %u)",
									 opp_core->pos.x, opp_core->pos.y);
			continue;
		}
		else
		{
			// Navigate along the outer border to the flank waypoint
			nav_move_to_exact(u, waypoint, true);
			core_debug_addObjectInfo(u, "Flank Breaker hugging border to waypoint (%u, %u)",
									 waypoint.x, waypoint.y);
			continue;
		}

		g_reserved_tiles[u->pos.y][u->pos.x] = true;
	}
}
