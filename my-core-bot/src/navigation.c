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

static unsigned long g_unit_received_ids[MAX_TRACKED_TARGETS];
static unsigned long g_unit_received_gems[MAX_TRACKED_TARGETS];
static unsigned int g_unit_received_count = 0;

static unsigned long g_pile_harvest_ids[MAX_TRACKED_TARGETS];
static unsigned long g_pile_harvest_gems[MAX_TRACKED_TARGETS];
static unsigned int g_pile_harvest_count = 0;

static unsigned long g_pending_core_gems = 0;
static bool g_yield_request[MAX_GRID_SIZE][MAX_GRID_SIZE];

bool is_pos_in_bounds(t_pos pos)
{
	return (pos.x < game.grid_size && pos.y < game.grid_size);
}

void nav_request_yield(t_pos pos)
{
	if (is_pos_in_bounds(pos))
		g_yield_request[pos.x][pos.y] = true;
}

void reservations_init_tick(void)
{
	memset(g_moved_to_tile, 0, sizeof(g_moved_to_tile));
	memset(g_yield_request, 0, sizeof(g_yield_request));
	g_acted_count = 0;
	g_dmg_count = 0;
	g_unit_received_count = 0;
	g_pile_harvest_count = 0;
	g_pending_core_gems = 0;
}

bool has_unit_acted(unsigned long id)
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

unsigned long get_pending_pile_gems(unsigned long pile_id)
{
	for (unsigned int i = 0; i < g_pile_harvest_count; i++)
	{
		if (g_pile_harvest_ids[i] == pile_id)
			return g_pile_harvest_gems[i];
	}
	return 0;
}

void add_pending_pile_gems(unsigned long pile_id, unsigned long gems)
{
	for (unsigned int i = 0; i < g_pile_harvest_count; i++)
	{
		if (g_pile_harvest_ids[i] == pile_id)
		{
			g_pile_harvest_gems[i] += gems;
			return;
		}
	}
	if (g_pile_harvest_count < MAX_TRACKED_TARGETS)
	{
		g_pile_harvest_ids[g_pile_harvest_count] = pile_id;
		g_pile_harvest_gems[g_pile_harvest_count] = gems;
		g_pile_harvest_count++;
	}
}

unsigned long get_pending_unit_gems(unsigned long id)
{
	for (unsigned int i = 0; i < g_unit_received_count; i++)
	{
		if (g_unit_received_ids[i] == id)
			return g_unit_received_gems[i];
	}
	return 0;
}

void add_pending_unit_gems(unsigned long id, unsigned long amount)
{
	for (unsigned int i = 0; i < g_unit_received_count; i++)
	{
		if (g_unit_received_ids[i] == id)
		{
			g_unit_received_gems[i] += amount;
			return;
		}
	}
	if (g_unit_received_count < MAX_TRACKED_TARGETS)
	{
		g_unit_received_ids[g_unit_received_count] = id;
		g_unit_received_gems[g_unit_received_count] = amount;
		g_unit_received_count++;
	}
}

static bool is_tile_occupied(t_pos pos)
{
	if (!is_pos_in_bounds(pos))
		return true;
	if (g_moved_to_tile[pos.x][pos.y])
		return true;
	t_obj *occ = core_get_obj_from_pos(pos);
	if (occ != NULL && (occ->hp > 0 || occ->type == OBJ_GEM_PILE))
		return true;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->pos.x == pos.x && o->pos.y == pos.y && (o->hp > 0 || o->type == OBJ_GEM_PILE))
			return true;
	}
	return false;
}

bool has_free_spawn_tile(t_pos core_pos)
{
	const int dx[4] = {1, -1, 0, 0};
	const int dy[4] = {0, 0, 1, -1};

	for (int i = 0; i < 4; i++)
	{
		int nx = (int)core_pos.x + dx[i];
		int ny = (int)core_pos.y + dy[i];
		if (nx < 0 || ny < 0 || nx >= (int)game.grid_size || ny >= (int)game.grid_size)
			continue;
		t_pos p = {(unsigned short)nx, (unsigned short)ny};
		if (!is_tile_occupied(p))
			return true;
	}
	return false;
}

