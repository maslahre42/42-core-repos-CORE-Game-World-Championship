#include "bot.h"

static bool g_seen_enemy_unit;

bool ft_enemy_units_cleared(void)
{
	bool enemy_alive = false;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id)
		{
			g_seen_enemy_unit = true;
			enemy_alive = true;
		}
	}
	return g_seen_enemy_unit && !enemy_alive;
}

/* Readiness wraps back to positive cooldown after standing too long. */
typedef struct s_sword_wait
{
	unsigned long id, since;
	bool waiting;
	struct s_sword_wait *next;
} t_sword_wait;
static t_sword_wait *g_waits;

static void clean_waits(void)
{
	t_sword_wait **link = &g_waits;
	while (*link)
	{
		t_obj *u = core_get_obj_from_id((*link)->id);
		if (!u || u->type != OBJ_UNIT || !u->s_unit.name || strcmp(u->s_unit.name, UNIT_NAME_SWORD))
		{
			t_sword_wait *old = *link;
			*link = old->next;
			free(old);
		}
		else
			link = &(*link)->next;
	}
}

static t_sword_wait *sword_wait(unsigned long id)
{
	for (t_sword_wait *w = g_waits; w; w = w->next)
		if (w->id == id)
			return w;
	t_sword_wait *w = calloc(1, sizeof(*w));
	if (!w)
		return NULL;
	w->id = id;
	w->next = g_waits;
	g_waits = w;
	return w;
}

static bool open_melee_gap(const t_obj *u, const t_obj *enemy)
{
	const int dx[4] = {1, 0, -1, 0}, dy[4] = {0, 1, 0, -1};
	for (int d = 0; d < 4; d++)
	{
		int x = u->pos.x + dx[d], y = u->pos.y + dy[d];
		if (x < 0 || y < 0 || x >= game.grid_size || y >= game.grid_size)
			continue;
		t_pos p = {(unsigned short)x, (unsigned short)y};
		if (ft_distance(p, enemy->pos) != 1)
			continue;
		const t_obj *o = core_get_obj_from_pos(p);
		if (!o || (o->type == OBJ_GEM_PILE && o->s_deposit_gems_pile.gems <=
			u->s_unit.properties.max_gems - u->s_unit.gems))
			return true;
	}
	return false;
}

static t_obj *sword_target(const t_obj *u, const t_obj *core)
{
	t_obj *best = NULL;
	long best_score = LONG_MIN;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *enemy = game.objects[i];
		if (enemy->type != OBJ_UNIT || enemy->s_unit.team_id == game.my_team_id)
			continue;
		unsigned int distance = ft_distance(u->pos, enemy->pos);
		long score = 1000 - (long)distance * 50 - (long)enemy->hp * 5;
		if (distance == 1)
			score += 10000;
		if (enemy->s_unit.properties.damage_unit == 0)
			score += 250 + (long)enemy->s_unit.gems * 2;
		else if (distance <= 5)
			score += 500;
		if (ft_distance(enemy->pos, core->pos) <= 6 && distance <= 10 &&
			(enemy->s_unit.properties.damage_unit || enemy->s_unit.properties.damage_core))
			score += 5000;
		if (score > best_score)
		{
			best = enemy;
			best_score = score;
		}
	}
	return best;
}

#define GUARD_GRID 64

typedef struct s_guard_post
{
	const t_obj *unit;
	t_pos pos;
	unsigned int side;
	bool assigned;
} t_guard_post;

/* Determine terrain reachability once per Sword. Friendly traffic is temporary;
 * walls, deposits and Cores cannot be crossed by these pure combat units. */
