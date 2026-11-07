#include "bot.h"

static bool is_pile_contested_by_enemy(t_pos pile_pos, unsigned int my_trip)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *enemy = game.objects[i];
		if (enemy->type != OBJ_UNIT || enemy->s_unit.team_id == game.my_team_id || enemy->hp == 0)
			continue;
		unsigned int edist = ft_distance(enemy->pos, pile_pos);

		// Never walk into an armed enemy guarding the pile within 3 tiles
		if (enemy->s_unit.properties.damage_unit > 0 && edist <= 3)
			return true;

		// For unarmed enemy collectors, compare actual ticks to arrive (including enemy base cooldown)
		unsigned int enemy_cd = (enemy->s_unit.properties.base_action_cooldown > 0)
			? (unsigned int)enemy->s_unit.properties.base_action_cooldown : 1;
		unsigned int enemy_wait = (enemy->s_unit.action_cooldown > 0)
			? (unsigned int)enemy->s_unit.action_cooldown : 0;
		unsigned int enemy_ticks = (edist > 0 ? (edist - 1) * enemy_cd : 0) + enemy_wait;
		if (edist <= 2 && enemy_ticks < my_trip)
			return true;
	}
	return false;
}

static bool carrier_evade_threat(t_obj *u, t_obj *core)
{
	if (!u || u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
		return false;

	const t_obj *closest_threat = NULL;
	unsigned int min_threat_dist = UINT_MAX;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id &&
			o->s_unit.properties.damage_unit > 0 && o->hp > 0)
		{
			unsigned int d = ft_distance(u->pos, o->pos);
			if (d < min_threat_dist)
			{
				min_threat_dist = d;
				closest_threat = o;
			}
		}
	}

	if (!closest_threat || min_threat_dist > 3)
		return false;

	if (core && u->s_unit.gems > 0)
	{
		if (ft_is_adjacent(u->pos, core->pos))
		{
			core_action_transferGems(u, core->pos, u->s_unit.gems);
			return true;
		}
		if (ft_distance(u->pos, core->pos) <= 2)
		{
			safe_travel(u, core->pos);
			return true;
		}
	}

	const int dx[4] = {1, -1, 0, 0};
	const int dy[4] = {0, 0, 1, -1};

	t_pos best_step = u->pos;
	long best_score = -999999;
	bool found = false;

	for (int d = 0; d < 4; d++)
	{
		int nx = (int)u->pos.x + dx[d];
		int ny = (int)u->pos.y + dy[d];
		if (nx < 0 || ny < 0 || nx >= (int)game.grid_size || ny >= (int)game.grid_size)
			continue;
		t_pos p = {(unsigned short)nx, (unsigned short)ny};
		if (is_tile_occupied(p))
			continue;

		unsigned int step_min_threat = UINT_MAX;
		for (int i = 0; game.objects && game.objects[i]; i++)
		{
			const t_obj *o = game.objects[i];
			if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id &&
				o->s_unit.properties.damage_unit > 0 && o->hp > 0)
			{
				unsigned int ed = ft_distance(p, o->pos);
				if (ed < step_min_threat)
					step_min_threat = ed;
			}
		}

		if (step_min_threat <= 1 && min_threat_dist > 1)
			continue;

		long score = (long)(step_min_threat * 1000);

		for (int i = 0; game.objects && game.objects[i]; i++)
		{
			const t_obj *o = game.objects[i];
			if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id &&
				o->s_unit.properties.damage_unit > 0 && o->hp > 0)
			{
				if (ft_distance(p, o->pos) <= 1 && o->s_unit.action_cooldown <= 0)
					score -= 50000;
			}
		}

		for (int i = 0; game.objects && game.objects[i]; i++)
		{
			const t_obj *o = game.objects[i];
			if (o->type == OBJ_UNIT && o->s_unit.team_id == game.my_team_id &&
				o->s_unit.properties.damage_unit > 0 && o->hp > 0)
			{
				unsigned int ad = ft_distance(p, o->pos);
				if (ad <= 6)
					score += (long)((7 - ad) * 40);
			}
		}

		if (core)
		{
			unsigned int cd = ft_distance(p, core->pos);
			score += (long)((30 - cd) * 15);
			if (cd == 1)
				score -= 80;
		}

		if (score > best_score)
		{
			best_score = score;
			best_step = p;
			found = true;
		}
	}

	if (found && (best_step.x != u->pos.x || best_step.y != u->pos.y) && best_score > -40000)
	{
		core_action_move(u, best_step);
		return true;
	}

	return false;
}

void handle_carrier(t_obj *core, t_obj **units)
{
	if (!core)
		return;

	unsigned long claimed_piles[64];
	unsigned int n_claimed_piles = 0;

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, "Carrier") != 0)
			continue;
		if (u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
			continue;
		if (carrier_evade_threat(u, core))
			continue;

		unsigned long carried = u->s_unit.gems;
		unsigned long capacity = u->s_unit.properties.max_gems;
		unsigned int n_miners = core_get_units_byName_count("Miner");

		// Evaluate best reachable safe gem pile FIRST using cargo-weighted trip cost
		t_obj *best_pile = NULL;
		long best_pile_score = LONG_MIN;
		unsigned int best_pile_trip = UINT_MAX;

		unsigned int dist[MAX_GRID_SIZE][MAX_GRID_SIZE];
		unsigned int home_dist[MAX_GRID_SIZE][MAX_GRID_SIZE];
		nav_compute_corridor_distances(u->pos, dist, true, u->id);
		nav_compute_corridor_distances(core->pos, home_dist, true, u->id);

		if (carried < capacity)
		{
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

				unsigned long space = capacity - carried;
				unsigned long gain = (o->s_deposit_gems_pile.gems < space)
					? o->s_deposit_gems_pile.gems : space;
				unsigned long step = u->s_unit.properties.gems_per_cooldown_step;
				unsigned long base_cd = (u->s_unit.properties.base_action_cooldown > 0)
					? (unsigned long)u->s_unit.properties.base_action_cooldown : 0;
				unsigned long loaded = base_cd + (carried + gain) / (step ? step : 1);
				if (loaded == 0)
					loaded = 1;
				unsigned long total_cost = (unsigned long)trip + ((unsigned long)home + 1) * loaded + 2;
				long score = (long)((gain * 1000) / (total_cost ? total_cost : 1));
				if (score > best_pile_score)
				{
					best_pile_score = score;
					best_pile = o;
					best_pile_trip = trip;
				}
			}
		}

		// BUG #4 FIX: Deliver immediately if full, if no safe pile exists, if haul unlocks next unit,
		// or if already carrying >= 50 gems and the next pile is far away (> 10 tiles)
		unsigned long needed = (n_miners == 0) ? COST_DIGGER : COST_SWORD;
		unsigned long core_gems = core->s_core.gems + get_pending_core_gems();
		bool need_delivery = false;
		if (carried > 0 && (carried >= 96 || carried >= capacity || !best_pile ||
			(carried >= 36 && core_gems < needed && core_gems + carried >= needed) ||
			(carried >= 50 && best_pile_trip > 10)))
		{
			need_delivery = true;
		}

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

		// Park at distance 2 from Core while waiting for Digger to crack the next Deposit
		if (ft_distance(u->pos, core->pos) == 1)
			step_away_from_core(u, core);
	}
}
