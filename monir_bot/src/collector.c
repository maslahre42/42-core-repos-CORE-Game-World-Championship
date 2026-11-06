#include "bot.h"

#define RUNNER_GRID 64
#define RUNNER_INF (UINT_MAX / 4)
static unsigned long g_runner_id, g_pile_id;
static bool g_returning;
static unsigned int g_blocked;

static bool safe_pile(const t_obj *o)
{
	if (!o || o->type != OBJ_GEM_PILE || !o->s_deposit_gems_pile.gems)
		return false;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *enemy = game.objects[i];
		if (enemy->type == OBJ_UNIT && enemy->s_unit.team_id != game.my_team_id &&
			enemy->s_unit.properties.damage_unit && ft_distance(enemy->pos, o->pos) <= 3)
			return false;
	}
	return true;
}

/* Measure actual corridors: an unarmed runner cannot clear walls/deposits. */
static void distances(t_pos start, unsigned int dist[RUNNER_GRID][RUNNER_GRID],
	bool honor_friendly, unsigned long ignore_id)
{
	bool blocked[RUNNER_GRID][RUNNER_GRID] = {{false}};
	t_pos queue[RUNNER_GRID * RUNNER_GRID];
	memset(dist, 0xff, sizeof(unsigned int) * RUNNER_GRID * RUNNER_GRID);
	if (game.grid_size > RUNNER_GRID || start.x >= game.grid_size || start.y >= game.grid_size)
		return;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *o = game.objects[i];
		if (o->pos.x >= game.grid_size || o->pos.y >= game.grid_size)
			continue;
		if (o->type == OBJ_WALL || o->type == OBJ_DEPOSIT || o->type == OBJ_CORE ||
			o->type == OBJ_GEM_PILE ||
			(o->type == OBJ_UNIT && o->id != ignore_id &&
			 (o->s_unit.team_id != game.my_team_id || honor_friendly)))
			blocked[o->pos.y][o->pos.x] = true;
		if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id && o->s_unit.properties.damage_unit)
			for (int dy = -2; dy <= 2; dy++)
				for (int dx = -2; dx <= 2; dx++)
				{
					int x = o->pos.x + dx, y = o->pos.y + dy;
					if (abs(dx) + abs(dy) <= 2 && x >= 0 && y >= 0 && x < game.grid_size && y < game.grid_size)
						blocked[y][x] = true;
				}
	}
	if (honor_friendly)
		for (unsigned short y = 0; y < game.grid_size; y++)
			for (unsigned short x = 0; x < game.grid_size; x++)
				if (nav_is_reserved((t_pos){x, y}))
					blocked[y][x] = true;
	const int dx[4] = {1, 0, -1, 0}, dy[4] = {0, 1, 0, -1};
	unsigned int head = 0, tail = 0;
	dist[start.y][start.x] = 0;
	queue[tail++] = start;
	while (head < tail)
	{
		t_pos p = queue[head++];
		for (int d = 0; d < 4; d++)
		{
			int x = p.x + dx[d], y = p.y + dy[d];
			if (x < 0 || y < 0 || x >= game.grid_size || y >= game.grid_size || blocked[y][x] || dist[y][x] < RUNNER_INF)
				continue;
			dist[y][x] = dist[p.y][p.x] + 1;
			queue[tail++] = (t_pos){(unsigned short)x, (unsigned short)y};
		}
	}
}

static unsigned int approach(t_pos p, unsigned int dist[RUNNER_GRID][RUNNER_GRID])
{
	unsigned int best = RUNNER_INF;
	const int dx[4] = {1, 0, -1, 0}, dy[4] = {0, 1, 0, -1};
	for (int d = 0; d < 4; d++)
	{
		int x = p.x + dx[d], y = p.y + dy[d];
		if (x >= 0 && y >= 0 && x < game.grid_size && y < game.grid_size &&
			x < RUNNER_GRID && y < RUNNER_GRID && dist[y][x] < best)
			best = dist[y][x];
	}
	return best;
}

