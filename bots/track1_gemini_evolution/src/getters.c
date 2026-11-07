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

static bool is_core(const t_obj *obj) { return (obj->type == OBJ_CORE); }
static bool is_core_own(const t_obj *obj) { return (is_core(obj) && obj->s_core.team_id == game.my_team_id); }
static bool is_core_opponent(const t_obj *obj) { return (is_core(obj) && obj->s_core.team_id != game.my_team_id); }
static bool is_deposit(const t_obj *obj) { return (obj->type == OBJ_DEPOSIT); }
static bool is_gems(const t_obj *obj) { return (obj->type == OBJ_GEM_PILE); }
static bool is_wall(const t_obj *obj) { return (obj->type == OBJ_WALL); }
static bool is_unit(const t_obj *obj) { return (obj->type == OBJ_UNIT); }
static bool is_unit_own(const t_obj *obj) { return (is_unit(obj) && obj->s_unit.team_id == game.my_team_id); }
static bool is_unit_opponent(const t_obj *obj) { return (is_unit(obj) && obj->s_unit.team_id != game.my_team_id); }

t_obj *ft_get_core_own(void)
{
	t_pos pos = {0, 0};
	return core_get_obj_filter_nearest(pos, is_core_own);
}

t_obj *ft_get_core_opponent(void)
{
	t_pos pos = {0, 0};
	return core_get_obj_filter_nearest(pos, is_core_opponent);
}

t_obj *ft_get_deposit_nearest(t_pos pos) { return core_get_obj_filter_nearest(pos, is_deposit); }
t_obj *ft_get_gems_nearest(t_pos pos) { return core_get_obj_filter_nearest(pos, is_gems); }
t_obj *ft_get_wall_nearest(t_pos pos) { return core_get_obj_filter_nearest(pos, is_wall); }
unsigned int ft_count_walls(void) { return core_get_objs_filter_count(is_wall); }
t_obj **ft_get_units_own(void) { return core_get_objs_filter(is_unit_own); }
t_obj **ft_get_units_opponent(void) { return core_get_objs_filter(is_unit_opponent); }
t_obj *ft_get_units_opponent_nearest(t_pos pos) { return core_get_obj_filter_nearest(pos, is_unit_opponent); }

t_obj *ft_get_threat_nearest(t_pos pos, unsigned int max_dist)
{
	t_obj *opp = ft_get_units_opponent_nearest(pos);
	if (!opp)
		return NULL;
	if (ft_distance(pos, opp->pos) <= max_dist)
		return opp;
	return NULL;
}

bool ft_is_pos_near_armed_enemy(t_pos pos, unsigned int radius)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id &&
			o->s_unit.properties.damage_unit > 0 && o->hp > 0)
		{
			if (ft_distance(pos, o->pos) <= radius)
				return true;
		}
	}
	return false;
}

bool ft_is_enemy_adjacent_to_pos(t_pos pos)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id &&
			o->hp > 0 && ft_distance(pos, o->pos) <= 1)
			return true;
	}
	return false;
}

unsigned int ft_count_allies_near_pos(t_pos pos, unsigned int radius)
{
	unsigned int count = 0;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id == game.my_team_id &&
			o->s_unit.properties.damage_unit > 0 && o->hp > 0 &&
			ft_distance(o->pos, pos) <= radius)
		{
			count++;
		}
	}
	return count;
}

// Overkill-aware, focus-fire adjacent target selector
t_obj *ft_get_best_adjacent_enemy(const t_obj *attacker)
{
	if (!attacker || attacker->s_unit.properties.damage_unit == 0)
		return NULL;

	t_obj **opps = ft_get_units_opponent();
	t_obj *best = NULL;
	long best_score = -999999;

	for (int i = 0; opps && opps[i]; i++)
	{
		t_obj *e = opps[i];
		if (!ft_is_adjacent(attacker->pos, e->pos))
			continue;

		unsigned long pending = get_pending_damage(e->id);
		if (pending >= e->hp)
			continue;

		unsigned long rem_hp = e->hp - pending;
		unsigned long raw_dmg = attacker->s_unit.properties.damage_unit;
		int red = e->s_unit.properties.damage_reduction_percent;
		unsigned long eff_dmg = (raw_dmg * (unsigned long)(100 - red)) / 100;
		if (eff_dmg == 0 && raw_dmg > 0)
			eff_dmg = 1;

		long score = 1000 - (long)(rem_hp * 30);

		if (eff_dmg >= rem_hp)
			score += 800;

		if (e->s_unit.properties.damage_unit > 0)
		{
			score += (long)(e->s_unit.properties.damage_unit * 60);
			if (e->s_unit.properties.base_action_cooldown <= 3)
				score += 250;
		}

		if (e->s_unit.gems > 0)
			score += 150;

		if (score > best_score)
		{
			best_score = score;
			best = e;
		}
	}
	free(opps);
	return best;
}