void step_away_from_core(const t_obj *u, t_obj *core)
{
	if (!u || !core || u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
		return;
	if (!ft_is_adjacent(u->pos, core->pos))
		return;

	const int dx[4] = {1, -1, 0, 0};
	const int dy[4] = {0, 0, 1, -1};
	unsigned int max_dist = ft_distance(u->pos, core->pos);
	t_pos best_step = u->pos;
	bool found = false;

	for (int d = 0; d < 4; d++)
	{
		int nx = (int)u->pos.x + dx[d];
		int ny = (int)u->pos.y + dy[d];
		if (nx < 0 || ny < 0 || nx >= (int)game.grid_size || ny >= (int)game.grid_size)
			continue;
		t_pos np = {(unsigned short)nx, (unsigned short)ny};
		if (is_tile_occupied(np))
			continue;
		unsigned int dist = ft_distance(np, core->pos);
		if (dist > max_dist)
		{
			max_dist = dist;
			best_step = np;
			found = true;
		}
	}
	if (found)
		core_action_move(u, best_step);
}

void __wrap_core_action_move(const t_obj *unit, t_pos pos)
{
	if (!unit || unit->s_unit.action_cooldown > 0 || has_unit_acted(unit->id))
		return;
	if (!is_pos_in_bounds(pos) || !ft_is_adjacent(unit->pos, pos))
		return;
	if (is_tile_occupied(pos))
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
	if (!t || !ft_is_adjacent(attacker->pos, t->pos))
		return;

	// Verify target object is actively present in game.objects
	bool exists_in_game = false;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		if (game.objects[i] == t && game.objects[i]->id == target->id)
		{
			exists_in_game = true;
			break;
		}
	}
	if (!exists_in_game)
		return;

	if (t->type != OBJ_GEM_PILE && t->hp == 0)
		return;
	if (t->type == OBJ_GEM_PILE)
	{
		if (t->s_deposit_gems_pile.gems == 0)
			return;
		if (attacker->s_unit.gems >= attacker->s_unit.properties.max_gems)
			return;
		if (t->s_deposit_gems_pile.gems <= get_pending_pile_gems(t->id))
			return;
		// Deconflict: if any enemy unit is adjacent, opponent could harvest it first in server tick order
		if (ft_is_enemy_adjacent_to_pos(t->pos))
			return;
	}

	// Reject zero-damage attacks
	if (t->type == OBJ_CORE && attacker->s_unit.properties.damage_core == 0)
		return;
	if (t->type == OBJ_UNIT && attacker->s_unit.properties.damage_unit == 0)
		return;
	if ((t->type == OBJ_WALL || t->type == OBJ_DEPOSIT) &&
		attacker->s_unit.properties.damage_object == 0)
		return;

	// Universal overkill protection (entities with HP only)
	if (t->type != OBJ_GEM_PILE && t->hp <= get_pending_damage(t->id))
		return;

	unsigned long raw_dmg = 0;
	if (t->type == OBJ_UNIT && t->s_unit.team_id != game.my_team_id)
	{
		raw_dmg = attacker->s_unit.properties.damage_unit;
		int red = t->s_unit.properties.damage_reduction_percent;
		unsigned long eff_dmg = (raw_dmg * (unsigned long)(100 - red)) / 100;
		if (eff_dmg == 0 && raw_dmg > 0)
			eff_dmg = 1;
		add_pending_damage(t->id, eff_dmg);
	}
	else if (t->type == OBJ_DEPOSIT || t->type == OBJ_WALL)
	{
		raw_dmg = attacker->s_unit.properties.damage_object;
		if (raw_dmg > 0)
			add_pending_damage(t->id, raw_dmg);
	}
	else if (t->type == OBJ_CORE && t->s_core.team_id != game.my_team_id)
	{
		raw_dmg = attacker->s_unit.properties.damage_core;
		if (raw_dmg > 0)
			add_pending_damage(t->id, raw_dmg);
	}
	else if (t->type == OBJ_GEM_PILE)
	{
		unsigned long space = attacker->s_unit.properties.max_gems - attacker->s_unit.gems;
		unsigned long gain = (t->s_deposit_gems_pile.gems < space) ? t->s_deposit_gems_pile.gems : space;
		add_pending_pile_gems(t->id, gain);
	}

	mark_unit_acted(attacker->id);
	__real_core_action_attack(attacker, t);
}

