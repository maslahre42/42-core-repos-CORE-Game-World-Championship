#include "bot.h"

void handle_carrier(t_obj *core, t_obj **units)
{
	if (!core)
		return;

	unsigned long claimed_miners[32];
	unsigned int n_claimed_miners = 0;
	unsigned long claimed_piles[64];
	unsigned int n_claimed_piles = 0;

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, "Carrier") != 0)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		unsigned long carried = u->s_unit.gems;
		unsigned long capacity = u->s_unit.properties.max_gems;
		unsigned long threshold = (game.elapsed_ticks < CARRIER_EARLY_TICKS)
			? CARRIER_EARLY_THRESHOLD : CARRIER_MID_THRESHOLD;

		// 1. Deliver to Core when holding profitable load
		if (carried >= threshold || (carried > 0 && carried >= capacity))
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, carried);
				continue;
			}
			safe_travel(u, core->pos);
			continue;
		}

		// 2. Meet friendly Miner with gems (Deconflicted across multiple Carriers!)
		t_obj *best_miner = NULL;
		long best_miner_score = -999999;

		for (int j = 0; units && units[j]; j++)
		{
			t_obj *m = units[j];
			if (!m->s_unit.name || strcmp(m->s_unit.name, "Miner") != 0)
				continue;
			if (m->s_unit.gems < CARRIER_MINER_PICKUP_MIN)
				continue;

			bool already_claimed = false;
			for (unsigned int c = 0; c < n_claimed_miners; c++)
			{
				if (claimed_miners[c] == m->id)
				{
					already_claimed = true;
					break;
				}
			}
			if (already_claimed)
				continue;

			long score = (long)(m->s_unit.gems * 4) - (long)(ft_distance(u->pos, m->pos) * 5);
			if (score > best_miner_score)
			{
				best_miner_score = score;
				best_miner = m;
			}
		}

		if (best_miner)
		{
			if (n_claimed_miners < 32)
				claimed_miners[n_claimed_miners++] = best_miner->id;

			if (!ft_is_adjacent(u->pos, best_miner->pos))
				safe_travel(u, best_miner->pos);
			else if (ft_is_adjacent(u->pos, core->pos))
				step_away_from_core(u, core);
			continue;
		}

		// 3. Scavenge free natural Gem Piles (Deconflicted across multiple Carriers!)
		t_obj *best_pile = NULL;
		unsigned int best_pile_dist = UINT_MAX;

		for (int k = 0; game.objects && game.objects[k]; k++)
		{
			t_obj *o = game.objects[k];
			if (o->type != OBJ_GEM_PILE || get_pending_damage(o->id) >= o->hp)
				continue;

			bool already_claimed = false;
			for (unsigned int c = 0; c < n_claimed_piles; c++)
			{
				if (claimed_piles[c] == o->id)
				{
					already_claimed = true;
					break;
				}
			}
			if (already_claimed)
				continue;

			unsigned int d = ft_distance(u->pos, o->pos);
			if (d < best_pile_dist)
			{
				best_pile_dist = d;
				best_pile = o;
			}
		}

		if (best_pile)
		{
			if (n_claimed_piles < 64)
				claimed_piles[n_claimed_piles++] = best_pile->id;

			if (ft_is_adjacent(u->pos, best_pile->pos))
				core_action_attack(u, best_pile);
			else
				safe_travel(u, best_pile->pos);
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

		// 5. Never park idle on the Core's spawn pads!
		step_away_from_core(u, core);
	}
}