bool ft_is_core_threatened(t_obj *core, unsigned int radius)
{
	if (!core)
		return false;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id &&
			(o->s_unit.properties.damage_unit > 0 || o->s_unit.properties.damage_core > 0))
		{
			if (ft_distance(core->pos, o->pos) <= radius)
				return true;
		}
	}
	return false;
}

unsigned int ft_count_active_resources(void)
{
	unsigned int count = 0;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->type == OBJ_DEPOSIT || o->type == OBJ_GEM_PILE)
			count++;
	}
	return count;
}

unsigned int ft_count_safe_resources(t_pos core_pos)
{
	unsigned int count = 0;
	t_obj *opp_core = ft_get_core_opponent();
	bool enemy_cleared = ft_enemy_units_cleared();

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->type != OBJ_DEPOSIT && o->type != OBJ_GEM_PILE)
			continue;
		if (o->s_deposit_gems_pile.gems == 0)
			continue;
		if (ft_is_pos_near_armed_enemy(o->pos, 3))
			continue;
		if (!enemy_cleared && game.elapsed_ticks <= 350 && opp_core &&
			ft_distance(o->pos, opp_core->pos) + 4 < ft_distance(o->pos, core_pos))
			continue;
		count++;
	}
	return count;
}

unsigned int ft_count_enemy_combat_units(void)
{
	unsigned int count = 0;
	t_obj **opps = ft_get_units_opponent();
	for (int i = 0; opps && opps[i]; i++)
	{
		if (opps[i]->s_unit.properties.damage_unit > 0 || opps[i]->s_unit.properties.damage_core > 0)
			count++;
	}
	free(opps);
	return count;
}

unsigned int ft_count_own_combat_units(void)
{
	unsigned int count = 0;
	t_obj **own = ft_get_units_own();
	for (int i = 0; own && own[i]; i++)
	{
		if (own[i]->s_unit.properties.damage_unit > 0)
			count++;
	}
	free(own);
	return count;
}

unsigned long ft_get_enemy_max_hp(void)
{
	unsigned long max_hp = 0;
	t_obj **opps = ft_get_units_opponent();
	for (int i = 0; opps && opps[i]; i++)
	{
		if (opps[i]->s_unit.properties.damage_unit > 0 && opps[i]->hp > max_hp)
			max_hp = opps[i]->hp;
	}
	free(opps);
	return max_hp;
}

unsigned int ft_count_enemy_speed_warriors(void)
{
	unsigned int count = 0;
	t_obj **opps = ft_get_units_opponent();
	for (int i = 0; opps && opps[i]; i++)
	{
		t_obj *e = opps[i];
		if (e->s_unit.properties.damage_unit > 0 &&
			(e->s_unit.properties.base_action_cooldown <= 3 || e->hp <= 6))
		{
			count++;
		}
	}
	free(opps);
	return count;
}

// Checks if another friendly combat unit is already within distance <= 2 of this enemy (2v1 advantage!)
bool ft_is_enemy_engaged_by_ally(const t_obj *enemy, unsigned long exclude_unit_id)
{
	if (!enemy)
		return false;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id == game.my_team_id && o->id != exclude_unit_id)
		{
			if (o->s_unit.properties.damage_unit > 0 && ft_distance(o->pos, enemy->pos) <= 2)
				return true;
		}
	}
	return false;
}

// Verifies that an open cardinal tile exists at Manhattan distance 1 from both u and enemy
bool ft_has_open_melee_gap(const t_obj *u, const t_obj *enemy)
{
	if (!u || !enemy)
		return false;

	const int dx[4] = {1, 0, -1, 0};
	const int dy[4] = {0, 1, 0, -1};

	for (int d = 0; d < 4; d++)
	{
		int x = (int)u->pos.x + dx[d];
		int y = (int)u->pos.y + dy[d];
		if (x < 0 || y < 0 || x >= (int)game.grid_size || y >= (int)game.grid_size)
			continue;
		t_pos p = {(unsigned short)x, (unsigned short)y};
		if (ft_distance(p, enemy->pos) != 1)
			continue;

		const t_obj *o = core_get_obj_from_pos(p);
		if (!o || (o->type == OBJ_GEM_PILE &&
			o->s_deposit_gems_pile.gems <= u->s_unit.properties.max_gems - u->s_unit.gems))
		{
			return true;
		}
	}
	return false;
}

bool ft_should_hold_for_first_strike(const t_obj *u, const t_obj *target)
{
	if (!u || !target || target->type != OBJ_UNIT)
		return false;

	if (ft_distance(u->pos, target->pos) != 2)
		return false;

	if (target->s_unit.properties.damage_unit == 0)
		return false;

	if (ft_is_enemy_engaged_by_ally(target, u->id))
		return false;

	if (!ft_has_open_melee_gap(u, target))
		return false;

	if (u->s_unit.action_cooldown <= -4)
		return false;

	long enemy_cd = target->s_unit.action_cooldown;
	long my_base_cd = (long)u->s_unit.properties.base_action_cooldown;

	if (enemy_cd >= my_base_cd || enemy_cd <= -5)
		return false;

	return true;
}

