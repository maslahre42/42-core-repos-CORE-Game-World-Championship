#include "bot.h"

static bool g_seen_enemy_unit = false;

typedef struct s_wait_entry
{
	unsigned long unit_id;
	unsigned long since_tick;
	bool active;
} t_wait_entry;

static t_wait_entry g_waits[64];

static unsigned long get_unit_wait_ticks(unsigned long unit_id, bool should_wait)
{
	int slot = -1;
	for (int i = 0; i < 64; i++)
	{
		if (g_waits[i].active && g_waits[i].unit_id == unit_id)
		{
			slot = i;
			break;
		}
		if (!g_waits[i].active && slot == -1)
			slot = i;
	}

	if (!should_wait)
	{
		if (slot != -1 && g_waits[slot].unit_id == unit_id)
			g_waits[slot].active = false;
		return 0;
	}

	if (slot != -1)
	{
		if (!g_waits[slot].active || g_waits[slot].unit_id != unit_id)
		{
			g_waits[slot].unit_id = unit_id;
			g_waits[slot].since_tick = game.elapsed_ticks;
			g_waits[slot].active = true;
		}
		return (game.elapsed_ticks - g_waits[slot].since_tick);
	}
	return 0;
}

bool ft_enemy_units_cleared(void)
{
	bool enemy_armed_alive = false;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id && o->hp > 0)
		{
			g_seen_enemy_unit = true;
			if (o->s_unit.properties.damage_unit > 0 || o->s_unit.properties.damage_core > 0)
				enemy_armed_alive = true;
		}
	}
	return (g_seen_enemy_unit && !enemy_armed_alive) || (game.elapsed_ticks > 350 && !enemy_armed_alive);
}

