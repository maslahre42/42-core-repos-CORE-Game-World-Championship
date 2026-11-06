#include "bot.h"

#define NAV_SIZE 64
#define INF (UINT_MAX / 4)

bool g_reserved_tiles[NAV_SIZE][NAV_SIZE];
static const t_obj *g_occupants[NAV_SIZE][NAV_SIZE];
static unsigned int g_danger[NAV_SIZE][NAV_SIZE];
static bool g_worker_lane[NAV_SIZE][NAV_SIZE];
static bool g_yield_request[NAV_SIZE][NAV_SIZE];
static bool g_yielded[NAV_SIZE][NAV_SIZE];

void nav_init_tick(void)
{
	memset(g_reserved_tiles, 0, sizeof(g_reserved_tiles));
	memset(g_danger, 0, sizeof(g_danger));
	memset(g_worker_lane, 0, sizeof(g_worker_lane));
	memset(g_yield_request, 0, sizeof(g_yield_request));
	memset(g_yielded, 0, sizeof(g_yielded));
	memset(g_occupants, 0, sizeof(g_occupants));
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *o = game.objects[i];
		if (o->pos.x < NAV_SIZE && o->pos.y < NAV_SIZE)
			g_occupants[o->pos.y][o->pos.x] = o;
		if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id && o->s_unit.properties.damage_unit)
			for (int dy = -2; dy <= 2; dy++)
				for (int dx = -2; dx <= 2; dx++)
				{
					int x = o->pos.x + dx, y = o->pos.y + dy;
					if (abs(dx) + abs(dy) <= 2 && x >= 0 && y >= 0 && x < NAV_SIZE && y < NAV_SIZE)
						g_danger[y][x] += abs(dx) + abs(dy) <= 1 ? 100 : 20;
				}
	}
}

bool nav_is_valid_tile(t_pos pos)
{
	return pos.x < game.grid_size && pos.y < game.grid_size &&
		pos.x < NAV_SIZE && pos.y < NAV_SIZE;
}

bool nav_is_reserved(t_pos pos)
{
	return !nav_is_valid_tile(pos) || g_reserved_tiles[pos.y][pos.x];
}

void nav_reserve_tile(t_pos pos)
{
	if (nav_is_valid_tile(pos))
		g_reserved_tiles[pos.y][pos.x] = true;
}

/* Path cost includes the attacks needed to clear an obstacle. Cores are
 * approached, never crossed. Friendly units may vacate future path tiles. */
static unsigned int tile_cost(const t_obj *unit, t_pos pos, bool planning_lane, bool allow_friendly)
{
	const t_obj *o = g_occupants[pos.y][pos.x];
	if (g_reserved_tiles[pos.y][pos.x] && !planning_lane && !allow_friendly)
		return INF;
	unsigned int danger = unit->s_unit.properties.damage_unit ? 0 : g_danger[pos.y][pos.x];
	if (danger >= 100)
		return INF;
	if (!o || o == unit)
		return 1 + danger;
	if (o->type == OBJ_CORE)
		return INF;
	if (o->type == OBJ_GEM_PILE)
	{
		/* A full unit cannot empty a pile to move through its tile. */
		if (o->s_deposit_gems_pile.gems > unit->s_unit.properties.max_gems - unit->s_unit.gems)
			return INF;
		return 2 + danger;
	}
	unsigned long damage = unit->s_unit.properties.damage_object;
	if (o->type == OBJ_UNIT)
	{
		if (o->s_unit.team_id == game.my_team_id)
			return planning_lane || allow_friendly ? 1 : 80;
		damage = unit->s_unit.properties.damage_unit;
		if (o->s_unit.properties.damage_reduction_percent > 0)
			damage = damage * (100 - o->s_unit.properties.damage_reduction_percent) / 100;
	}
	if (damage == 0)
		return INF;
	return 2 + (unsigned int)((o->hp + damage - 1) / damage) + danger;
}

typedef struct s_node
{
	unsigned int cost;
	t_pos pos;
} t_node;

