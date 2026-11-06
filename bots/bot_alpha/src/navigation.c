#include "bot.h"

bool g_reserved_tiles[64][64];

typedef struct s_pq_node
{
	unsigned int cost;
	t_pos pos;
} t_pq_node;

typedef struct s_min_heap
{
	t_pq_node data[4096];
	int size;
} t_min_heap;

static void heap_init(t_min_heap *h)
{
	h->size = 0;
}

static void heap_push(t_min_heap *h, unsigned int cost, t_pos pos)
{
	if (h->size >= 4095)
		return;
	int i = h->size++;
	h->data[i].cost = cost;
	h->data[i].pos = pos;
	while (i > 0)
	{
		int parent = (i - 1) / 2;
		if (h->data[parent].cost <= h->data[i].cost)
			break;
		t_pq_node tmp = h->data[i];
		h->data[i] = h->data[parent];
		h->data[parent] = tmp;
		i = parent;
	}
}

static bool heap_pop(t_min_heap *h, t_pq_node *out)
{
	if (h->size <= 0)
		return false;
	*out = h->data[0];
	h->size--;
	if (h->size > 0)
	{
		h->data[0] = h->data[h->size];
		int i = 0;
		while (1)
		{
			int left = 2 * i + 1;
			int right = 2 * i + 2;
			int smallest = i;

			if (left < h->size && h->data[left].cost < h->data[smallest].cost)
				smallest = left;
			if (right < h->size && h->data[right].cost < h->data[smallest].cost)
				smallest = right;
			if (smallest == i)
				break;
			t_pq_node tmp = h->data[i];
			h->data[i] = h->data[smallest];
			h->data[smallest] = tmp;
			i = smallest;
		}
	}
	return true;
}

void nav_init_tick(void)
{
	memset(g_reserved_tiles, 0, sizeof(g_reserved_tiles));

	// 1. Mark static obstacles and all entities currently on the board
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *obj = game.objects[i];
		if (obj->pos.x < 64 && obj->pos.y < 64)
		{
			// Walls, Cores, Deposits, Gem Piles are completely impassable
			if (obj->type == OBJ_WALL || obj->type == OBJ_CORE ||
				obj->type == OBJ_DEPOSIT || obj->type == OBJ_GEM_PILE)
			{
				g_reserved_tiles[obj->pos.y][obj->pos.x] = true;
			}
			// All units (friendly & enemy) initially reserve their own tile
			else if (obj->type == OBJ_UNIT)
			{
				g_reserved_tiles[obj->pos.y][obj->pos.x] = true;
			}
		}
	}
}

bool nav_is_valid_tile(t_pos pos)
{
	return (pos.x < game.grid_size && pos.y < game.grid_size);
}

bool nav_is_reserved(t_pos pos)
{
	if (!nav_is_valid_tile(pos))
		return true;
	return g_reserved_tiles[pos.y][pos.x];
}

void nav_reserve_tile(t_pos pos)
{
	if (nav_is_valid_tile(pos))
		g_reserved_tiles[pos.y][pos.x] = true;
}

static bool is_border_tile(t_pos pos)
{
	if (!nav_is_valid_tile(pos))
		return false;
	unsigned short max_coord = game.grid_size - 1;
	return (pos.x == 0 || pos.x == max_coord || pos.y == 0 || pos.y == max_coord);
}