void __wrap_core_action_transferGems(const t_obj *source, t_pos target_pos, unsigned long amount)
{
	if (!source || source->s_unit.action_cooldown > 0 || amount == 0 || has_unit_acted(source->id))
		return;
	if (source->s_unit.gems == 0)
		return;
	if (!is_pos_in_bounds(target_pos) || !ft_is_adjacent(source->pos, target_pos))
		return;
	if (amount > source->s_unit.gems)
		amount = source->s_unit.gems;
	if (amount == 0)
		return;

	t_obj *dest = core_get_obj_from_pos(target_pos);
	if (dest && dest->type == OBJ_UNIT && dest->s_unit.team_id == game.my_team_id)
	{
		unsigned long max_g = dest->s_unit.properties.max_gems;
		unsigned long curr_g = dest->s_unit.gems;
		unsigned long pending = get_pending_unit_gems(dest->id);
		if (curr_g + pending >= max_g)
			return;
		unsigned long space = max_g - (curr_g + pending);
		if (amount > space)
			amount = space;
		if (amount == 0)
			return;
		add_pending_unit_gems(dest->id, amount);
	}
	else if (dest && dest->type == OBJ_CORE && dest->s_core.team_id == game.my_team_id)
	{
		record_core_gem_transfer(amount);
	}
	else
	{
		return;
	}

	mark_unit_acted(source->id);
	__real_core_action_transferGems(source, target_pos, amount);
}

static unsigned int get_tile_enemy_danger(t_pos pos)
{
	unsigned int danger = 0;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id &&
			o->s_unit.properties.damage_unit > 0 && o->hp > 0)
		{
			unsigned int d = ft_distance(pos, o->pos);
			if (d <= 1)
				danger += 500;
			else if (d == 2)
				danger += 20;
		}
	}
	return danger;
}