/* At most four relaxations per grid tile; heap entries can be stale. */
static bool next_step(const t_obj *unit, t_pos target, bool adjacent, t_pos *step,
	bool planning_lane, bool allow_friendly, bool lane[NAV_SIZE][NAV_SIZE])
{
	static unsigned int dist[NAV_SIZE][NAV_SIZE];
	static t_pos parent[NAV_SIZE][NAV_SIZE];
	t_node heap[NAV_SIZE * NAV_SIZE * 4 + 1];
	unsigned int size = 0;
	memset(dist, 0x3f, sizeof(dist));
	dist[unit->pos.y][unit->pos.x] = 0;
	heap[size++] = (t_node){0, unit->pos};
	const int dx[4] = {1, 0, -1, 0};
	const int dy[4] = {0, 1, 0, -1};
	while (size)
	{
		t_node node = heap[0];
		heap[0] = heap[--size];
		unsigned int at = 0;
		while (at * 2 + 1 < size)
		{
			unsigned int child = at * 2 + 1;
			if (child + 1 < size && heap[child + 1].cost < heap[child].cost)
				child++;
			if (heap[at].cost <= heap[child].cost)
				break;
			t_node tmp = heap[at]; heap[at] = heap[child]; heap[child] = tmp;
			at = child;
		}
		if (node.cost != dist[node.pos.y][node.pos.x])
			continue;
		unsigned int distance = ft_distance(node.pos, target);
		if ((adjacent && distance == 1) || (!adjacent && distance == 0))
		{
			t_pos trace = node.pos;
			if (lane)
			{
				while (true)
				{
					lane[trace.y][trace.x] = true;
					if (!ft_distance(trace, unit->pos))
						break;
					trace = parent[trace.y][trace.x];
				}
				if (planning_lane)
					return true;
				trace = node.pos;
			}
			if (ft_distance(trace, unit->pos) == 0)
				return false;
			while (ft_distance(parent[trace.y][trace.x], unit->pos) != 0)
				trace = parent[trace.y][trace.x];
			*step = trace;
			return true;
		}
		for (int d = 0; d < 4; d++)
		{
			int x = node.pos.x + dx[d], y = node.pos.y + dy[d];
			if (x < 0 || y < 0 || x >= game.grid_size || y >= game.grid_size)
				continue;
			t_pos next = {(unsigned short)x, (unsigned short)y};
			unsigned int cost = tile_cost(unit, next, planning_lane, allow_friendly);
			/* Prefer the diagonal access lane so the two cardinal guard posts
			 * can still cover opposite approaches when the terrain permits. */
			if (planning_lane && ft_distance(next, target) == 2 &&
				(next.x == target.x || next.y == target.y) && cost < INF)
				cost += 50;
			if (cost >= INF || node.cost + cost >= dist[y][x])
				continue;
			/* Never plan an immediate step through a friendly unit. */
			const t_obj *o = g_occupants[y][x];
			if (!planning_lane && !allow_friendly && node.cost == 0 && o && o->type == OBJ_UNIT &&
				o->s_unit.team_id == game.my_team_id)
				continue;
			dist[y][x] = node.cost + cost;
			parent[y][x] = node.pos;
			if (size >= NAV_SIZE * NAV_SIZE * 4)
				return false;
			unsigned int index = size++;
			heap[index] = (t_node){dist[y][x], next};
			while (index > 0 && heap[index].cost < heap[(index - 1) / 2].cost)
			{
				unsigned int up = (index - 1) / 2;
				t_node tmp = heap[index]; heap[index] = heap[up]; heap[up] = tmp;
				index = up;
			}
		}
	}
	return false;
}

/* Guard destinations avoid this corridor; it is not a movement reservation,
 * so the finisher itself can use it once blocking guards have stepped aside. */
bool nav_guard_lane(const t_obj *core, const t_obj *enemy_core, t_obj **units, bool lane[NAV_SIZE][NAV_SIZE])
{
	memset(lane, 0, sizeof(bool) * NAV_SIZE * NAV_SIZE);
	if (!core || !enemy_core || game.grid_size > NAV_SIZE)
		return false;
	t_obj future = {.type = OBJ_UNIT, .pos = core->pos};
	future.s_unit.team_id = game.my_team_id;
	future.s_unit.properties.max_gems = 16;
	future.s_unit.properties.damage_object = 3;
	const t_obj *finisher = &future;
	for (int i = 0; units && units[i]; i++)
		if (units[i]->s_unit.name && !strcmp(units[i]->s_unit.name, UNIT_NAME_FLANK_BREAKER))
		{
			finisher = units[i];
			break;
		}
	if (!nav_is_valid_tile(finisher->pos) || !nav_is_valid_tile(enemy_core->pos))
		return false;
	t_pos unused;
	return next_step(finisher, enemy_core->pos, true, &unused, true, true, lane);
}

