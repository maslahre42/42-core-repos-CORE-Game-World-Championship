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

t_obj *ft_get_nearest_gem_pile(t_pos pos)
{
	t_obj *best = NULL;
	unsigned int min_dist = UINT_MAX;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type == OBJ_GEM_PILE && obj->s_deposit_gems_pile.gems > 0)
		{
			unsigned int d = ft_distance(pos, obj->pos);
			if (d < min_dist)
			{
				min_dist = d;
				best = obj;
			}
		}
	}
	return best;
}

t_obj *ft_get_deposit_for_miner(const t_obj *miner, const t_obj *core)
{
	if (!miner || !core)
		return NULL;

	// Calculate this miner's index among all friendly miners
	unsigned int miner_idx = 0;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type == OBJ_UNIT && obj->s_unit.team_id == game.my_team_id)
		{
			if (obj->s_unit.name && strcmp(obj->s_unit.name, UNIT_NAME_MINER) == 0)
			{
				if (obj->id < miner->id)
					miner_idx++;
			}
		}
	}

	// Collect valid deposits
	t_obj *deposits[128];
	int count = 0;
	for (int i = 0; game.objects && game.objects[i] && count < 128; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type == OBJ_DEPOSIT && obj->hp > 0)
		{
			deposits[count++] = obj;
		}
	}

	if (count == 0)
	{
		// Fallback to nearest gem pile if all deposits are depleted
		return ft_get_nearest_gem_pile(miner->pos);
	}

	// Sort deposits by distance to our Core (safest nearest deposits first)
	for (int i = 0; i < count - 1; i++)
	{
		for (int j = i + 1; j < count; j++)
		{
			unsigned int d1 = ft_distance(core->pos, deposits[i]->pos);
			unsigned int d2 = ft_distance(core->pos, deposits[j]->pos);
			if (d2 < d1)
			{
				t_obj *tmp = deposits[i];
				deposits[i] = deposits[j];
				deposits[j] = tmp;
			}
		}
	}

	return deposits[miner_idx % count];
}

t_obj *ft_get_adjacent_carrier(t_pos pos)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type == OBJ_UNIT && obj->s_unit.team_id == game.my_team_id)
		{
			if (obj->s_unit.name && strcmp(obj->s_unit.name, UNIT_NAME_CARRIER) == 0)
			{
				if (ft_is_adjacent(pos, obj->pos))
					return obj;
			}
		}
	}
	return NULL;
}

t_obj *ft_get_miner_needing_carrier(t_pos pos)
{
	t_obj *best = NULL;
	long best_score = LONG_MIN;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type == OBJ_UNIT && obj->s_unit.team_id == game.my_team_id)
		{
			if (obj->s_unit.name && strcmp(obj->s_unit.name, UNIT_NAME_MINER) == 0)
			{
				long carried = (long)obj->s_unit.gems;
				long dist = (long)ft_distance(pos, obj->pos);
				long score = (carried * 15) - (dist * 2);

				if (score > best_score)
				{
					best_score = score;
					best = obj;
				}
			}
		}
	}
	return best;
}

t_obj *ft_get_best_hunter_target(const t_obj *hunter, const t_obj *core)
{
	t_obj *best = NULL;
	long best_score = LONG_MIN;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type == OBJ_UNIT && obj->s_unit.team_id != game.my_team_id)
		{
			long score = 0;

			// Priority 1: Unarmed units (carriers / unarmed miners)
			if (obj->s_unit.properties.damage_unit == 0)
			{
				score += 2000;
				// Carrying gems makes it an even higher priority target
				score += (long)obj->s_unit.gems * 15;
			}

			// Priority 2: Threat to our Core
			if (core)
			{
				unsigned int dist_core = ft_distance(obj->pos, core->pos);
				if (dist_core <= 6)
					score += (long)(10 - dist_core) * 100;
			}

			// Distance penalty from Hunter
			unsigned int dist_hunter = ft_distance(hunter->pos, obj->pos);
			score -= (long)dist_hunter * 20;

			// Low HP bonus (finish off wounded enemies)
			if (obj->hp <= 6)
				score += 300;

			if (score > best_score)
			{
				best_score = score;
				best = obj;
			}
		}
	}
	return best;
}

t_obj *ft_get_adjacent_enemy(t_pos pos)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type == OBJ_UNIT && obj->s_unit.team_id != game.my_team_id)
		{
			if (ft_is_adjacent(pos, obj->pos))
				return obj;
		}
	}
	return NULL;
}

bool ft_is_core_threatened(t_obj *core, unsigned int radius)
{
	if (!core)
		return false;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type == OBJ_UNIT && obj->s_unit.team_id != game.my_team_id)
		{
			if ((obj->s_unit.properties.damage_unit > 0 || obj->s_unit.properties.damage_core > 0) &&
				ft_distance(core->pos, obj->pos) <= radius)
				return true;
		}
	}
	return false;
}

/* Stay on our side; assignment scores favor short trips. Reject resources
 * covered by enemy fighters, both when buying and when assigning a collector. */
bool ft_is_harvest_resource(const t_obj *resource, const t_obj *core)
{
	if (!resource || !core ||
		(resource->type != OBJ_DEPOSIT && resource->type != OBJ_GEM_PILE) ||
		!resource->s_deposit_gems_pile.gems)
		return false;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *enemy = game.objects[i];
		if (enemy->type == OBJ_UNIT && enemy->s_unit.team_id != game.my_team_id &&
			enemy->s_unit.properties.damage_unit && ft_distance(enemy->pos, resource->pos) <= 3)
			return false;
	}
	return true;
}

bool ft_is_safe_resource(const t_obj *resource, const t_obj *core)
{
	if (!ft_is_harvest_resource(resource, core))
		return false;
	t_obj *enemy = ft_get_core_opponent();
	return !enemy || ft_distance(core->pos, resource->pos) < ft_distance(enemy->pos, resource->pos);
}

bool ft_is_local_resource(const t_obj *resource, const t_obj *core)
{
	return ft_is_safe_resource(resource, core) &&
		ft_distance(core->pos, resource->pos) <= LOCAL_RESOURCE_RADIUS;
}