t_tile_travelability safe_travelability(t_pos pos, const t_obj *unit)
{
	t_tile_travelability res;
	res.weight = 1;
	res.action = CORE_TRAVEL_PASS;

	t_obj *obj = core_get_obj_from_pos(pos);
	if (!obj)
	{
		if (is_tile_occupied(pos))
		{
			res.weight = 10000;
			res.action = CORE_TRAVEL_BLOCK;
			return res;
		}
		// Unarmed units (Carriers, Miners, Speed Siegers) route around enemy melee range!
		if (unit && unit->s_unit.properties.damage_unit == 0)
		{
			res.weight += get_tile_enemy_danger(pos);
		}
		return res;
	}

	if (obj->type == OBJ_GEM_PILE)
	{
		if (obj->s_deposit_gems_pile.gems == 0 ||
			obj->s_deposit_gems_pile.gems <= get_pending_pile_gems(obj->id) ||
			ft_is_enemy_adjacent_to_pos(obj->pos))
		{
			res.weight = 10000;
			res.action = CORE_TRAVEL_BLOCK;
			return res;
		}

		unsigned long room = (unit && unit->s_unit.properties.max_gems > unit->s_unit.gems)
			? (unit->s_unit.properties.max_gems - unit->s_unit.gems) : 0;
		if (room > 0 && unit && unit->s_unit.properties.gems_per_cooldown_step > 0)
		{
			res.weight = 2;
			res.action = CORE_TRAVEL_ATTACK;
		}
		else
		{
			res.weight = 10000;
			res.action = CORE_TRAVEL_BLOCK;
		}
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

	// If travel did not execute an action (e.g. bottleneck blocked by a friendly unit),
	// check adjacent friendly units towards target and request yield, or sidestep!
	if (!has_unit_acted(u->id) && ft_distance(u->pos, target) >= 1)
	{
		const int dx[4] = {1, -1, 0, 0};
		const int dy[4] = {0, 0, 1, -1};
		unsigned int best_d = ft_distance(u->pos, target);
		t_pos best_step = u->pos;
		bool found_free = false;

		for (int d = 0; d < 4; d++)
		{
			int nx = (int)u->pos.x + dx[d];
			int ny = (int)u->pos.y + dy[d];
			if (nx < 0 || ny < 0 || nx >= (int)game.grid_size || ny >= (int)game.grid_size)
				continue;
			t_pos np = {(unsigned short)nx, (unsigned short)ny};
			t_obj *occ = core_get_obj_from_pos(np);
			if (occ && occ->type == OBJ_UNIT && occ->s_unit.team_id == game.my_team_id)
			{
				unsigned int dist = ft_distance(np, target);
				if (dist < best_d)
					nav_request_yield(np);
			}
			else if (!is_tile_occupied(np))
			{
				unsigned int dist = ft_distance(np, target);
				if (dist <= best_d)
				{
					best_d = dist;
					best_step = np;
					found_free = true;
				}
			}
		}
		if (found_free && ft_distance(u->pos, target) > 1)
			core_action_move(u, best_step);
	}
}

#define INF_DIST (UINT_MAX / 4)

void nav_compute_corridor_distances(t_pos start, unsigned int dist[MAX_GRID_SIZE][MAX_GRID_SIZE],
									bool is_unarmed, unsigned long ignore_id)
{
	for (unsigned int y = 0; y < MAX_GRID_SIZE; y++)
		for (unsigned int x = 0; x < MAX_GRID_SIZE; x++)
			dist[y][x] = INF_DIST;

	if (start.x >= game.grid_size || start.y >= game.grid_size)
		return;

	bool blocked[MAX_GRID_SIZE][MAX_GRID_SIZE] = {{false}};

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *o = game.objects[i];
		if (o->pos.x >= game.grid_size || o->pos.y >= game.grid_size)
			continue;
		if (o->type == OBJ_WALL || o->type == OBJ_DEPOSIT || o->type == OBJ_CORE ||
			o->type == OBJ_GEM_PILE)
		{
			blocked[o->pos.y][o->pos.x] = true;
		}
		else if (o->type == OBJ_UNIT && o->id != ignore_id)
		{
			if (o->s_unit.team_id != game.my_team_id)
				blocked[o->pos.y][o->pos.x] = true;
		}

		// Avoid enemy melee zones for unarmed workers
		if (is_unarmed && o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id &&
			o->s_unit.properties.damage_unit > 0)
		{
			for (int dy = -2; dy <= 2; dy++)
				for (int dx = -2; dx <= 2; dx++)
				{
					if (abs(dx) + abs(dy) <= 2)
					{
						int ex = (int)o->pos.x + dx;
						int ey = (int)o->pos.y + dy;
						if (ex >= 0 && ey >= 0 && ex < (int)game.grid_size && ey < (int)game.grid_size)
							blocked[ey][ex] = true;
					}
				}
		}
	}

	t_pos queue[MAX_GRID_SIZE * MAX_GRID_SIZE];
	unsigned int head = 0, tail = 0;

	dist[start.y][start.x] = 0;
	queue[tail++] = start;

	const int dx[4] = {1, -1, 0, 0};
	const int dy[4] = {0, 0, 1, -1};

	while (head < tail)
	{
		t_pos p = queue[head++];
		for (int d = 0; d < 4; d++)
		{
			int nx = (int)p.x + dx[d];
			int ny = (int)p.y + dy[d];
			if (nx < 0 || ny < 0 || nx >= (int)game.grid_size || ny >= (int)game.grid_size)
				continue;
			if (blocked[ny][nx] || dist[ny][nx] < INF_DIST)
				continue;
			dist[ny][nx] = dist[p.y][p.x] + 1;
			queue[tail++] = (t_pos){(unsigned short)nx, (unsigned short)ny};
		}
	}
}

unsigned int nav_approach_distance(t_pos target, const unsigned int dist[MAX_GRID_SIZE][MAX_GRID_SIZE])
{
	unsigned int best = INF_DIST;
	const int dx[4] = {1, -1, 0, 0};
	const int dy[4] = {0, 0, 1, -1};

	for (int d = 0; d < 4; d++)
	{
		int nx = (int)target.x + dx[d];
		int ny = (int)target.y + dy[d];
		if (nx >= 0 && ny >= 0 && nx < (int)game.grid_size && ny < (int)game.grid_size &&
			nx < MAX_GRID_SIZE && ny < MAX_GRID_SIZE)
		{
			if (dist[ny][nx] < best)
				best = dist[ny][nx];
		}
	}
	return best;
}

