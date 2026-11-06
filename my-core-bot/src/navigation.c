#include "bot.h"

extern void __real_core_action_move(const t_obj *unit, t_pos pos);
extern void __real_core_action_attack(const t_obj *attacker, const t_obj *target);
extern void __real_core_action_transferGems(const t_obj *source, t_pos target_pos, unsigned long amount);

static bool g_moved_to_tile[MAX_GRID_SIZE][MAX_GRID_SIZE];
static unsigned long g_acted_unit_ids[MAX_TRACKED_TARGETS];
static unsigned int g_acted_count = 0;

static unsigned long g_dmg_target_ids[MAX_TRACKED_TARGETS];
static unsigned long g_dmg_amounts[MAX_TRACKED_TARGETS];
static unsigned int g_dmg_count = 0;

static unsigned long g_pending_core_gems = 0;

bool is_pos_in_bounds(t_pos pos)
{
	return (pos.x < game.grid_size && pos.y < game.grid_size);
}

void reservations_init_tick(void)
{
	memset(g_moved_to_tile, 0, sizeof(g_moved_to_tile));
	g_acted_count = 0;
	g_dmg_count = 0;
	g_pending_core_gems = 0;
}

static bool has_unit_acted(unsigned long id)
{
	for (unsigned int i = 0; i < g_acted_count; i++)
	{
		if (g_acted_unit_ids[i] == id)
			return true;
	}
	return false;
}

static void mark_unit_acted(unsigned long id)
{
	if (g_acted_count < MAX_TRACKED_TARGETS)
		g_acted_unit_ids[g_acted_count++] = id;
}

void record_core_gem_transfer(unsigned long amount)
{
	g_pending_core_gems += amount;
}

unsigned long get_pending_core_gems(void)
{
	return g_pending_core_gems;
}

unsigned long get_pending_damage(unsigned long target_id)
{
	for (unsigned int i = 0; i < g_dmg_count; i++)
	{
		if (g_dmg_target_ids[i] == target_id)
			return g_dmg_amounts[i];
	}
	return 0;
}

void add_pending_damage(unsigned long target_id, unsigned long dmg)
{
	for (unsigned int i = 0; i < g_dmg_count; i++)
	{
		if (g_dmg_target_ids[i] == target_id)
		{
			g_dmg_amounts[i] += dmg;
			return;
		}
	}
	if (g_dmg_count < MAX_TRACKED_TARGETS)
	{
		g_dmg_target_ids[g_dmg_count] = target_id;
		g_dmg_amounts[g_dmg_count] = dmg;
		g_dmg_count++;
	}
}

void __wrap_core_action_move(const t_obj *unit, t_pos pos)
{
	if (!unit || unit->s_unit.action_cooldown > 0 || has_unit_acted(unit->id))
		return;
	if (!is_pos_in_bounds(pos) || !ft_is_adjacent(unit->pos, pos))
		return;
	if (g_moved_to_tile[pos.x][pos.y])
		return;

	t_obj *obj = core_get_obj_from_pos(pos);
	if (obj)
		return;

	g_moved_to_tile[pos.x][pos.y] = true;
	mark_unit_acted(unit->id);
	__real_core_action_move(unit, pos);
}

void __wrap_core_action_attack(const t_obj *attacker, const t_obj *target)
{
	if (!attacker || !target || attacker->s_unit.action_cooldown > 0 || has_unit_acted(attacker->id))
		return;

	t_obj *t = core_get_obj_from_id(target->id);
	if (!t || t->hp == 0 || !ft_is_adjacent(attacker->pos, t->pos))
		return;

	// Track damage dealt to enemy units so allies don't overkill dead targets
	if (t->type == OBJ_UNIT && t->s_unit.team_id != game.my_team_id)
	{
		unsigned long raw_dmg = attacker->s_unit.properties.damage_unit;
		int red = t->s_unit.properties.damage_reduction_percent;
		unsigned long eff_dmg = (raw_dmg * (unsigned long)(100 - red)) / 100;
		if (eff_dmg == 0 && raw_dmg > 0)
			eff_dmg = 1;
		add_pending_damage(t->id, eff_dmg);
	}

	mark_unit_acted(attacker->id);
	__real_core_action_attack(attacker, t);
}

void __wrap_core_action_transferGems(const t_obj *source, t_pos target_pos, unsigned long amount)
{
	if (!source || source->s_unit.action_cooldown > 0 || amount == 0 || has_unit_acted(source->id))
		return;
	if (amount > source->s_unit.gems)
		amount = source->s_unit.gems;

	t_obj *dest = core_get_obj_from_pos(target_pos);
	if (dest && dest->type == OBJ_UNIT)
	{
		unsigned long max_g = dest->s_unit.properties.max_gems;
		unsigned long curr_g = dest->s_unit.gems;
		if (curr_g >= max_g)
			return;
		unsigned long space = max_g - curr_g;
		if (amount > space)
			amount = space;
	}
	else if (dest && dest->type == OBJ_CORE && dest->s_core.team_id == game.my_team_id)
	{
		record_core_gem_transfer(amount);
	}

	mark_unit_acted(source->id);
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
		return res;
	}

	if (obj->type == OBJ_GEM_PILE)
	{
		// Gem piles are objects on tiles; attack them to scoop or route through
		res.weight = 2;
		res.action = CORE_TRAVEL_ATTACK;
		return res;
	}

	if (obj->type == OBJ_WALL || obj->type == OBJ_DEPOSIT)
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
		else if (unit && unit->s_unit.properties.damage_unit > 0)
		{
			res.weight = (obj->hp + unit->s_unit.properties.damage_unit - 1) / unit->s_unit.properties.damage_unit;
			res.action = CORE_TRAVEL_ATTACK;
		}
		else
		{
			res.weight = 10000;
			res.action = CORE_TRAVEL_BLOCK;
		}
		return res;
	}

	if (obj->type == OBJ_CORE)
	{
		if (obj->s_core.team_id != game.my_team_id && unit && unit->s_unit.properties.damage_core > 0)
		{
			res.weight = 1;
			res.action = CORE_TRAVEL_ATTACK;
		}
		else
		{
			res.weight = 10000;
			res.action = CORE_TRAVEL_BLOCK;
		}
		return res;
	}

	return res;
}

void safe_travel(const t_obj *u, t_pos target)
{
	if (!u || u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
		return;
	core_action_travel(u, target, safe_travelability);

	// Fallback sidestep if primary travel path was blocked by a friendly unit
	if (!has_unit_acted(u->id) && ft_distance(u->pos, target) > 1)
	{
		const int dx[4] = {1, -1, 0, 0};
		const int dy[4] = {0, 0, 1, -1};
		unsigned int best_d = ft_distance(u->pos, target);
		t_pos best_step = u->pos;
		bool found = false;

		for (int d = 0; d < 4; d++)
		{
			int nx = (int)u->pos.x + dx[d];
			int ny = (int)u->pos.y + dy[d];
			if (nx < 0 || ny < 0 || nx >= (int)game.grid_size || ny >= (int)game.grid_size)
				continue;
			t_pos np = {(unsigned short)nx, (unsigned short)ny};
			if (g_moved_to_tile[np.x][np.y] || core_get_obj_from_pos(np) != NULL)
				continue;
			unsigned int dist = ft_distance(np, target);
			if (dist <= best_d)
			{
				best_d = dist;
				best_step = np;
				found = true;
			}
		}
		if (found)
			core_action_move(u, best_step);
	}
}
