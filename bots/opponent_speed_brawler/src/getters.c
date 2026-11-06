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

t_obj *ft_get_nearest_deposit(t_pos pos) { return core_get_obj_filter_nearest(pos, is_deposit); }
t_obj *ft_get_nearest_gem_pile(t_pos pos) { return core_get_obj_filter_nearest(pos, is_gems); }
t_obj **ft_get_units_own(void) { return core_get_objs_filter(is_unit_own); }
t_obj **ft_get_units_opponent(void) { return core_get_objs_filter(is_unit_opponent); }
t_obj *ft_get_units_opponent_nearest(t_pos pos) { return core_get_obj_filter_nearest(pos, is_unit_opponent); }

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
			continue;

		unsigned long rem_hp = e->hp - pending;
		long score = 1000 - (long)(rem_hp * 20);

		if (attacker->s_unit.properties.damage_unit >= rem_hp)
			score += 500;
		if (e->s_unit.properties.damage_unit > 0)
			score += 300;

		if (score > best_score)
		{
			best_score = score;
			best = e;
		}
	}
	free(opps);
	return best;
}
