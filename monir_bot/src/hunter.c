#include "bot.h"

static unsigned long g_defense_target;

static t_obj *defense_target(const t_obj *core)
{
	t_obj *previous = core_get_obj_from_id(g_defense_target);
	t_obj *best = NULL;
	long best_score = LONG_MIN;
	for (int i = 0; core && game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->type != OBJ_UNIT || o->s_unit.team_id == game.my_team_id ||
			o->s_unit.properties.damage_unit + o->s_unit.properties.damage_core == 0)
			continue;
		unsigned int distance = ft_distance(o->pos, core->pos);
		if (distance > 12)
			continue;
		long score = 1000 - distance * 50 - (long)o->hp * 5;
		if (o == previous)
			score += 60;
		if (score > best_score)
		{
			best_score = score;
			best = o;
		}
	}
	g_defense_target = best ? best->id : 0;
	return best;
}

static void guard(const t_obj *u, const t_obj *core, const t_obj *enemy, unsigned int index)
{
	/* Guard outside the two Core-adjacent unloading/spawning tiles. */
	t_pos preferred = core->pos;
	int direction_x = enemy && enemy->pos.x < core->pos.x ? -1 : 1;
	int direction_y = enemy && enemy->pos.y < core->pos.y ? -1 : 1;
	int px = (int)core->pos.x + direction_x * (index % 2 ? 1 : 3);
	int py = (int)core->pos.y + direction_y * (index % 2 ? 3 : 1);
	if (px >= 0 && py >= 0 && px < game.grid_size && py < game.grid_size)
		preferred = (t_pos){(unsigned short)px, (unsigned short)py};
	t_pos post = u->pos;
	unsigned int best_score = UINT_MAX;
	for (unsigned short y = 0; y < game.grid_size; y++)
		for (unsigned short x = 0; x < game.grid_size; x++)
		{
			t_pos p = {x, y};
			unsigned int home = ft_distance(p, core->pos);
			const t_obj *occupant = core_get_obj_from_pos(p);
			if (home < 3 || home > 5 || (occupant && occupant != u) || nav_is_reserved(p))
				continue;
			unsigned int score = ft_distance(p, preferred) * 20 + ft_distance(p, u->pos);
			if (score < best_score)
			{
				best_score = score;
				post = p;
			}
		}
	core_debug_addObjectInfo(u, "GUARD (%u,%u), keep Core unloading lanes clear", post.x, post.y);
	if (u->s_unit.action_cooldown <= 0 && ft_distance(u->pos, post) > 0)
		nav_move_to_exact(u, post, false);
}

void handle_hunters(t_obj *core, t_obj *opp_core, t_obj **units)
{
	if (!core)
		return;
	t_obj *target = defense_target(core);
	unsigned int index = 0;
	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, UNIT_NAME_HUNTER) != 0 || nav_unit_yielded(u))
			continue;
		t_obj *adjacent = ft_get_adjacent_enemy(u->pos);
		if (adjacent)
		{
			core_debug_addObjectInfo(u, "DEFEND attack #%lu", adjacent->id);
			if (u->s_unit.action_cooldown <= 0)
				core_action_attack(u, adjacent);
		}
		else if (target)
		{
			core_debug_addObjectInfo(u, "INTERCEPT shared threat #%lu", target->id);
			if (u->s_unit.action_cooldown <= 0)
				nav_move_adjacent_to(u, target->pos, false);
		}
		else
			guard(u, core, opp_core, index);
		index++;
	}
}
