#include "bot.h"

extern void __real_core_action_move(const t_obj *unit, t_pos pos);
extern void __real_core_action_attack(const t_obj *attacker, const t_obj *target);
extern void __real_core_action_transferGems(const t_obj *source, t_pos target_pos, unsigned long amount);

static bool g_moved_to_tile[MAX_GRID_SIZE][MAX_GRID_SIZE];

bool is_pos_in_bounds(t_pos pos)
{
	return (pos.x < game.grid_size && pos.y < game.grid_size);
}

void reservations_init_tick(void)
{
	memset(g_moved_to_tile, 0, sizeof(g_moved_to_tile));
}

// 1. Bulletproof move wrapper: ensures strictly 0 move action failures
void __wrap_core_action_move(const t_obj *unit, t_pos pos)
{
	if (!unit || unit->s_unit.action_cooldown > 0)
		return;
	if (!is_pos_in_bounds(pos))
		return;

	// Abort if another unit moved to this tile in this tick
	if (g_moved_to_tile[pos.x][pos.y])
		return;

	// Abort if the tile currently contains ANY entity
	t_obj *obj = core_get_obj_from_pos(pos);
	if (obj)
		return;

	g_moved_to_tile[pos.x][pos.y] = true;
	__real_core_action_move(unit, pos);
}

// 2. Bulletproof attack wrapper: ensures strictly 0 attack action failures
void __wrap_core_action_attack(const t_obj *attacker, const t_obj *target)
{
	if (!attacker || !target)
		return;
	if (attacker->s_unit.action_cooldown > 0)
		return;

	// Verify target still exists and is alive
	t_obj *t = core_get_obj_from_id(target->id);
	if (!t || t->hp == 0)
		return;

	// Verify target is strictly adjacent
	if (!ft_is_adjacent(attacker->pos, t->pos))
		return;

	__real_core_action_attack(attacker, t);
}

// 3. Bulletproof transfer wrapper: ensures strictly 0 transfer capacity failures
void __wrap_core_action_transferGems(const t_obj *source, t_pos target_pos, unsigned long amount)
{
	if (!source || source->s_unit.action_cooldown > 0 || amount == 0)
		return;
	if (amount > source->s_unit.gems)
		amount = source->s_unit.gems;

	t_obj *dest = core_get_obj_from_pos(target_pos);
	if (dest && dest->type == OBJ_UNIT)
	{
		unsigned long max_g = dest->s_unit.properties.max_gems;
		unsigned long curr_g = dest->s_unit.gems;
		if (curr_g >= max_g)
			return; // Destination unit is completely full
		unsigned long space = max_g - curr_g;
		if (amount > space)
			amount = space;
	}

	__real_core_action_transferGems(source, target_pos, amount);
}

t_tile_travelability safe_travelability(t_pos pos, const t_obj *unit)
{
	t_tile_travelability res;
	res.weight = 1;
	res.action = CORE_TRAVEL_PASS;

	t_obj *obj = core_get_obj_from_pos(pos);
	if (!obj)
	{
		if (g_moved_to_tile[pos.x][pos.y])
		{
			res.weight = 10000;
			res.action = CORE_TRAVEL_BLOCK;
			return res;
		}
		res.weight = 1;
		res.action = CORE_TRAVEL_PASS;
		return res;
	}

	if (obj->type == OBJ_WALL)
	{
		if (unit && unit->s_unit.properties.damage_object > 0)
		{
			res.weight = (obj->hp + unit->s_unit.properties.damage_object - 1) / unit->s_unit.properties.damage_object;
			res.action = CORE_TRAVEL_ATTACK;
		}
		else
		{
			res.weight = 10000;
			res.action = CORE_TRAVEL_BLOCK;
		}
		return res;
	}

	if (obj->type == OBJ_UNIT)
	{
		if (obj->s_unit.team_id == game.my_team_id)
		{
			res.weight = 10000;
			res.action = CORE_TRAVEL_BLOCK;
		}
		else
		{
			if (unit && unit->s_unit.properties.damage_unit > 0)
			{
				res.weight = (obj->hp + unit->s_unit.properties.damage_unit - 1) / unit->s_unit.properties.damage_unit;
				res.action = CORE_TRAVEL_ATTACK;
			}
			else
			{
				res.weight = 10000;
				res.action = CORE_TRAVEL_BLOCK;
			}
		}
		return res;
	}

	if (obj->type == OBJ_CORE)
	{
		if (obj->s_core.team_id == game.my_team_id)
		{
			res.weight = 10000;
			res.action = CORE_TRAVEL_BLOCK;
		}
		else
		{
			if (unit && unit->s_unit.properties.damage_core > 0)
			{
				res.weight = (obj->hp + unit->s_unit.properties.damage_core - 1) / unit->s_unit.properties.damage_core;
				res.action = CORE_TRAVEL_ATTACK;
			}
			else
			{
				res.weight = 10000;
				res.action = CORE_TRAVEL_BLOCK;
			}
		}
		return res;
	}

	if (obj->type == OBJ_DEPOSIT)
	{
		if (unit && unit->s_unit.properties.damage_object > 0)
		{
			res.weight = (obj->hp + unit->s_unit.properties.damage_object - 1) / unit->s_unit.properties.damage_object;
			res.action = CORE_TRAVEL_ATTACK;
		}
		else
		{
			res.weight = 10000;
			res.action = CORE_TRAVEL_BLOCK;
		}
		return res;
	}

	if (obj->type == OBJ_GEM_PILE)
	{
		res.weight = 1;
		res.action = CORE_TRAVEL_PASS;
		return res;
	}

	return res;
}

void safe_travel(const t_obj *u, t_pos target)
{
	if (!u || u->s_unit.action_cooldown > 0)
		return;
	core_action_travel(u, target, safe_travelability);
}