static void guard_reachability(const t_obj *u, bool terrain[GUARD_GRID][GUARD_GRID],
	bool reached[GUARD_GRID][GUARD_GRID])
{
	t_pos queue[GUARD_GRID * GUARD_GRID];
	unsigned int head = 0, tail = 0;
	const int dx[4] = {0, -1, 0, 1};
	const int dy[4] = {-1, 0, 1, 0};
	memset(reached, 0, sizeof(bool) * GUARD_GRID * GUARD_GRID);
	queue[tail++] = u->pos;
	reached[u->pos.y][u->pos.x] = true;
	while (head < tail)
	{
		t_pos p = queue[head++];
		for (int d = 0; d < 4; d++)
		{
			int x = p.x + dx[d], y = p.y + dy[d];
			if (x < 0 || y < 0 || x >= game.grid_size || y >= game.grid_size ||
				terrain[y][x] || reached[y][x])
				continue;
			reached[y][x] = true;
			queue[tail++] = (t_pos){(unsigned short)x, (unsigned short)y};
		}
	}
}

static t_guard_post *plan_guard_posts(const t_obj *core, const t_obj *enemy_core, t_obj **units, unsigned int *count)
{
	*count = 0;
	if (game.grid_size > GUARD_GRID)
		return NULL;
	bool terrain[GUARD_GRID][GUARD_GRID] = {{false}};
	const t_obj *occupants[GUARD_GRID][GUARD_GRID] = {{NULL}};
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id)
			return NULL;
		if (o->pos.x >= game.grid_size || o->pos.y >= game.grid_size)
			continue;
		occupants[o->pos.y][o->pos.x] = o;
		terrain[o->pos.y][o->pos.x] = o->type == OBJ_WALL || o->type == OBJ_DEPOSIT || o->type == OBJ_CORE;
	}
	for (int i = 0; units && units[i]; i++)
		if (units[i]->s_unit.name && !strcmp(units[i]->s_unit.name, UNIT_NAME_SWORD))
			(*count)++;
	if (!*count)
		return NULL;
	t_guard_post *posts = calloc(*count, sizeof(*posts));
	if (!posts)
	{
		*count = 0;
		return NULL;
	}
	unsigned int n = 0;
	for (int i = 0; units && units[i]; i++)
		if (units[i]->s_unit.name && !strcmp(units[i]->s_unit.name, UNIT_NAME_SWORD))
			posts[n++].unit = units[i];
	/* Plan for all Swords, including cooling-down ones, in stable ID order. */
	for (unsigned int i = 1; i < *count; i++)
	{
		t_guard_post p = posts[i];
		unsigned int j = i;
		while (j && posts[j - 1].unit->id > p.unit->id)
		{
			posts[j] = posts[j - 1];
			j--;
		}
		posts[j] = p;
	}
	bool lane[GUARD_GRID][GUARD_GRID];
	nav_guard_lane(core, enemy_core, units, lane);
	unsigned int use[4] = {0};
	bool claimed[GUARD_GRID][GUARD_GRID] = {{false}};
	const int dx[4] = {0, -1, 0, 1};
	const int dy[4] = {-1, 0, 1, 0};
	for (unsigned int i = 0; i < *count; i++)
	{
		const t_obj *u = posts[i].unit;
		if (u->pos.x >= game.grid_size || u->pos.y >= game.grid_size)
			continue;
		bool reached[GUARD_GRID][GUARD_GRID];
		guard_reachability(u, terrain, reached);
		unsigned int best = UINT_MAX;
		for (unsigned int side = 0; side < 4; side++)
		{
			int ax = enemy_core->pos.x + dx[side], ay = enemy_core->pos.y + dy[side];
			if (ax < 0 || ay < 0 || ax >= game.grid_size || ay >= game.grid_size || terrain[ay][ax])
				continue;
			for (unsigned short y = 0; y < game.grid_size; y++)
				for (unsigned short x = 0; x < game.grid_size; x++)
				{
					t_pos p = {x, y};
					unsigned int d = ft_distance(p, enemy_core->pos);
					int offset_x = (int)x - enemy_core->pos.x;
					int offset_y = (int)y - enemy_core->pos.y;
					if (d < 2 || d > 5 || lane[y][x] || offset_x * dx[side] + offset_y * dy[side] <= 0 ||
						!reached[y][x] || claimed[y][x] || nav_is_reserved(p) ||
						(occupants[y][x] && occupants[y][x] != u))
						continue;
					unsigned int ideal_distance = abs(offset_x - 2 * dx[side]) + abs(offset_y - 2 * dy[side]);
					unsigned int score = use[side] * 10000 + ideal_distance * 200 + (d > 3 ? (d - 3) * 2000 : 0) + ft_distance(u->pos, p);
					if (score < best)
					{
						best = score;
						posts[i].pos = p;
						posts[i].side = side;
						posts[i].assigned = true;
					}
				}
		}
		if (posts[i].assigned)
		{
			claimed[posts[i].pos.y][posts[i].pos.x] = true;
			use[posts[i].side]++;
		}
	}
	return posts;
}