unsigned long ft_runner_opportunity(const t_obj *core)
{
	if (!core || game.grid_size > MAX_GRID_SIZE)
		return 0;

	unsigned long total_safe_gems = 0;
	const int dx[4] = {1, -1, 0, 0};
	const int dy[4] = {0, 0, 1, -1};

	for (int d = 0; d < 4; d++)
	{
		int sx = (int)core->pos.x + dx[d];
		int sy = (int)core->pos.y + dy[d];
		if (sx < 0 || sy < 0 || sx >= (int)game.grid_size || sy >= (int)game.grid_size)
			continue;
		t_pos spawn_pos = {(unsigned short)sx, (unsigned short)sy};
		t_obj *occ = core_get_obj_from_pos(spawn_pos);
		if (occ && (occ->type == OBJ_WALL || occ->type == OBJ_DEPOSIT || occ->type == OBJ_CORE))
			continue;

		unsigned int dist[MAX_GRID_SIZE][MAX_GRID_SIZE];
		nav_compute_corridor_distances(spawn_pos, dist, true, 0);

		unsigned long exit_gems = 0;
		for (int i = 0; game.objects && game.objects[i]; i++)
		{
			t_obj *o = game.objects[i];
			if (o->type != OBJ_GEM_PILE || o->s_deposit_gems_pile.gems == 0)
				continue;
			if (ft_is_pos_near_armed_enemy(o->pos, 3))
				continue;
			unsigned int app = nav_approach_distance(o->pos, dist);
			if (app <= 24)
				exit_gems += o->s_deposit_gems_pile.gems;
		}
		if (exit_gems > total_safe_gems)
			total_safe_gems = exit_gems;
	}
	return total_safe_gems;
}

void nav_yield_for_workers(t_obj *core, t_obj *enemy_core, t_obj **units)
{
	if (!core || !units)
		return;

	bool has_workers = false;
	for (int i = 0; units && units[i]; i++)
	{
		if (units[i]->s_unit.name && (strcmp(units[i]->s_unit.name, "Carrier") == 0 ||
			strcmp(units[i]->s_unit.name, "Miner") == 0))
		{
			has_workers = true;
			break;
		}
	}

	const int dx[4] = {1, -1, 0, 0};
	const int dy[4] = {0, 0, 1, -1};

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
			continue;
		if (strcmp(u->s_unit.name, "Hunter") != 0 && strcmp(u->s_unit.name, "Striker") != 0 &&
			strcmp(u->s_unit.name, "Defender") != 0 && strcmp(u->s_unit.name, "Scout") != 0)
			continue;

		bool requested = (u->pos.x < MAX_GRID_SIZE && u->pos.y < MAX_GRID_SIZE &&
			g_yield_request[u->pos.x][u->pos.y]);
		bool blocking_spawn = (has_workers && ft_distance(u->pos, core->pos) == 1);

		if (!requested && !blocking_spawn)
			continue;

		// Fight adjacent enemies instead of giving up an active interception!
		if (ft_get_best_adjacent_enemy(u) != NULL)
			continue;

		unsigned int best_score = UINT_MAX;
		t_pos destination = u->pos;
		bool found = false;

		for (int d = 0; d < 4; d++)
		{
			int nx = (int)u->pos.x + dx[d];
			int ny = (int)u->pos.y + dy[d];
			if (nx < 0 || ny < 0 || nx >= (int)game.grid_size || ny >= (int)game.grid_size)
				continue;
			t_pos p = {(unsigned short)nx, (unsigned short)ny};
			if (is_tile_occupied(p))
				continue;

			unsigned int score = (enemy_core ? ft_distance(p, enemy_core->pos) : 0);
			if (ft_distance(p, core->pos) == 1)
				score += 500;
			if (p.x < MAX_GRID_SIZE && p.y < MAX_GRID_SIZE && g_yield_request[p.x][p.y])
				score += 1000;

			if (score < best_score)
			{
				best_score = score;
				destination = p;
				found = true;
			}
		}

		if (found)
		{
			core_action_move(u, destination);
			core_debug_addObjectInfo(u, "Yielded corridor to (%u,%u)", destination.x, destination.y);
		}
	}
}
