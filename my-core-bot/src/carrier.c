#include "bot.h"

void handle_carrier(t_obj *core, t_obj **units)
{
	if (!core)
		return;

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, "Carrier") != 0)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		unsigned long carried = u->s_unit.gems;
		unsigned long capacity = u->s_unit.properties.max_gems;

		// 1. Deliver to Core when holding profitable load
		// Early game: deliver early to fund the first miner immediately!
		unsigned long threshold = (game.elapsed_ticks < CARRIER_EARLY_TICKS)
									  ? CARRIER_EARLY_THRESHOLD
									  : CARRIER_MID_THRESHOLD;

		if (carried >= threshold || (carried > 0 && carried >= capacity))
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, carried);
				core_debug_addObjectInfo(u, "Carrier unloaded %lu gems to Core", carried);
				continue; // Turn terminates on transfer
			}
			safe_travel(u, core->pos);
			core_debug_addObjectInfo(u, "Carrier sprinting to Core with %lu gems", carried);
			continue;
		}

		// 2. Check for friendly Miner with gems ready to transfer
		t_obj *best_miner = NULL;
		unsigned long max_miner_gems = 0;
		for (int j = 0; units && units[j]; j++)
		{
			t_obj *m = units[j];
			if (m->s_unit.name && strcmp(m->s_unit.name, "Miner") == 0)
			{
				if (m->s_unit.gems > max_miner_gems)
				{
					max_miner_gems = m->s_unit.gems;
					best_miner = m;
				}
			}
		}

		if (best_miner && max_miner_gems >= CARRIER_MINER_PICKUP_MIN)
		{
			if (ft_is_adjacent(u->pos, best_miner->pos))
			{
				// Stand next to miner to receive transfer
				core_debug_addObjectInfo(u, "Carrier waiting next to Miner #%lu for gems", best_miner->id);
				continue;
			}
			safe_travel(u, best_miner->pos);
			core_debug_addObjectInfo(u, "Carrier meeting Miner #%lu (%lu gems)", best_miner->id, max_miner_gems);
			continue;
		}

		// 3. Scavenge free natural Gem Piles across the map
		t_obj *pile = ft_get_gems_nearest(u->pos);
		if (pile)
		{
			if (ft_is_adjacent(u->pos, pile->pos))
			{
				core_action_attack(u, pile);
				core_debug_addObjectInfo(u, "Carrier scooping GemPile at (%u, %u)", pile->pos.x, pile->pos.y);
			}
			else
			{
				safe_travel(u, pile->pos);
				core_debug_addObjectInfo(u, "Carrier sprinting to GemPile at (%u, %u)", pile->pos.x, pile->pos.y);
			}
			continue;
		}

		// 4. If holding any residual gems, deliver to Core
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
	}
}