unsigned long ft_runner_opportunity(const t_obj *core)
{
	if (!core || game.grid_size > RUNNER_GRID)
		return 0;
	/* Check every possible free spawn exit, not a fictitious route that
	 * crosses the Core between disconnected corridors. */
	unsigned long guaranteed = ULONG_MAX;
	const int dx[4] = {1, 0, -1, 0}, dy[4] = {0, 1, 0, -1};
	for (int d = 0; d < 4; d++)
	{
		int x = core->pos.x + dx[d], y = core->pos.y + dy[d];
		if (x < 0 || y < 0 || x >= game.grid_size || y >= game.grid_size)
			continue;
		t_pos spawn = {(unsigned short)x, (unsigned short)y};
		const t_obj *occupant = core_get_obj_from_pos(spawn);
		bool vacating = occupant && occupant->type == OBJ_UNIT &&
			occupant->s_unit.team_id == game.my_team_id && nav_unit_yielded(occupant);
		if ((occupant && !vacating) || nav_is_reserved(spawn))
			continue;
		unsigned int home[RUNNER_GRID][RUNNER_GRID];
		distances(spawn, home, true, 0);
		unsigned long total = 0;
		for (int i = 0; game.objects && game.objects[i]; i++)
			if (safe_pile(game.objects[i]) && approach(game.objects[i]->pos, home) <= 24)
				total += game.objects[i]->s_deposit_gems_pile.gems;
		if (total < guaranteed)
			guaranteed = total;
	}
	return guaranteed == ULONG_MAX ? 0 : guaranteed;
}

bool ft_runner_can_collect(const t_obj *pile, const t_obj *core)
{
	if (!core || !safe_pile(pile) || game.grid_size > RUNNER_GRID)
		return false;
	const t_obj *runner = NULL;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id == game.my_team_id &&
			o->s_unit.name && !strcmp(o->s_unit.name, UNIT_NAME_RUNNER))
		{
			runner = o;
			break;
		}
	}
	if (!runner)
		return false;
	static unsigned long tick = ULONG_MAX, id;
	static t_pos position;
	static unsigned int out[RUNNER_GRID][RUNNER_GRID], home[RUNNER_GRID][RUNNER_GRID];
	if (tick != game.elapsed_ticks || id != runner->id || ft_distance(position, runner->pos))
	{
		tick = game.elapsed_ticks;
		id = runner->id;
		position = runner->pos;
		distances(runner->pos, out, true, runner->id);
		distances(core->pos, home, true, runner->id);
	}
	const int dx[4] = {1, 0, -1, 0}, dy[4] = {0, 1, 0, -1};
	for (int d = 0; d < 4; d++)
	{
		int x = pile->pos.x + dx[d], y = pile->pos.y + dy[d];
		if (x >= 0 && y >= 0 && x < game.grid_size && y < game.grid_size &&
			out[y][x] < RUNNER_INF && home[y][x] < RUNNER_INF)
			return true;
	}
	return false;
}