static void handle_defender(t_obj *u, t_obj *core, t_obj *opp_core, t_game_phase phase)
{
	if (!u || u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
		return;
	if (!core)
		return;

	t_obj *adj_opp = ft_get_best_adjacent_enemy(u);
	if (adj_opp)
	{
		core_action_attack(u, adj_opp);
		return;
	}

	t_obj *threat = ft_get_threat_nearest(core->pos, DEFENDER_INTERCEPT_RADIUS);
	if (threat)
	{
		if (ft_is_adjacent(u->pos, threat->pos))
			core_action_attack(u, threat);
		else if (!ft_should_hold_for_first_strike(u, threat))
			safe_travel(u, threat->pos);
		return;
	}

	if (phase == PHASE_3_ALL_OUT_ASSAULT || !opp_core || opp_core->hp == 0)
	{
		t_obj *opp = ft_get_units_opponent_nearest(u->pos);
		if (opp)
		{
			if (ft_is_adjacent(u->pos, opp->pos))
				core_action_attack(u, opp);
			else if (!ft_should_hold_for_first_strike(u, opp))
				safe_travel(u, opp->pos);
			return;
		}
	}

	t_pos choke1 = core->pos;
	t_pos choke2 = core->pos;
	if (core->pos.x == 0 && core->pos.y == 0)
	{
		choke1.x = 2; choke1.y = 0;
		choke2.x = 0; choke2.y = 2;
	}
	else
	{
		choke1.x = (core->pos.x >= 2) ? core->pos.x - 2 : 0;
		choke1.y = core->pos.y;
		choke2.x = core->pos.x;
		choke2.y = (core->pos.y >= 2) ? core->pos.y - 2 : 0;
	}

	t_pos choke = (u->id % 2 == 0) ? choke1 : choke2;
	t_obj *occ = core_get_obj_from_pos(choke);
	if (occ && occ->id != u->id)
		choke = (u->id % 2 == 0) ? choke2 : choke1;

	if (ft_distance(u->pos, choke) > 0)
		safe_travel(u, choke);
}

// Assigns Hunters to guard posts at distance 2-4 around enemy Core OUTSIDE the Finisher's lane!
static void position_hunter_around_enemy_core(t_obj *u, t_obj *opp_core,
	const bool lane[MAX_GRID_SIZE][MAX_GRID_SIZE],
	bool claimed_posts[MAX_GRID_SIZE][MAX_GRID_SIZE])
{
	if (!u || !opp_core)
		return;

	unsigned int d_opp = ft_distance(u->pos, opp_core->pos);
	if (u->s_unit.properties.damage_core > 0 && d_opp == 1)
	{
		core_action_attack(u, opp_core);
		return;
	}

	t_pos best_post = u->pos;
	unsigned int best_score = UINT_MAX;
	bool found = false;

	for (unsigned short y = 0; y < game.grid_size && y < MAX_GRID_SIZE; y++)
	{
		for (unsigned short x = 0; x < game.grid_size && x < MAX_GRID_SIZE; x++)
		{
			t_pos p = {x, y};
			unsigned int d = ft_distance(p, opp_core->pos);
			if (d < 2 || d > 4 || lane[y][x] || claimed_posts[y][x])
				continue;
			t_obj *occ = core_get_obj_from_pos(p);
			if (occ && occ->id != u->id)
				continue;
			if (is_tile_occupied(p) && (p.x != u->pos.x || p.y != u->pos.y))
				continue;

			unsigned int score = (d - 2) * 300 + ft_distance(u->pos, p) * 10;
			// Prefer cardinal distance-2 posts that cover enemy spawn exits
			if (d == 2 && (p.x == opp_core->pos.x || p.y == opp_core->pos.y))
				score = (score > 100) ? (score - 100) : 0;

			if (score < best_score)
			{
				best_score = score;
				best_post = p;
				found = true;
			}
		}
	}

	if (found)
	{
		claimed_posts[best_post.y][best_post.x] = true;
		if (best_post.x != u->pos.x || best_post.y != u->pos.y)
			safe_travel(u, best_post);
	}
	else if (d_opp == 1 || (u->pos.x < MAX_GRID_SIZE && u->pos.y < MAX_GRID_SIZE && lane[u->pos.y][u->pos.x]))
	{
		step_away_from_core(u, opp_core);
	}
}

static void handle_hunter_or_scout(t_obj *u, t_obj *core, t_obj *opp_core,
	t_game_phase phase, unsigned long shared_focus_id,
	const bool lane[MAX_GRID_SIZE][MAX_GRID_SIZE],
	bool claimed_posts[MAX_GRID_SIZE][MAX_GRID_SIZE])
{
	(void)phase;
	if (!u || u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
		return;

	// 1. Attack best adjacent enemy immediately (executes in Stage 3 before movement)
	t_obj *adj_opp = ft_get_best_adjacent_enemy(u);
	if (adj_opp)
	{
		get_unit_wait_ticks(u->id, false);
		core_action_attack(u, adj_opp);
		return;
	}

	// 2. Select highest-priority enemy unit (with Pack Focus-Fire)
	t_obj **opps = ft_get_units_opponent();
	t_obj *best_target = NULL;
	long best_priority = LONG_MIN;

	for (int i = 0; opps && opps[i]; i++)
	{
		t_obj *e = opps[i];
		if (get_pending_damage(e->id) >= e->hp)
			continue;

		unsigned int dist_me = ft_distance(u->pos, e->pos);
		unsigned int dist_core = core ? ft_distance(core->pos, e->pos) : 20;
		bool is_unarmed = (e->s_unit.properties.damage_unit == 0 && e->s_unit.properties.damage_core == 0);

		// Prevent infinite rabbit-chase: if all armed enemies are dead, do not chase distant 1-tick runners!
		// Advance directly to enemy Core instead, where returning runners will be trapped and eliminated.
		if (ft_enemy_units_cleared() && is_unarmed && dist_me > 3)
			continue;

		long priority = 1000 - (long)(dist_me * 50) - (long)(e->hp * 5);
		if (e->id == shared_focus_id && dist_me <= 8)
			priority += 700;
		if (ft_is_enemy_engaged_by_ally(e, u->id))
			priority += 900;
		if (u->s_unit.properties.damage_unit >= e->hp)
			priority += 1000;
		if (is_unarmed)
			priority += 250 + (long)(e->s_unit.gems * 2);
		else if (dist_me <= 5)
			priority += 500;
		if (core && dist_core <= 6 && dist_me <= 10 && !is_unarmed)
			priority += 5000;
		if (opp_core && core_get_units_byName_count("Striker") > 0 &&
			ft_distance(e->pos, opp_core->pos) <= 2)
		{
			priority += 2500;
		}

		if (priority > best_priority)
		{
			best_priority = priority;
			best_target = e;
		}
	}
	free(opps);

	if (best_target)
	{
		if (ft_is_adjacent(u->pos, best_target->pos))
		{
			get_unit_wait_ticks(u->id, false);
			core_action_attack(u, best_target);
			return;
		}

		// BUG #7 FIX: Bounded Distance-2 First-Strike Ambush with Open Melee Gap verification
		bool raw_hold = false;
		unsigned long step = u->s_unit.properties.gems_per_cooldown_step;
		long move_cd = (long)u->s_unit.properties.base_action_cooldown +
			(long)(u->s_unit.gems / (step ? step : 1));
		if (move_cd < 1)
			move_cd = 1;

		for (int j = 0; game.objects && game.objects[j]; j++)
		{
			const t_obj *e = game.objects[j];
			if (e->type == OBJ_UNIT && e->s_unit.team_id != game.my_team_id &&
				e->s_unit.properties.damage_unit > 0 && e->hp > 0 &&
				ft_distance(u->pos, e->pos) == 2 &&
				e->s_unit.action_cooldown < move_cd &&
				ft_has_open_melee_gap(u, e))
			{
				raw_hold = true;
				break;
			}
		}

		unsigned long waited = get_unit_wait_ticks(u->id, raw_hold);
		if (raw_hold && waited < 6)
		{
			return;
		}

		safe_travel(u, best_target->pos);
		return;
	}

	// 3. BUG #2 FIX: When enemy units are cleared, guard enemy Core at distance 2-4
	// OUTSIDE the Finisher's corridor so Striker can march straight to distance 1!
	get_unit_wait_ticks(u->id, false);
	if (opp_core)
		position_hunter_around_enemy_core(u, opp_core, lane, claimed_posts);
}

static void handle_striker(t_obj *u, t_obj *core, t_obj *opp_core)
{
	(void)core;
	if (!u || u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
		return;
	if (!opp_core || opp_core->hp == 0)
		return;

	if (ft_is_adjacent(u->pos, opp_core->pos))
	{
		core_action_attack(u, opp_core);
		return;
	}
	safe_travel(u, opp_core->pos);
}

void handle_combat(t_obj *core, t_obj *opp_core, t_obj **units)
{
	t_game_phase phase = ft_get_game_phase(core, opp_core);
	unsigned long shared_focus_id = 0;
	if (core)
	{
		t_obj *threat = ft_get_units_opponent_nearest(core->pos);
		if (threat)
			shared_focus_id = threat->id;
	}

	// Compute Finisher lane from active Striker (or our Core) to enemy Core
	bool finisher_lane[MAX_GRID_SIZE][MAX_GRID_SIZE];
	bool claimed_posts[MAX_GRID_SIZE][MAX_GRID_SIZE] = {{false}};
	memset(finisher_lane, 0, sizeof(finisher_lane));

	if (core && opp_core)
	{
		t_pos lane_start = core->pos;
		for (int i = 0; units && units[i]; i++)
		{
			if (units[i]->s_unit.name && strcmp(units[i]->s_unit.name, "Striker") == 0)
			{
				lane_start = units[i]->pos;
				break;
			}
		}
		nav_compute_finisher_lane(lane_start, opp_core->pos, finisher_lane);
	}

	// Run Strikers FIRST when enemy units are cleared so they claim the approach tile
	if (ft_enemy_units_cleared())
	{
		for (int i = 0; units && units[i]; i++)
		{
			t_obj *u = units[i];
			if (u->s_unit.name && strcmp(u->s_unit.name, "Striker") == 0)
				handle_striker(u, core, opp_core);
		}
	}

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
			continue;

		if (strcmp(u->s_unit.name, "Defender") == 0)
			handle_defender(u, core, opp_core, phase);
		else if (strcmp(u->s_unit.name, "Hunter") == 0 || strcmp(u->s_unit.name, "Scout") == 0)
			handle_hunter_or_scout(u, core, opp_core, phase, shared_focus_id, finisher_lane, claimed_posts);
		else if (strcmp(u->s_unit.name, "Striker") == 0)
			handle_striker(u, core, opp_core);
	}
}
