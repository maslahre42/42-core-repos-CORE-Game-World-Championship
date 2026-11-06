#include "bot.h"

void handle_phalanxes(t_obj *opp_core)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *u = game.objects[i];
		if (u->type != OBJ_UNIT || u->s_unit.team_id != game.my_team_id)
			continue;
		if (!u->s_unit.name || strcmp(u->s_unit.name, ROLE_PHALANX) != 0)
			continue;

		// Cooldown check
		if (u->s_unit.action_cooldown > 0)
			continue;

		// Priority 1: Destroy Opponent Core
		if (opp_core && ft_is_adjacent(u->pos, opp_core->pos))
		{
			core_action_attack(u, opp_core);
			core_debug_addObjectInfo(u, "Phalanx battering opponent Core!");
			continue;
		}

		// Priority 2: Engage adjacent enemy units
		t_obj *adj_enemy = ft_get_adjacent_enemy(u->pos);
		if (adj_enemy)
		{
			core_action_attack(u, adj_enemy);
			core_debug_addObjectInfo(u, "Phalanx crushing enemy #%lu", adj_enemy->id);
			continue;
		}

		// Priority 3: Advance toward opponent Core with overlapping support
		if (opp_core)
		{
			t_pos next_step = path_find_step(u->pos, opp_core->pos, true);
			if (next_step.x != u->pos.x || next_step.y != u->pos.y)
			{
				reserve_tile(next_step);
				core_action_move(u, next_step);
				core_debug_addObjectInfo(u, "Phalanx marching toward opponent Core at (%u, %u)", opp_core->pos.x, opp_core->pos.y);
			}
		}
	}
}
