#include "bot.h"

bool is_tile_walkable_neighbor(t_pos pos)
{
	const int dx[4] = {0, 0, 1, -1};
	const int dy[4] = {1, -1, 0, 0};

	for (int d = 0; d < 4; d++)
	{
		int nx = (int)pos.x + dx[d];
		int ny = (int)pos.y + dy[d];

		if (nx < 0 || ny < 0 || nx >= (int)game.grid_size || ny >= (int)game.grid_size)
			continue;

		t_pos p = {(unsigned short)nx, (unsigned short)ny};
		t_obj *obj = core_get_obj_from_pos(p);

		// If neighbor tile is not a wall, deposit, or core, a unit can stand there
		if (!obj || (obj->type != OBJ_WALL && obj->type != OBJ_DEPOSIT && obj->type != OBJ_CORE))
			return true;
	}
	return false;
}

t_obj *ft_get_core_own(void)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type == OBJ_CORE && obj->s_core.team_id == game.my_team_id)
			return obj;
	}
	return NULL;
}

t_obj *ft_get_core_opponent(void)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type == OBJ_CORE && obj->s_core.team_id != game.my_team_id)
			return obj;
	}
	return NULL;
}

t_pos ft_get_choke_point(int choke_index)
{
	t_obj *core = ft_get_core_own();
	t_pos choke = {0, 0};
	if (!core)
		return choke;

	if (core->pos.x == 0 && core->pos.y == 0)
	{
		if (choke_index == 0)
			choke = (t_pos){2, 0};
		else if (choke_index == 1)
			choke = (t_pos){0, 2};
		else
			choke = (t_pos){1, 1};
	}
	else
	{
		unsigned short max_x = (game.grid_size > 0) ? game.grid_size - 1 : 19;
		unsigned short max_y = (game.grid_size > 0) ? game.grid_size - 1 : 19;
		if (choke_index == 0)
			choke = (t_pos){(unsigned short)(max_x >= 2 ? max_x - 2 : 0), max_y};
		else if (choke_index == 1)
			choke = (t_pos){max_x, (unsigned short)(max_y >= 2 ? max_y - 2 : 0)};
		else
			choke = (t_pos){(unsigned short)(max_x >= 1 ? max_x - 1 : 0), (unsigned short)(max_y >= 1 ? max_y - 1 : 0)};
	}

	return choke;
}

t_obj *ft_get_nearest_gem_pile(t_pos pos)
{
	t_obj *best = NULL;
	unsigned int best_dist = UINT_MAX;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_GEM_PILE)
			continue;
		if (obj->s_deposit_gems_pile.gems == 0)
			continue;
		if (!is_tile_walkable_neighbor(obj->pos))
			continue;

		unsigned int dist = ft_distance(pos, obj->pos);
		if (dist < best_dist)
		{
			best_dist = dist;
			best = obj;
		}
	}
	return best;
}

t_obj *ft_get_best_deposit_for_miner(t_pos miner_pos, unsigned long miner_id)
{
	t_obj *best = NULL;
	unsigned int best_score = UINT_MAX;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_DEPOSIT || obj->hp == 0)
			continue;

		// Ensure deposit is not completely walled in / surrounded
		if (!is_tile_walkable_neighbor(obj->pos))
			continue;

		// Ensure a reachable path exists to this deposit
		if (!ft_is_adjacent(miner_pos, obj->pos))
		{
			t_pos test_step = path_find_step(miner_pos, obj->pos, true);
			if (test_step.x == miner_pos.x && test_step.y == miner_pos.y)
				continue; // Unreachable
		}

		unsigned int dist = ft_distance(miner_pos, obj->pos);
		unsigned int score = dist * 2;

		// Check if another friendly miner is already assigned/adjacent to this deposit
		for (int j = 0; game.objects && game.objects[j]; j++)
		{
			t_obj *other = game.objects[j];
			if (other->type == OBJ_UNIT && other->s_unit.team_id == game.my_team_id &&
				other->id != miner_id && other->s_unit.name &&
				strcmp(other->s_unit.name, ROLE_MINER) == 0)
			{
				if (ft_is_adjacent(other->pos, obj->pos))
				{
					score += 40; // Penalty to encourage spreading across deposits
					break;
				}
			}
		}

		// Small tie-breaking variance
		score += ((miner_id * 7 + obj->id) % 3);

		if (score < best_score)
		{
			best_score = score;
			best = obj;
		}
	}
	return best;
}