static void secure_core_approach(const t_obj *u, t_guard_post *posts, unsigned int count)
{
	const char *sides[] = {"above", "left", "below", "right"};
	for (unsigned int i = 0; i < count; i++)
		if (posts[i].unit == u && posts[i].assigned)
		{
			core_debug_addObjectInfo(u, "GUARD %s (%u,%u); keep siege access", sides[posts[i].side],
				posts[i].pos.x, posts[i].pos.y);
			if (ft_distance(u->pos, posts[i].pos))
				nav_move_to_exact(u, posts[i].pos, false);
			return;
		}
	core_debug_addObjectInfo(u, "GUARD waiting for an open reachable post");
}

void handle_swords(t_obj *core, t_obj *opp_core, t_obj **units)
{
	if (!core || !opp_core)
		return;
	ft_enemy_units_cleared();
	clean_waits();
	unsigned int post_count;
	t_guard_post *posts = plan_guard_posts(core, opp_core, units, &post_count);
	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, UNIT_NAME_SWORD) != 0 ||
			(u->s_unit.action_cooldown > 0 || nav_unit_yielded(u)))
			continue;
		t_sword_wait *wait = sword_wait(u->id);
		t_obj *target = sword_target(u, core);
		if (!target)
		{
			if (wait)
				wait->waiting = false;
			secure_core_approach(u, posts, post_count);
			continue;
		}
		if (ft_distance(u->pos, target->pos) == 1)
		{
			if (wait)
				wait->waiting = false;
			core_debug_addObjectInfo(u, "SWORD kill enemy #%lu", target->id);
			core_action_attack(u, target);
			continue;
		}
		bool hold = false;
		unsigned long step = u->s_unit.properties.gems_per_cooldown_step;
		long move_cooldown = u->s_unit.properties.base_action_cooldown +
			(long)(u->s_unit.gems / (step ? step : 1));
		if (move_cooldown < 1)
			move_cooldown = 1;
		for (int j = 0; game.objects && game.objects[j]; j++)
		{
			const t_obj *enemy = game.objects[j];
			if (enemy->type == OBJ_UNIT && enemy->s_unit.team_id != game.my_team_id &&
				enemy->s_unit.properties.damage_unit > 0 && ft_distance(u->pos, enemy->pos) == 2 &&
				enemy->s_unit.action_cooldown < move_cooldown && open_melee_gap(u, enemy))
				hold = true;
		}
		if (hold && wait)
		{
			if (!wait->waiting)
			{
				wait->waiting = true;
				wait->since = game.elapsed_ticks;
			}
			hold = game.elapsed_ticks - wait->since < 6;
		}
		else
		{
			hold = false;
			if (wait)
				wait->waiting = false;
		}
		core_debug_addObjectInfo(u, hold ? "SWORD hold ready (bounded)" : "SWORD pursue #%lu", target->id);
		if (!hold)
			nav_move_adjacent_to(u, target->pos, false);
	}
	free(posts);
}
