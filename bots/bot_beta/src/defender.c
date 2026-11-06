#include "bot.h"

void handle_defenders(t_obj *core)
{
	if (!core)
		return;
	int defender_idx = 0;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *u = game.objects[i];
		if (u->type != OBJ_UNIT || u->s_unit.team_id != game.my_team_id)
			continue;
		if (!u->s_unit.name || strcmp(u->s_unit.name, ROLE_DEFENDER) != 0)
			continue;

		int current_choke_idx = defender_idx++;

		// Cooldown check: idle units decrement to negative cooldowns
		if (u->s_unit.action_cooldown > 0)
			continue;

		t_pos target_choke = ft_get_choke_point(current_choke_idx);

		// Priority 1: Crush adjacent enemy intruder/rusher
		t_obj *adj_enemy = ft_get_adjacent_enemy(u->pos);
		if (adj_enemy)
		{
			core_action_attack(u, adj_enemy);
			core_debug_addObjectInfo(u, "Defender corking choke, striking enemy #%lu", adj_enemy->id);
			continue;
		}

		// Priority 2: Infiltration defense - intercept any enemy approaching our core perimeter
		t_obj *threat = ft_get_threat_nearest(core->pos, 3);
		if (threat && ft_distance(u->pos, threat->pos) <= 3)
		{
			t_pos intercept = path_find_step(u->pos, threat->pos, true);
			if (intercept.x != u->pos.x || intercept.y != u->pos.y)
			{
				reserve_tile(intercept);
				core_action_move(u, intercept);
				core_debug_addObjectInfo(u, "Defender intercepting threat at (%u, %u)", threat->pos.x, threat->pos.y);
				continue;
			}
		}

		// Priority 3: Hold anchor at choke point
		if (u->pos.x == target_choke.x && u->pos.y == target_choke.y)
		{
			core_debug_addObjectInfo(u, "Defender anchored at choke (%u, %u)", u->pos.x, u->pos.y);
			continue;
		}

		// Priority 4: Advance to assigned choke point
		t_pos next_step = path_find_step(u->pos, target_choke, false);
		if (next_step.x != u->pos.x || next_step.y != u->pos.y)
		{
			reserve_tile(next_step);
			core_action_move(u, next_step);
			core_debug_addObjectInfo(u, "Defender moving to choke (%u, %u)", target_choke.x, target_choke.y);
		}
	}
}
