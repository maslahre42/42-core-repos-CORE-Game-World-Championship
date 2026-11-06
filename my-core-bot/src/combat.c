#include "bot.h"

static void handle_defender(t_obj *u, t_obj *core, t_obj *opp_core)
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
		else
			safe_travel(u, threat->pos);
		return;
	}

	if (!opp_core || opp_core->hp == 0)
	{
		t_obj *opp = ft_get_units_opponent_nearest(u->pos);
		if (opp)
		{
			if (ft_is_adjacent(u->pos, opp->pos))
				core_action_attack(u, opp);
			else
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

static void handle_hunter(t_obj *u, t_obj *core, t_obj *opp_core)
{
	t_obj *adj_opp = ft_get_best_adjacent_enemy(u);
	if (adj_opp)
	{
		core_action_attack(u, adj_opp);
		return;
	}

	t_obj **opps = ft_get_units_opponent();
	t_obj *best_target = NULL;
	long best_priority = -999999;

	for (int i = 0; opps && opps[i]; i++)
	{
		t_obj *e = opps[i];
		if (get_pending_damage(e->id) >= e->hp)
			continue;

		long priority = 1000 - (long)(ft_distance(u->pos, e->pos) * 15);
		priority -= (long)(e->hp * HUNTER_LOW_HP_WEIGHT);

		// High bounty on unarmed carriers/miners
		if (e->s_unit.properties.damage_unit == 0 && e->s_unit.properties.damage_core == 0)
			priority += HUNTER_UNARMED_BOUNTY;

		// Bounty on enemies carrying gems
		if (e->s_unit.gems > 0)
			priority += HUNTER_GEM_CARRIER_BOUNTY;

		// Urgent bounty on enemies threatening our Core
		if (core && ft_distance(core->pos, e->pos) <= CORE_THREAT_RADIUS)
			priority += HUNTER_CORE_THREAT_BOUNTY;

		// Escort Breach Bonus: if we have Strikers alive, clear enemy units
		// corking the tiles around the opponent's Core!
		if (opp_core && core_get_units_byName_count("Striker") > 0 &&
			ft_distance(e->pos, opp_core->pos) <= 2)
			priority += 2500;

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
			core_action_attack(u, best_target);
		else
			safe_travel(u, best_target->pos);
		return;
	}

	if (opp_core)
	{
		if (u->s_unit.properties.damage_core > 0)
		{
			if (ft_is_adjacent(u->pos, opp_core->pos))
				core_action_attack(u, opp_core);
			else
				safe_travel(u, opp_core->pos);
		}
		else
		{
			// If adjacent to opponent Core without siege damage, step away to clear slots for Strikers!
			if (ft_is_adjacent(u->pos, opp_core->pos))
				step_away_from_core(u, opp_core);
			else if (ft_distance(u->pos, opp_core->pos) > 2)
				safe_travel(u, opp_core->pos);
		}
	}
}

static void handle_striker(t_obj *u, t_obj *core, t_obj *opp_core, unsigned int n_strikers)
{
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

	// 1. Attack opponent Core immediately if adjacent
	if (ft_is_adjacent(u->pos, opp_core->pos))
	{
		core_action_attack(u, opp_core);
		return;
	}

	// 2. Focus-fire adjacent enemy unit blocking our advance (only if capable of combat damage)
	if (u->s_unit.properties.damage_unit > 0)
	{
		t_obj *adj_opp = ft_get_best_adjacent_enemy(u);
		if (adj_opp)
		{
			core_action_attack(u, adj_opp);
			return;
		}
	}

	// 3. Lanchester Squad Rally: if newly spawned near our Core and alone, rally briefly
	if (core && n_strikers < SQUAD_RALLY_SIZE && ft_distance(u->pos, core->pos) <= SQUAD_RALLY_DIST)
	{
		t_obj *threat = ft_get_threat_nearest(core->pos, CORE_THREAT_RADIUS);
		if (threat)
		{
			safe_travel(u, threat->pos);
			return;
		}
	}

	// 4. March straight to opponent Core
	safe_travel(u, opp_core->pos);
}

void handle_combat(t_obj *core, t_obj *opp_core, t_obj **units)
{
	unsigned int n_strikers = core_get_units_byName_count("Striker");

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || u->s_unit.action_cooldown > 0)
			continue;

		if (strcmp(u->s_unit.name, "Defender") == 0)
			handle_defender(u, core, opp_core);
		else if (strcmp(u->s_unit.name, "Hunter") == 0)
			handle_hunter(u, core, opp_core);
		else if (strcmp(u->s_unit.name, "Striker") == 0)
			handle_striker(u, core, opp_core, n_strikers);
	}
}
