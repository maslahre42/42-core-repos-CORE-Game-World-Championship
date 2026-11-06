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

t_pos path_find_step(t_pos start, t_pos target, bool target_adjacent)
{
	if (!is_pos_in_bounds(start) || !is_pos_in_bounds(target))
		return start;

	if (target_adjacent && ft_distance(start, target) <= 1)
		return start;
	if (!target_adjacent && start.x == target.x && start.y == target.y)
		return start;

	static t_pos queue[4096];
	static bool visited[MAX_GRID_SIZE][MAX_GRID_SIZE];
	static t_pos parent[MAX_GRID_SIZE][MAX_GRID_SIZE];

	memset(visited, 0, sizeof(visited));

	int head = 0;
	int tail = 0;

	queue[tail++] = start;
	visited[start.x][start.y] = true;
	parent[start.x][start.y] = start;

	t_pos dest = start;
	bool found = false;

	const int dx[4] = {0, 0, 1, -1};
	const int dy[4] = {1, -1, 0, 0};

	while (head < tail)
	{
		t_pos curr = queue[head++];

		if (target_adjacent)
		{
			if (ft_distance(curr, target) <= 1 && !(curr.x == start.x && curr.y == start.y))
			{
				dest = curr;
				found = true;
				break;
			}
		}
		else
		{
			if (curr.x == target.x && curr.y == target.y && !(curr.x == start.x && curr.y == start.y))
			{
				dest = curr;
				found = true;
				break;
			}
		}

		for (int d = 0; d < 4; d++)
		{
			int nx = (int)curr.x + dx[d];
			int ny = (int)curr.y + dy[d];

			if (nx < 0 || ny < 0 || nx >= (int)game.grid_size || ny >= (int)game.grid_size)
				continue;

			t_pos next_p = {(unsigned short)nx, (unsigned short)ny};

			if (visited[next_p.x][next_p.y])
				continue;

			// Intermediate steps must be non-reserved (open empty tiles)
			if (!target_adjacent && next_p.x == target.x && next_p.y == target.y)
			{
				if (g_reserved_tiles[next_p.x][next_p.y])
					continue;
			}
			else
			{
				if (g_reserved_tiles[next_p.x][next_p.y])
					continue;
			}

			visited[next_p.x][next_p.y] = true;
			parent[next_p.x][next_p.y] = curr;
			queue[tail++] = next_p;
		}
	}

	if (!found)
		return start;

	t_pos step = dest;
	while (!(parent[step.x][step.y].x == start.x && parent[step.x][step.y].y == start.y))
	{
		step = parent[step.x][step.y];
	}

	return step;
}