static t_obj *choose_pile(const t_obj *u, const t_obj *core, bool honor_friendly)
{
	if (game.grid_size > RUNNER_GRID)
		return NULL;
	unsigned int out[RUNNER_GRID][RUNNER_GRID], home[RUNNER_GRID][RUNNER_GRID];
	unsigned long room = u->s_unit.properties.max_gems - u->s_unit.gems;
	distances(u->pos, out, honor_friendly, u->id);
	distances(core->pos, home, honor_friendly, u->id);
	t_obj *best = NULL;
	long best_score = LONG_MIN;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (!safe_pile(o))
			continue;
		unsigned long gain = o->s_deposit_gems_pile.gems < room ? o->s_deposit_gems_pile.gems : room;
		unsigned long step = u->s_unit.properties.gems_per_cooldown_step;
		unsigned long loaded = (u->s_unit.properties.base_action_cooldown > 0 ?
			(unsigned long)u->s_unit.properties.base_action_cooldown : 0) +
			(u->s_unit.gems + gain) / (step ? step : 1);
		if (!loaded)
			loaded = 1;
		unsigned long trip = RUNNER_INF;
		const int dx[4] = {1, 0, -1, 0}, dy[4] = {0, 1, 0, -1};
		for (int d = 0; d < 4; d++)
		{
			int x = o->pos.x + dx[d], y = o->pos.y + dy[d];
			if (x < 0 || y < 0 || x >= game.grid_size || y >= game.grid_size ||
				out[y][x] >= RUNNER_INF || home[y][x] >= RUNNER_INF)
				continue;
			unsigned long cost = out[y][x] + (home[y][x] + 1) * loaded + 2;
			if (cost < trip)
				trip = cost;
		}
		if (trip >= RUNNER_INF)
			continue;
		long score = (long)(gain * 1000 / trip);
		if (o->id == g_pile_id)
			score += 10;
		if (score > best_score)
		{
			best = o;
			best_score = score;
		}
	}
	return best;
}

void handle_collectors(t_obj *core, t_obj **units)
{
	if (!core)
		return;
	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, UNIT_NAME_RUNNER))
			continue;
		if (g_runner_id != u->id)
		{
			g_runner_id = u->id;
			g_pile_id = 0;
			g_returning = false;
			g_blocked = 0;
		}
		if (u->s_unit.action_cooldown > 0)
			continue;
		unsigned long cargo = u->s_unit.gems, capacity = u->s_unit.properties.max_gems;
		if (!cargo)
			g_returning = false;
		t_obj *pile = cargo < capacity ? choose_pile(u, core, true) : NULL;
		/* A temporary Sword obstruction requests yielding, not a fake route
		 * at purchase time. Terrain and pile obstructions remain impassable. */
		if (!pile && cargo < capacity)
			pile = choose_pile(u, core, false);
		unsigned long needed = core_get_units_byName_count(UNIT_NAME_MINER) ? COST_SWORD : COST_DIGGER;
		if (cargo && (cargo >= 96 || cargo >= capacity || !pile ||
			(cargo >= 48 && core->s_core.gems < needed && core->s_core.gems + cargo >= needed)))
			g_returning = true;
		if (g_returning)
		{
			g_pile_id = 0;
			core_debug_addObjectInfo(u, "RUNNER deliver %lu gems", cargo);
			if (ft_distance(u->pos, core->pos) == 1)
				core_action_transferGems(u, core->pos, cargo);
			else
				nav_move_adjacent_to(u, core->pos, false);
		}
		else if (pile)
		{
			g_pile_id = pile->id;
			core_debug_addObjectInfo(u, "RUNNER collect #%lu; cargo %lu", pile->id, cargo);
			if (ft_distance(u->pos, pile->pos) == 1)
			{
				core_action_attack(u, pile);
				g_blocked = 0;
			}
			else if (nav_move_adjacent_to(u, pile->pos, false))
				g_blocked = 0;
			else if (++g_blocked >= 4)
			{
				g_pile_id = 0;
				g_returning = cargo > 0;
				g_blocked = 0;
			}
		}
		else
		{
			core_debug_addObjectInfo(u, "RUNNER await loose gems from digger");
			/* Clear the spawn/unload tiles while waiting. */
			for (unsigned short y = 0; y < game.grid_size; y++)
				for (unsigned short x = 0; x < game.grid_size; x++)
				{
					t_pos p = {x, y};
					if (ft_distance(p, core->pos) == 2 && !core_get_obj_from_pos(p) && !nav_is_reserved(p))
					{
						if (ft_distance(u->pos, core->pos) != 2)
							nav_move_to_exact(u, p, false);
						goto parked;
					}
				}
		}
parked:
		;
	}
}
