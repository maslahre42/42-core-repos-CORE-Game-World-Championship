#include "bot.h"

static bool is_pile_contested_by_enemy(t_pos pile_pos, unsigned int my_trip)
{
	if (ft_is_enemy_adjacent_to_pos(pile_pos))
		return true;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *enemy = game.objects[i];
		if (enemy->type != OBJ_UNIT || enemy->s_unit.team_id == game.my_team_id || enemy->hp == 0)
			continue;
		unsigned int edist = ft_distance(enemy->pos, pile_pos);
		if (edist <= 2 && edist <= my_trip)
			return true;
		if (edist < my_trip)
			return true;
	}
	return false;
}

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
		if (u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
			continue;

		unsigned long carried = u->s_unit.gems;
		unsigned long capacity = u->s_unit.properties.max_gems;
		unsigned int n_miners = core_get_units_byName_count("Miner");
		unsigned int n_hunters = core_get_units_byName_count("Hunter");

		// 1. Deliver to Core when holding a substantial load or needed funds
		bool need_delivery = false;
		if (carried >= capacity || carried >= 96)
			need_delivery = true;
		else if (n_miners < 1 && carried >= 20 && core->s_core.gems + carried >= COST_MINER)
			need_delivery = true;
		else if (n_hunters < 1 && carried >= 30 && core->s_core.gems + carried >= COST_SWORD)
			need_delivery = true;
		else if (carried >= 48 && core->s_core.gems + carried >= COST_SWORD)
			need_delivery = true;

		if (need_delivery)
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, carried);
				continue;
			}
			safe_travel(u, core->pos);
			continue;
		}

		// 2. Scavenge SAFE reachable Gem Piles across the entire map
		// (sweeps natural neutral piles & loose piles dropped by fast diggers)
		t_obj *best_pile = NULL;
		long best_pile_score = -999999;

		unsigned int dist[MAX_GRID_SIZE][MAX_GRID_SIZE];
		unsigned int home_dist[MAX_GRID_SIZE][MAX_GRID_SIZE];
		nav_compute_corridor_distances(u->pos, dist, true, u->id);
		nav_compute_corridor_distances(core->pos, home_dist, true, u->id);

		for (int k = 0; game.objects && game.objects[k]; k++)
		{
			t_obj *o = game.objects[k];
			if (o->type != OBJ_GEM_PILE || o->s_deposit_gems_pile.gems == 0)
				continue;
			if (ft_is_pos_near_armed_enemy(o->pos, 3))
				continue;
			if (o->s_deposit_gems_pile.gems <= get_pending_pile_gems(o->id))
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

			unsigned int trip = nav_approach_distance(o->pos, dist);
			unsigned int home = nav_approach_distance(o->pos, home_dist);
			if (trip >= (UINT_MAX / 4) || home >= (UINT_MAX / 4))
				continue;
			if (is_pile_contested_by_enemy(o->pos, trip))
				continue;

			unsigned long space = (capacity > carried) ? (capacity - carried) : 0;
			if (space == 0)
				continue;
			unsigned long gain = (o->s_deposit_gems_pile.gems < space)
				? o->s_deposit_gems_pile.gems : space;
			long score = (long)(gain * 1000 / (trip + home + 2));
			if (score > best_pile_score)
			{
				best_pile_score = score;
				best_pile = o;
			}
		}

		if (best_pile)
		{
			if (n_claimed_piles < 64)
				claimed_piles[n_claimed_piles++] = best_pile->id;

			if (ft_is_adjacent(u->pos, best_pile->pos))
			{
				core_action_attack(u, best_pile);
				if (!has_unit_acted(u->id) && carried > 0)
					safe_travel(u, core->pos);
			}
			else
			{
				safe_travel(u, best_pile->pos);
			}
			continue;
		}

		// 3. Meet friendly Miner with gems if no gem piles are available
		t_obj *best_miner = NULL;
		long best_miner_score = -999999;

		for (int j = 0; units && units[j]; j++)
		{
			t_obj *m = units[j];
			if (!m->s_unit.name || strcmp(m->s_unit.name, "Miner") != 0)
				continue;
			if (m->s_unit.gems < CARRIER_MINER_PICKUP_MIN)
				continue;
			if (ft_is_pos_near_armed_enemy(m->pos, 2))
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

		// 4. If holding any residual gems and no work remains, deliver to Core
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

		// 5. Park safely at distance 2 from Core (never blocking distance-1 spawn pads)
		step_away_from_core(u, core);
	}
}
