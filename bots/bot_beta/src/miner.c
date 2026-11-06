#include "bot.h"

void handle_miners(t_obj *core)
{
	if (!core)
		return;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *u = game.objects[i];
		if (u->type != OBJ_UNIT || u->s_unit.team_id != game.my_team_id)
			continue;
		if (!u->s_unit.name || strcmp(u->s_unit.name, ROLE_MINER) != 0)
			continue;

		// Cooldown check: idle units decrement to negative cooldowns
		if (u->s_unit.action_cooldown > 0)
			continue;

		unsigned long gems = u->s_unit.gems;

		// 1. Dedicated Hand-off: Transfer gems to adjacent Courier
		if (gems > 0)
		{
			t_obj *courier = ft_get_adjacent_courier(u->pos);
			if (courier)
			{
				unsigned long capacity = courier->s_unit.properties.max_gems;
				unsigned long space_left = (capacity > courier->s_unit.gems) ? (capacity - courier->s_unit.gems) : 0;
				if (space_left > 0)
				{
					unsigned long amount = (gems > space_left) ? space_left : gems;
					if (amount > 0)
					{
						core_action_transferGems(u, courier->pos, amount);
						core_debug_addObjectInfo(u, "Transferred %lu gems to Courier #%lu", amount, courier->id);
						continue; // MUST immediately follow transferGems with continue to avoid cooldown reset failure!
					}
				}
			}
		}

		// 2. On-Site Mining: Attack adjacent Deposit or GemPile
		t_obj *adj_resource = ft_get_adjacent_deposit_or_pile(u->pos);
		if (adj_resource)
		{
			core_action_attack(u, adj_resource);
			core_debug_addObjectInfo(u, "Mining %s at (%u, %u)",
				(adj_resource->type == OBJ_DEPOSIT ? "Deposit" : "GemPile"),
				adj_resource->pos.x, adj_resource->pos.y);
			continue;
		}

		// 3. Relocate to best unmined Deposit
		t_obj *deposit = ft_get_best_deposit_for_miner(u->pos, u->id);
		if (deposit)
		{
			t_pos next_step = path_find_step(u->pos, deposit->pos, true);
			if (next_step.x != u->pos.x || next_step.y != u->pos.y)
			{
				reserve_tile(next_step);
				core_action_move(u, next_step);
				core_debug_addObjectInfo(u, "Traveling to Deposit at (%u, %u)", deposit->pos.x, deposit->pos.y);
			}
			continue;
		}

		// 4. Fallback: If carrying gems and no deposits left, deliver to Core
		if (gems > 0)
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, gems);
				continue;
			}
			t_pos next_step = path_find_step(u->pos, core->pos, true);
			if (next_step.x != u->pos.x || next_step.y != u->pos.y)
			{
				reserve_tile(next_step);
				core_action_move(u, next_step);
			}
		}
	}
}