static bool navigate(const t_obj *unit, t_pos target, bool adjacent)
{
	if (!unit || unit->s_unit.action_cooldown > 0)
		return false;
	if (game.grid_size > NAV_SIZE)
	{
		core_action_travel(unit, target, NULL);
		return true;
	}
	if (!nav_is_valid_tile(unit->pos) || !nav_is_valid_tile(target))
		return false;
	t_pos step;
	if (!next_step(unit, target, adjacent, &step, false, false, NULL))
	{
		bool worker = unit->s_unit.name && (!strcmp(unit->s_unit.name, UNIT_NAME_RUNNER) ||
			!strcmp(unit->s_unit.name, UNIT_NAME_MINER));
		if (worker && next_step(unit, target, adjacent, &step, false, true, g_worker_lane))
		{
			const t_obj *blocker = g_occupants[step.y][step.x];
			if (blocker && blocker->type == OBJ_UNIT && blocker->s_unit.team_id == game.my_team_id)
				g_yield_request[step.y][step.x] = true;
		}
		return false;
	}
	const t_obj *o = g_occupants[step.y][step.x];
	if (o)
	{
		if (o->type == OBJ_CORE || (o->type == OBJ_UNIT && o->s_unit.team_id == game.my_team_id))
			return false;
		core_action_attack(unit, o);
	}
	else
	{
		core_action_move(unit, step);
		nav_reserve_tile(step);
	}
	core_debug_addObjectPathStep(unit, step);
	return true;
}

bool nav_move_adjacent_to(const t_obj *unit, t_pos target, bool hug_borders)
{
	(void)hug_borders;
	return navigate(unit, target, true);
}

bool nav_move_to_exact(const t_obj *unit, t_pos target, bool hug_borders)
{
	(void)hug_borders;
	return navigate(unit, target, false);
}

bool nav_can_reach(const t_obj *unit, t_pos target, bool adjacent, bool allow_friendly)
{
	if (!unit || game.grid_size > NAV_SIZE || !nav_is_valid_tile(unit->pos) || !nav_is_valid_tile(target))
		return false;
	unsigned int distance = ft_distance(unit->pos, target);
	if (distance == (adjacent ? 1U : 0U))
		return true;
	t_pos step;
	return next_step(unit, target, adjacent, &step, false, allow_friendly, NULL);
}

bool nav_unit_yielded(const t_obj *unit)
{
	return unit && nav_is_valid_tile(unit->pos) && g_yielded[unit->pos.y][unit->pos.x];
}

void nav_yield_for_workers(const t_obj *core, const t_obj *enemy_core, t_obj **units)
{
	if (!core || !enemy_core || game.grid_size > NAV_SIZE)
		return;
	bool workers = false;
	for (int i = 0; units && units[i]; i++)
		if (units[i]->s_unit.name && (!strcmp(units[i]->s_unit.name, UNIT_NAME_RUNNER) ||
			!strcmp(units[i]->s_unit.name, UNIT_NAME_MINER)))
			workers = true;
	const int dx[4] = {1, 0, -1, 0}, dy[4] = {0, 1, 0, -1};
	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || (strcmp(u->s_unit.name, UNIT_NAME_SWORD) &&
			strcmp(u->s_unit.name, UNIT_NAME_HUNTER)) || u->s_unit.action_cooldown > 0 ||
			!nav_is_valid_tile(u->pos))
			continue;
		if (!g_yield_request[u->pos.y][u->pos.x] && !(workers && ft_distance(u->pos, core->pos) == 1))
			continue;
		/* Fight adjacent enemies instead of giving up a necessary interception. */
		if (ft_get_adjacent_enemy(u->pos))
			continue;
		unsigned int best = UINT_MAX;
		t_pos destination = u->pos;
		for (int d = 0; d < 4; d++)
		{
			int x = u->pos.x + dx[d], y = u->pos.y + dy[d];
			if (x < 0 || y < 0 || x >= game.grid_size || y >= game.grid_size)
				continue;
			t_pos p = {(unsigned short)x, (unsigned short)y};
			if (g_occupants[y][x] || nav_is_reserved(p))
				continue;
			unsigned int score = ft_distance(p, enemy_core->pos) +
				(g_worker_lane[y][x] ? 1000 : 0) + (ft_distance(p, core->pos) == 1 ? 500 : 0);
			if (score < best)
			{
				best = score;
				destination = p;
			}
		}
		if (best != UINT_MAX)
		{
			core_debug_addObjectInfo(u, "YIELD worker corridor (%u,%u)", destination.x, destination.y);
			core_action_move(u, destination);
			nav_reserve_tile(destination);
			g_yielded[u->pos.y][u->pos.x] = true;
		}
	}
}
