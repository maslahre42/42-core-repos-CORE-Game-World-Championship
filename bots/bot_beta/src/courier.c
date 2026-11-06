#include "bot.h"

void handle_couriers(t_obj *core)
{
	if (!core)
		return;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *u = game.objects[i];
		if (u->type != OBJ_UNIT || u->s_unit.team_id != game.my_team_id)
			continue;
		if (!u->s_unit.name || strcmp(u->s_unit.name, ROLE_COURIER) != 0)
			continue;

		// Universal cooldown check (idle units decrement to negative cooldowns)
		if (u->s_unit.action_cooldown > 0)
			continue;

		unsigned long gems = u->s_unit.gems;
		unsigned long capacity = u->s_unit.properties.max_gems;

		// 1. Unload gems at Core if adjacent
		if (gems > 0 && ft_is_adjacent(u->pos, core->pos))
		{
			core_action_transferGems(u, core->pos, gems);
			core_debug_addObjectInfo(u, "Courier unloaded %lu gems into Core", gems);
			continue; // Cooldown immediately resets after transfer
		}

		// 2. Return to Core if carrying a full load or substantial gems
		t_obj *nearest_pile = ft_get_nearest_gem_pile(u->pos);
		t_obj *miner_with_gems = ft_get_miner_with_gems_nearest(u->pos);

		if (gems >= capacity || gems >= 40 || (gems > 0 && !nearest_pile && !miner_with_gems))
		{
			t_pos next_step = path_find_step(u->pos, core->pos, true);
			if (next_step.x != u->pos.x || next_step.y != u->pos.y)
			{
				reserve_tile(next_step);
				core_action_move(u, next_step);
				core_debug_addObjectInfo(u, "Courier returning to Core with %lu gems", gems);
			}
			continue;
		}

		// 3. Early Game: Gather natural gem piles if reachable
		if (nearest_pile && ft_distance(core->pos, nearest_pile->pos) <= 13)
		{
			if (ft_is_adjacent(u->pos, nearest_pile->pos))
			{
				core_action_attack(u, nearest_pile);
				core_debug_addObjectInfo(u, "Courier scooping gem pile at (%u, %u)", nearest_pile->pos.x, nearest_pile->pos.y);
				continue;
			}
			else
			{
				t_pos next_step = path_find_step(u->pos, nearest_pile->pos, true);
				if (next_step.x != u->pos.x || next_step.y != u->pos.y)
				{
					reserve_tile(next_step);
					core_action_move(u, next_step);
					core_debug_addObjectInfo(u, "Courier sprinting to gem pile at (%u, %u)", nearest_pile->pos.x, nearest_pile->pos.y);
					continue;
				}
			}
		}

		// 4. Relay Service: Travel to stationary miners with gems
		if (miner_with_gems)
		{
			if (ft_is_adjacent(u->pos, miner_with_gems->pos))
			{
				core_debug_addObjectInfo(u, "Courier standing by adjacent to Miner #%lu for gems", miner_with_gems->id);
			}
			else
			{
				t_pos next_step = path_find_step(u->pos, miner_with_gems->pos, true);
				if (next_step.x != u->pos.x || next_step.y != u->pos.y)
				{
					reserve_tile(next_step);
					core_action_move(u, next_step);
					core_debug_addObjectInfo(u, "Courier relaying toward Miner #%lu at (%u, %u)", miner_with_gems->id, miner_with_gems->pos.x, miner_with_gems->pos.y);
				}
			}
			continue;
		}

		// 5. Idle / Positioning: Stay near miners (only move if far away)
		t_obj *any_miner = ft_get_any_miner_nearest(u->pos);
		if (any_miner)
		{
			if (ft_distance(u->pos, any_miner->pos) > 2)
			{
				t_pos next_step = path_find_step(u->pos, any_miner->pos, true);
				if (next_step.x != u->pos.x || next_step.y != u->pos.y)
				{
					reserve_tile(next_step);
					core_action_move(u, next_step);
					core_debug_addObjectInfo(u, "Courier positioning near Miner #%lu", any_miner->id);
				}
			}
			continue;
		}

		// Fallback: Return any collected gems to Core
		if (gems > 0)
		{
			t_pos next_step = path_find_step(u->pos, core->pos, true);
			if (next_step.x != u->pos.x || next_step.y != u->pos.y)
			{
				reserve_tile(next_step);
				core_action_move(u, next_step);
			}
		}
	}
}