static unsigned long estimate_team_power(unsigned long team_id, unsigned int *combat_count)
{
	unsigned long total_power = 0;
	unsigned int count = 0;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->type != OBJ_UNIT || o->s_unit.team_id != team_id || o->hp == 0)
			continue;

		unsigned long dmg_u = o->s_unit.properties.damage_unit;
		unsigned long dmg_c = o->s_unit.properties.damage_core;
		if (dmg_u == 0 && dmg_c == 0)
			continue;

		count++;
		unsigned long cd = (o->s_unit.properties.base_action_cooldown > 0)
			? (unsigned long)o->s_unit.properties.base_action_cooldown : 1;
		unsigned long armor_mult = 100 + (unsigned long)o->s_unit.properties.damage_reduction_percent;
		unsigned long eff_hp = (o->hp * armor_mult) / 100;
		unsigned long dps_score = ((dmg_u * 3 + dmg_c) * 6) / cd;

		total_power += eff_hp * 10 + dps_score * 15;
	}

	if (combat_count)
		*combat_count = count;
	return total_power;
}

t_game_phase ft_get_game_phase(t_obj *core, t_obj *opp_core)
{
	if (!opp_core || opp_core->hp == 0)
		return PHASE_3_ALL_OUT_ASSAULT;

	unsigned int active_res = ft_count_active_resources();
	unsigned int my_combat_units = 0;
	unsigned int opp_combat_units = 0;
	unsigned long my_power = estimate_team_power(game.my_team_id, &my_combat_units);
	unsigned long opp_power = estimate_team_power(opp_core->s_core.team_id, &opp_combat_units);

	if (opp_combat_units == 0 && my_combat_units >= 1)
		return PHASE_3_ALL_OUT_ASSAULT;

	if (my_combat_units >= MIN_ASSAULT_ARMY_SIZE &&
		(my_power * 100 >= opp_power * ASSAULT_POWER_RATIO_PCT))
	{
		return PHASE_3_ALL_OUT_ASSAULT;
	}

	if (active_res > PHASE2_RESOURCE_CUTOFF)
		return PHASE_1_AGGRESSIVE_ECONOMY;

	if (core && core->s_core.gems >= COST_HUNTER_TANK && my_combat_units < 10)
		return PHASE_2_TACTICAL_DEFENSE;

	if (my_power >= opp_power && my_combat_units >= 2)
		return PHASE_3_ALL_OUT_ASSAULT;

	return PHASE_2_TACTICAL_DEFENSE;
}

static bool is_claimed_by_other_miner(t_pos res_pos, unsigned long my_id)
{
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id == game.my_team_id && o->id != my_id)
		{
			if (o->s_unit.name && strcmp(o->s_unit.name, "Miner") == 0)
			{
				if (ft_distance(o->pos, res_pos) <= 2)
					return true;
			}
		}
	}
	return false;
}

t_obj *ft_get_resource_for_miner(t_pos unit_pos, t_pos core_pos, unsigned long unit_id)
{
	t_obj *core = ft_get_core_own();
	t_obj *opp_core = ft_get_core_opponent();
	t_obj *best = NULL;
	long best_score = LONG_MIN;
	unsigned int n_carriers = core_get_units_byName_count("Carrier");

	unsigned int dist_grid[MAX_GRID_SIZE][MAX_GRID_SIZE];
	nav_compute_corridor_distances(unit_pos, dist_grid, true, unit_id);

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *obj = game.objects[i];
		if (obj->type != OBJ_DEPOSIT && obj->type != OBJ_GEM_PILE)
			continue;
		if (obj->s_deposit_gems_pile.gems == 0)
			continue;

		// If Carrier can reach a Gem Pile, leave it for the Carrier!
		if (obj->type == OBJ_GEM_PILE && n_carriers > 0 &&
			ft_carrier_can_reach_pile(obj, core))
		{
			continue;
		}

		if (ft_is_pos_near_armed_enemy(obj->pos, 3))
			continue;

		if (is_claimed_by_other_miner(obj->pos, unit_id))
			continue;

		unsigned int app_dist = nav_approach_distance(obj->pos, dist_grid);
		bool in_open_corridor = (app_dist < INF_DIST);
		unsigned int dist_u = in_open_corridor ? app_dist : ft_distance(unit_pos, obj->pos);
		unsigned int dist_c = ft_distance(core_pos, obj->pos);
		long score = 1000 - (long)(dist_u * 25) - (long)(dist_c * 12);
		if (opp_core && ft_distance(opp_core->pos, obj->pos) <= dist_c)
			score -= 250;

		if (obj->type == OBJ_DEPOSIT)
		{
			score -= 180 + (long)(obj->hp * 3);
			// With an active Carrier, strongly prioritize cracking Deposits over locked piles
			if (n_carriers > 0)
				score += 10000;
			if (in_open_corridor)
				score += 50000;
		}

		if (score > best_score)
		{
			best_score = score;
			best = obj;
		}
	}
	return best;
}
