#include "bot.h"

static __attribute__((unused)) t_pos get_defensive_perimeter_slot(const t_obj *u, const t_obj *core)
{
	t_pos slot = core->pos;
	int dist = DEFENSE_PERIMETER_DIST;
	int offset = (int)(u->id % 4);

	if (core->pos.x < game.grid_size / 2)
	{
		slot.x = (unsigned short)(core->pos.x + (unsigned short)(dist - offset));
		slot.y = (unsigned short)(core->pos.y + (unsigned short)(offset + 1));
	}
	else
	{
		slot.x = (core->pos.x >= (unsigned short)(dist - offset))
			? (unsigned short)(core->pos.x - (unsigned short)(dist - offset)) : 0;
		slot.y = (core->pos.y >= (unsigned short)(offset + 1))
			? (unsigned short)(core->pos.y - (unsigned short)(offset + 1)) : 0;
	}

	if (!is_pos_in_bounds(slot))
		return core->pos;
	return slot;
}

static void handle_defender(t_obj *u, t_obj *core, t_obj *opp_core, t_game_phase phase)
{
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

static void handle_hunter_or_scout(t_obj *u, t_obj *core, t_obj *opp_core,
									t_game_phase phase, unsigned long shared_focus_id)
{
	// 1. Stage 3 Attack: strike best adjacent enemy immediately
	t_obj *adj_opp = ft_get_best_adjacent_enemy(u);
	if (adj_opp)
	{
		core_action_attack(u, adj_opp);
		return;
	}

	// 2. Select highest-priority enemy unit (with Pack Focus-Fire!)
	t_obj **opps = ft_get_units_opponent();
	t_obj *best_target = NULL;
	long best_priority = -999999;

	for (int i = 0; opps && opps[i]; i++)
	{
		t_obj *e = opps[i];
		if (get_pending_damage(e->id) >= e->hp)
			continue;

		unsigned int dist_me = ft_distance(u->pos, e->pos);
		unsigned int dist_core = core ? ft_distance(core->pos, e->pos) : 20;
		bool is_unarmed = (e->s_unit.properties.damage_unit == 0 && e->s_unit.properties.damage_core == 0);

		if (phase == PHASE_2_TACTICAL_DEFENSE && !is_unarmed &&
			dist_core > CORE_THREAT_RADIUS + 4 && dist_me > 5)
		{
			continue;
		}

		long priority = 1000 - (long)(dist_me * 35);
		priority -= (long)(e->hp * 8);

		// Pack Focus-Fire: gang up on the shared priority enemy or any enemy already engaged by an ally!
		if (e->id == shared_focus_id && dist_me <= 8)
			priority += 700;
		if (ft_is_enemy_engaged_by_ally(e, u->id))
			priority += 900;

		// Instant 1-shot kill bonus
		if (u->s_unit.properties.damage_unit >= e->hp)
			priority += 1000;

		// Unarmed worker assassination bonus (when not in immediate melee with an enemy fighter)
		if (is_unarmed && dist_me <= 6)
			priority += 1200 + (long)(e->s_unit.gems * 5);

		// Core defense urgency (matches Monir's +5000 core defense priority!)
		if (core && dist_core <= CORE_THREAT_RADIUS && !is_unarmed)
			priority += 5000;

		// Escort Breach Bonus: clear enemy units corking the opponent's Core
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
			core_action_attack(u, best_target);
		}
		else if (ft_should_hold_for_first_strike(u, best_target))
		{
			step_away_from_core(u, core);
		}
		else
		{
			safe_travel(u, best_target->pos);
		}
		return;
	}

	// 3. If all enemy units are wiped out, surround the enemy Core at distance 2-3
	// (leaving distance 1 open for our Speed Sieger, while spawn-camping any new enemy unit!)
	if (opp_core)
	{
		unsigned int d_opp = ft_distance(u->pos, opp_core->pos);
		if (u->s_unit.properties.damage_core > 0 && d_opp == 1)
		{
			core_action_attack(u, opp_core);
		}
		else if (d_opp > 3)
		{
			safe_travel(u, opp_core->pos);
		}
		else if (d_opp == 1 && u->s_unit.properties.damage_core == 0)
		{
			step_away_from_core(u, opp_core);
		}
	}
}

static void handle_striker(t_obj *u, t_obj *core, t_obj *opp_core)
{
	(void)core;
	if (!opp_core || opp_core->hp == 0)
	{
		t_obj *opp_unit = ft_get_units_opponent_nearest(u->pos);
		if (opp_unit && u->s_unit.properties.damage_unit > 0)
		{
			if (ft_is_adjacent(u->pos, opp_unit->pos))
				core_action_attack(u, opp_unit);
			else
				safe_travel(u, opp_unit->pos);
		}
		return;
	}

	if (ft_is_adjacent(u->pos, opp_core->pos))
	{
		core_action_attack(u, opp_core);
		return;
	}

	if (u->s_unit.properties.damage_unit > 0)
	{
		t_obj *adj_opp = ft_get_best_adjacent_enemy(u);
		if (adj_opp)
		{
			core_action_attack(u, adj_opp);
			return;
		}
	}

	safe_travel(u, opp_core->pos);
}

void handle_combat(t_obj *core, t_obj *opp_core, t_obj **units)
{
	t_game_phase phase = ft_get_game_phase(core, opp_core);

	// Find primary shared focus target (closest armed enemy to our Core)
	unsigned long shared_focus_id = 0;
	if (core)
	{
		t_obj *threat = ft_get_units_opponent_nearest(core->pos);
		if (threat)
			shared_focus_id = threat->id;
	}

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || u->s_unit.action_cooldown > 0)
			continue;

		if (strcmp(u->s_unit.name, "Defender") == 0)
			handle_defender(u, core, opp_core, phase);
		else if (strcmp(u->s_unit.name, "Hunter") == 0 || strcmp(u->s_unit.name, "Scout") == 0)
			handle_hunter_or_scout(u, core, opp_core, phase, shared_focus_id);
		else if (strcmp(u->s_unit.name, "Striker") == 0)
			handle_striker(u, core, opp_core);
	}
}