t_obj *ft_get_adjacent_deposit_or_pile(t_pos pos)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_DEPOSIT && obj->type != OBJ_GEM_PILE)
			continue;
		if (obj->hp == 0 && obj->type == OBJ_DEPOSIT)
			continue;
		if (obj->type == OBJ_GEM_PILE && obj->s_deposit_gems_pile.gems == 0)
			continue;

		if (ft_is_adjacent(pos, obj->pos))
			return obj;
	}
	return NULL;
}

t_obj *ft_get_adjacent_enemy(t_pos pos)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_UNIT || obj->s_unit.team_id == game.my_team_id)
			continue;
		if (obj->hp == 0)
			continue;

		if (ft_is_adjacent(pos, obj->pos))
			return obj;
	}
	return NULL;
}

t_obj *ft_get_adjacent_courier(t_pos pos)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_UNIT || obj->s_unit.team_id != game.my_team_id)
			continue;
		if (!obj->s_unit.name || strcmp(obj->s_unit.name, ROLE_COURIER) != 0)
			continue;

		if (ft_is_adjacent(pos, obj->pos))
			return obj;
	}
	return NULL;
}

t_obj *ft_get_adjacent_miner_with_gems(t_pos pos)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_UNIT || obj->s_unit.team_id != game.my_team_id)
			continue;
		if (!obj->s_unit.name || strcmp(obj->s_unit.name, ROLE_MINER) != 0)
			continue;
		if (obj->s_unit.gems == 0)
			continue;

		if (ft_is_adjacent(pos, obj->pos))
			return obj;
	}
	return NULL;
}

t_obj *ft_get_miner_with_gems_nearest(t_pos pos)
{
	t_obj *best = NULL;
	unsigned int best_dist = UINT_MAX;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_UNIT || obj->s_unit.team_id != game.my_team_id)
			continue;
		if (!obj->s_unit.name || strcmp(obj->s_unit.name, ROLE_MINER) != 0)
			continue;
		if (obj->s_unit.gems == 0)
			continue;

		unsigned int dist = ft_distance(pos, obj->pos);
		if (dist < best_dist)
		{
			best_dist = dist;
			best = obj;
		}
	}
	return best;
}

t_obj *ft_get_any_miner_nearest(t_pos pos)
{
	t_obj *best = NULL;
	unsigned int best_dist = UINT_MAX;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_UNIT || obj->s_unit.team_id != game.my_team_id)
			continue;
		if (!obj->s_unit.name || strcmp(obj->s_unit.name, ROLE_MINER) != 0)
			continue;

		unsigned int dist = ft_distance(pos, obj->pos);
		if (dist < best_dist)
		{
			best_dist = dist;
			best = obj;
		}
	}
	return best;
}

t_obj *ft_get_threat_nearest(t_pos pos, unsigned int max_dist)
{
	t_obj *best = NULL;
	unsigned int best_dist = UINT_MAX;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_UNIT || obj->s_unit.team_id == game.my_team_id)
			continue;

		unsigned int dist = ft_distance(pos, obj->pos);
		if (dist <= max_dist && dist < best_dist)
		{
			best_dist = dist;
			best = obj;
		}
	}
	return best;
}

bool ft_is_core_threatened(unsigned int radius)
{
	t_obj *core = ft_get_core_own();
	if (!core)
		return false;
	return (ft_get_threat_nearest(core->pos, radius) != NULL);
}

unsigned int count_owned_units_by_role(const char *role)
{
	unsigned int count = 0;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_UNIT || obj->s_unit.team_id != game.my_team_id)
			continue;
		if (obj->s_unit.name && strcmp(obj->s_unit.name, role) == 0)
			count++;
	}
	return count;
}