static bool nav_find_next_step(const t_obj *unit, t_pos target, bool target_is_adjacent, bool hug_borders, t_pos *out_step)
{
	if (!unit || !nav_is_valid_tile(unit->pos))
		return false;

	// Check if already at goal
	if (target_is_adjacent && ft_is_adjacent(unit->pos, target))
		return false;
	if (!target_is_adjacent && unit->pos.x == target.x && unit->pos.y == target.y)
		return false;

	static unsigned int dist[64][64];
	static t_pos parent[64][64];
	static bool visited[64][64];
	t_min_heap heap;

	memset(dist, 0x3F, sizeof(dist)); // large initial cost
	memset(visited, 0, sizeof(visited));
	heap_init(&heap);

	dist[unit->pos.y][unit->pos.x] = 0;
	heap_push(&heap, 0, unit->pos);

	t_pos goal_pos = {65535, 65535};
	bool goal_found = false;

	const int dx[4] = {0, 0, 1, -1};
	const int dy[4] = {1, -1, 0, 0};

	while (heap.size > 0)
	{
		t_pq_node curr;
		if (!heap_pop(&heap, &curr))
			break;

		if (visited[curr.pos.y][curr.pos.x])
			continue;
		visited[curr.pos.y][curr.pos.x] = true;

		// Check if goal reached
		if (curr.pos.x != unit->pos.x || curr.pos.y != unit->pos.y)
		{
			if (target_is_adjacent && ft_is_adjacent(curr.pos, target))
			{
				goal_pos = curr.pos;
				goal_found = true;
				break;
			}
			else if (!target_is_adjacent && curr.pos.x == target.x && curr.pos.y == target.y)
			{
				goal_pos = curr.pos;
				goal_found = true;
				break;
			}
		}

		for (int d = 0; d < 4; d++)
		{
			int nx = (int)curr.pos.x + dx[d];
			int ny = (int)curr.pos.y + dy[d];

			if (nx < 0 || nx >= game.grid_size || ny < 0 || ny >= game.grid_size)
				continue;

			t_pos neighbor = {(unsigned short)nx, (unsigned short)ny};

			if (visited[neighbor.y][neighbor.x])
				continue;

			// If expanding directly from the unit's start position, neighbor MUST not be reserved this tick
			if (curr.pos.x == unit->pos.x && curr.pos.y == unit->pos.y)
			{
				if (g_reserved_tiles[neighbor.y][neighbor.x])
					continue;
			}
			else
			{
				// For deeper path steps, impassable terrain like walls, cores, deposits cannot be crossed
				if (g_reserved_tiles[neighbor.y][neighbor.x])
				{
					// If target is an exact empty waypoint, don't walk into obstacles
					continue;
				}
			}

			// Edge cost calculation: Hug outer border tiles if requested
			unsigned int step_cost = 1;
			if (hug_borders)
			{
				if (is_border_tile(neighbor))
					step_cost = 1;
				else
					step_cost = 15; // penalty for moving inward
			}

			unsigned int new_cost = dist[curr.pos.y][curr.pos.x] + step_cost;
			if (new_cost < dist[neighbor.y][neighbor.x])
			{
				dist[neighbor.y][neighbor.x] = new_cost;
				parent[neighbor.y][neighbor.x] = curr.pos;
				heap_push(&heap, new_cost, neighbor);
			}
		}
	}

	if (!goal_found)
		return false;

	// Trace back from goal_pos to find the first step adjacent to unit->pos
	t_pos trace = goal_pos;
	while (trace.x != unit->pos.x || trace.y != unit->pos.y)
	{
		t_pos p = parent[trace.y][trace.x];
		if (p.x == unit->pos.x && p.y == unit->pos.y)
		{
			*out_step = trace;
			return true;
		}
		trace = p;
	}

	return false;
}

bool nav_move_adjacent_to(const t_obj *unit, t_pos target, bool hug_borders)
{
	t_pos step;
	if (nav_find_next_step(unit, target, true, hug_borders, &step))
	{
		g_reserved_tiles[step.y][step.x] = true;
		g_reserved_tiles[unit->pos.y][unit->pos.x] = true;
		core_action_move(unit, step);
		core_debug_addObjectPathStep(unit, step);
		return true;
	}
	// If no path step found or already adjacent, stay put and reserve current spot
	g_reserved_tiles[unit->pos.y][unit->pos.x] = true;
	return false;
}

bool nav_move_to_exact(const t_obj *unit, t_pos target, bool hug_borders)
{
	t_pos step;
	if (nav_find_next_step(unit, target, false, hug_borders, &step))
	{
		g_reserved_tiles[step.y][step.x] = true;
		g_reserved_tiles[unit->pos.y][unit->pos.x] = true;
		core_action_move(unit, step);
		core_debug_addObjectPathStep(unit, step);
		return true;
	}
	// Fallback: if exact tile is blocked/unreachable, try moving adjacent to it
	return nav_move_adjacent_to(unit, target, hug_borders);
}
