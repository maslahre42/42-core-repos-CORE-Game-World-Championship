#include "bot.h"

unsigned int ft_distance(t_pos a, t_pos b)
{
	unsigned int dx = (a.x > b.x) ? (a.x - b.x) : (b.x - a.x);
	unsigned int dy = (a.y > b.y) ? (a.y - b.y) : (b.y - a.y);
	return (dx + dy);
}

bool ft_is_adjacent(t_pos a, t_pos b)
{
	return (ft_distance(a, b) <= 1);
}

static bool is_core(const t_obj *obj)
{
	return (obj->type == OBJ_CORE);
}

static bool is_core_own(const t_obj *obj)
{
	return (is_core(obj) && obj->s_core.team_id == game.my_team_id);
}

static bool is_core_opponent(const t_obj *obj)
{
	return (is_core(obj) && obj->s_core.team_id != game.my_team_id);
}

static bool is_deposit(const t_obj *obj)
{
	return (obj->type == OBJ_DEPOSIT);
}

static bool is_gems(const t_obj *obj)
{
	return (obj->type == OBJ_GEM_PILE);
}

static bool is_unit(const t_obj *obj)
{
	return (obj->type == OBJ_UNIT);
}

static bool is_unit_own(const t_obj *obj)
{
	return (is_unit(obj) && obj->s_unit.team_id == game.my_team_id);
}

static bool is_unit_opponent(const t_obj *obj)
{
	return (is_unit(obj) && obj->s_unit.team_id != game.my_team_id);
}

t_obj *ft_get_core_own(void)
{
	t_pos pos = {0, 0};
	return core_get_obj_filter_nearest(pos, is_core_own);
}

t_obj *ft_get_core_opponent(void)
{
	t_pos pos = {0, 0};
	return core_get_obj_filter_nearest(pos, is_core_opponent);
}

t_obj *ft_get_deposit_nearest(t_pos pos)
{
	return core_get_obj_filter_nearest(pos, is_deposit);
}

t_obj *ft_get_gems_nearest(t_pos pos)
{
	return core_get_obj_filter_nearest(pos, is_gems);
}

t_obj **ft_get_units_own(void)
{
	return core_get_objs_filter(is_unit_own);
}

t_obj **ft_get_units_opponent(void)
{
	return core_get_objs_filter(is_unit_opponent);
}

t_obj *ft_get_units_opponent_nearest(t_pos pos)
{
	return core_get_obj_filter_nearest(pos, is_unit_opponent);
}

t_obj *ft_get_threat_nearest(t_pos pos, unsigned int max_dist)
{
	t_obj *opp = ft_get_units_opponent_nearest(pos);
	if (!opp)
		return NULL;
	if (ft_distance(pos, opp->pos) <= max_dist)
		return opp;
	return NULL;
}

t_obj *ft_get_adjacent_enemy(t_pos pos)
{
	t_obj **opps = ft_get_units_opponent();
	t_obj *adjacent = NULL;
	for (int i = 0; opps && opps[i]; i++)
	{
		if (ft_is_adjacent(pos, opps[i]->pos))
		{
			adjacent = opps[i];
			break;
		}
	}
	free(opps);
	return adjacent;
}

bool ft_is_core_threatened(t_obj *core, unsigned int radius)
{
	if (!core)
		return false;
	t_obj *threat = ft_get_threat_nearest(core->pos, radius);
	return (threat != NULL);
}

static bool is_claimed_by_other_miner(t_pos res_pos, unsigned long my_id)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id == game.my_team_id && o->id != my_id)
		{
			if (o->s_unit.name && strcmp(o->s_unit.name, "Miner") == 0)
			{
				if (ft_is_adjacent(o->pos, res_pos))
					return true;
			}
		}
	}
	return false;
}

t_obj *ft_get_resource_for_miner(t_pos unit_pos, t_pos core_pos, unsigned long unit_id)
{
	// 1. Check for nearby loose gem pile
	t_obj *pile = ft_get_gems_nearest(unit_pos);
	if (pile && ft_distance(unit_pos, pile->pos) <= 4)
		return pile;

	// 2. Select best deposit, penalizing deposits already claimed by another friendly miner
	t_obj *best = NULL;
	unsigned int best_score = UINT_MAX;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_DEPOSIT && obj->type != OBJ_GEM_PILE)
			continue;

		unsigned int dist_u = ft_distance(unit_pos, obj->pos);
		unsigned int dist_c = ft_distance(core_pos, obj->pos);

		// Prioritize resources closer to home + unit
		unsigned int score = dist_u * 2 + dist_c;

		// Deconfliction: if another miner is already working this deposit, add high penalty
		if (is_claimed_by_other_miner(obj->pos, unit_id))
			score += 100;

		// Hash variance based on unit ID
		score += ((unit_id * 11 + obj->id) % 3);

		if (score < best_score)
		{
			best_score = score;
			best = obj;
		}
	}

	return best;
}
