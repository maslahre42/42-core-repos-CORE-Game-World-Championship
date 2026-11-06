#include "bot.h"

bool g_reserved_tiles[MAX_GRID_SIZE][MAX_GRID_SIZE];

bool is_pos_in_bounds(t_pos pos)
{
	return (pos.x < game.grid_size && pos.y < game.grid_size);
}

void reserve_tile(t_pos pos)
{
	if (is_pos_in_bounds(pos))
		g_reserved_tiles[pos.x][pos.y] = true;
}

void unreserve_tile(t_pos pos)
{
	if (is_pos_in_bounds(pos))
		g_reserved_tiles[pos.x][pos.y] = false;
}

bool is_tile_reserved(t_pos pos)
{
	if (!is_pos_in_bounds(pos))
		return true;
	return g_reserved_tiles[pos.x][pos.y];
}

bool is_tile_walkable_for_move(t_pos pos)
{
	if (!is_pos_in_bounds(pos))
		return false;
	return !g_reserved_tiles[pos.x][pos.y];
}

void reservations_init_tick(void)
{
	memset(g_reserved_tiles, 0, sizeof(g_reserved_tiles));

	// Populate reservation grid with all existing entities on the board.
	// Units, walls, deposits, gem piles, and cores cannot be stepped onto.
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (!is_pos_in_bounds(obj->pos))
			continue;

		if (obj->type == OBJ_CORE || obj->type == OBJ_WALL ||
			obj->type == OBJ_DEPOSIT || obj->type == OBJ_GEM_PILE ||
			obj->type == OBJ_UNIT)
		{
			g_reserved_tiles[obj->pos.x][obj->pos.y] = true;
		}
	}
}
