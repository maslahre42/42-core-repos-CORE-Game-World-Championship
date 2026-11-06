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

static bool is_core(const t_obj *obj) { return (obj->type == OBJ_CORE); }
static bool is_core_own(const t_obj *obj) { return (is_core(obj) && obj->s_core.team_id == game.my_team_id); }
static bool is_core_opponent(const t_obj *obj) { return (is_core(obj) && obj->s_core.team_id != game.my_team_id); }
static bool is_deposit(const t_obj *obj) { return (obj->type == OBJ_DEPOSIT); }
static bool is_gems(const t_obj *obj) { return (obj->type == OBJ_GEM_PILE); }
static bool is_unit(const t_obj *obj) { return (obj->type == OBJ_UNIT); }
static bool is_unit_own(const t_obj *obj) { return (is_unit(obj) && obj->s_unit.team_id == game.my_team_id); }
static bool is_unit_opponent(const t_obj *obj) { return (is_unit(obj) && obj->s_unit.team_id != game.my_team_id); }

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

t_obj *ft_get_deposit_nearest(t_pos pos) { return core_get_obj_filter_nearest(pos, is_deposit); }
t_obj *ft_get_gems_nearest(t_pos pos) { return core_get_obj_filter_nearest(pos, is_gems); }
t_obj **ft_get_units_own(void) { return core_get_objs_filter(is_unit_own); }
t_obj **ft_get_units_opponent(void) { return core_get_objs_filter(is_unit_opponent); }
t_obj *ft_get_units_opponent_nearest(t_pos pos) { return core_get_obj_filter_nearest(pos, is_unit_opponent); }

t_obj *ft_get_threat_nearest(t_pos pos, unsigned int max_dist)
{
	t_obj *opp = ft_get_units_opponent_nearest(pos);
	if (!opp)
		return NULL;
	if (ft_distance(pos, opp->pos) <= max_dist)
		return opp;
	return NULL;
}

// Overkill-aware, focus-fire adjacent target selector
t_obj *ft_get_best_adjacent_enemy(const t_obj *attacker)
{
	if (!attacker)
		return NULL;
	t_obj **opps = ft_get_units_opponent();
	t_obj *best = NULL;
	long best_score = -999999;

	for (int i = 0; opps && opps[i]; i++)
	{
		t_obj *e = opps[i];
		if (!ft_is_adjacent(attacker->pos, e->pos))
			continue;

		unsigned long pending = get_pending_damage(e->id);
		if (pending >= e->hp)
			continue; // Already lethal this tick! Don't waste an attack.

		unsigned long rem_hp = e->hp - pending;
		long score = 1000 - (long)(rem_hp * 25);

		// Bonus if this single strike is a guaranteed kill
		if (attacker->s_unit.properties.damage_unit >= rem_hp)
			score += 500;

		// Prioritize armed threats in melee or high-gem carriers
		if (e->s_unit.properties.damage_unit > 0)
			score += 200;
		if (e->s_unit.gems > 0)
			score += 150;

		if (score > best_score)
		{
			best_score = score;
			best = e;
		}
	}
	free(opps);
	return best;
}

bool ft_is_core_threatened(t_obj *core, unsigned int radius)
{
	if (!core)
		return false;
	return (ft_get_threat_nearest(core->pos, radius) != NULL);
}

unsigned int ft_count_active_resources(void)
{
	unsigned int count = 0;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->type == OBJ_DEPOSIT || o->type == OBJ_GEM_PILE)
			count++;
	}
	return count;
}

unsigned int ft_count_enemy_combat_units(void)
{
	unsigned int count = 0;
	t_obj **opps = ft_get_units_opponent();
	for (int i = 0; opps && opps[i]; i++)
	{
		if (opps[i]->s_unit.properties.damage_unit > 0 || opps[i]->s_unit.properties.damage_core > 0)
			count++;
	}
	free(opps);
	return count;
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
				if (ft_distance(o->pos, res_pos) <= 2)
					return true;
			}
		}
	}
	return false;
}

t_obj *ft_get_resource_for_miner(t_pos unit_pos, t_pos core_pos, unsigned long unit_id)
{
	t_obj *pile = ft_get_gems_nearest(unit_pos);
	if (pile && ft_distance(unit_pos, pile->pos) <= RES_PILE_NEAR_RADIUS)
		return pile;

	t_obj *best = NULL;
	unsigned int best_score = UINT_MAX;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_DEPOSIT && obj->type != OBJ_GEM_PILE)
			continue;

		unsigned int dist_u = ft_distance(unit_pos, obj->pos);
		unsigned int dist_c = ft_distance(core_pos, obj->pos);
		unsigned int score = dist_u * RES_DIST_UNIT_WEIGHT + dist_c * RES_DIST_CORE_WEIGHT;

		if (obj->type == OBJ_GEM_PILE && score > 3)
			score -= 3; // Prefer ready-to-scoop gem piles over unbroken deposits

		if (is_claimed_by_other_miner(obj->pos, unit_id))
			score += RES_CLAIMED_PENALTY;

		score += ((unit_id * 11 + obj->id) % 3);

		if (score < best_score)
		{
			best_score = score;
			best = obj;
		}
	}
	return best;
}
