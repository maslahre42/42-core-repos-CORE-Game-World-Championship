#include "bot.h"

static void handle_defender(t_obj *u, t_obj *core, t_obj *opp_core)
{
	if (!core)
		return;

	// 1. Attack adjacent enemy immediately
	t_obj *adj_opp = ft_get_adjacent_enemy(u->pos);
	if (adj_opp)
	{
		core_action_attack(u, adj_opp);
		core_debug_addObjectInfo(u, "Defender striking adjacent enemy #%lu", adj_opp->id);
		return;
	}

	// 2. Intercept any enemy within distance 7 of our Core
	t_obj *threat = ft_get_threat_nearest(core->pos, 7);
	if (threat)
	{
		if (ft_is_adjacent(u->pos, threat->pos))
			core_action_attack(u, threat);
		else
			safe_travel(u, threat->pos);
		core_debug_addObjectInfo(u, "Defender intercepting threat at (%u, %u)", threat->pos.x, threat->pos.y);
		return;
	}

	// 3. If opponent Core is dead, help sweep remaining enemy units
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

	// 4. Anchor at defensive choke point (2 tiles from Core)
	t_pos choke1 = core->pos;
	t_pos choke2 = core->pos;
	if (core->pos.x == 0 && core->pos.y == 0)
	{
		choke1.x = 2; choke1.y = 0;
		choke2.x = 0; choke2.y = 2;
	}
	else
	{
		choke1.x = core->pos.x - 2; choke1.y = core->pos.y;
		choke2.x = core->pos.x; choke2.y = core->pos.y - 2;
	}

	t_pos choke = (u->id % 2 == 0) ? choke1 : choke2;
	t_obj *occ = core_get_obj_from_pos(choke);
	if (occ && occ->id != u->id)
		choke = (u->id % 2 == 0) ? choke2 : choke1;

	if (ft_distance(u->pos, choke) > 0)
		safe_travel(u, choke);
	core_debug_addObjectInfo(u, "Defender guarding Core choke point");
}

static void handle_hunter(t_obj *u, t_obj *core, t_obj *opp_core)
{
	(void)core;
	// 1. Attack adjacent enemy immediately
	t_obj *adj_opp = ft_get_adjacent_enemy(u->pos);
	if (adj_opp)
	{
		core_action_attack(u, adj_opp);
		core_debug_addObjectInfo(u, "Hunter striking enemy #%lu for 6 DMG!", adj_opp->id);
		return;
	}

	// 2. Hunt down closest enemy unit (prioritizing unarmed runners)
	t_obj **opps = ft_get_units_opponent();
	t_obj *best_target = NULL;
	int best_priority = -1;

	for (int i = 0; opps && opps[i]; i++)
	{
		t_obj *e = opps[i];
		int priority = 1000 - (int)ft_distance(u->pos, e->pos);

		// Assassination bounty: prioritize unarmed enemy carriers
		if (e->s_unit.properties.damage_unit == 0 && e->s_unit.properties.damage_core == 0)
			priority += 5000;

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
		core_debug_addObjectInfo(u, "Hunter tracking target #%lu", best_target->id);
		return;
	}

	// 3. Fallback: attack opponent core if no enemy units remain
	if (opp_core)
	{
		if (ft_is_adjacent(u->pos, opp_core->pos))
			core_action_attack(u, opp_core);
		else
			safe_travel(u, opp_core->pos);
	}
}

static void handle_striker(t_obj *u, t_obj *core, t_obj *opp_core)
{
	(void)core;

	// 1. If opponent core is destroyed, sweep and destroy remaining enemy units!
	if (!opp_core || opp_core->hp == 0)
	{
		t_obj *opp_unit = ft_get_units_opponent_nearest(u->pos);
		if (opp_unit)
		{
			if (ft_is_adjacent(u->pos, opp_unit->pos))
				core_action_attack(u, opp_unit);
			else
				safe_travel(u, opp_unit->pos);
			core_debug_addObjectInfo(u, "Striker eliminating enemy unit #%lu", opp_unit->id);
		}
		return;
	}

	// 2. Win condition: attack opponent Core if adjacent
	if (ft_is_adjacent(u->pos, opp_core->pos))
	{
		core_action_attack(u, opp_core);
		core_debug_addObjectInfo(u, "Striker battering opponent Core!");
		return;
	}

	// 3. Clear adjacent enemy unit blocking our advance
	t_obj *adj_opp = ft_get_adjacent_enemy(u->pos);
	if (adj_opp)
	{
		core_action_attack(u, adj_opp);
		core_debug_addObjectInfo(u, "Striker slashing adjacent enemy #%lu", adj_opp->id);
		return;
	}

	// 4. March directly on opponent Core!
	safe_travel(u, opp_core->pos);
	core_debug_addObjectInfo(u, "Striker marching on opponent Core (%u, %u)", opp_core->pos.x, opp_core->pos.y);
}

void handle_combat(t_obj *core, t_obj *opp_core, t_obj **units)
{
	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		if (strcmp(u->s_unit.name, "Defender") == 0)
			handle_defender(u, core, opp_core);
		else if (strcmp(u->s_unit.name, "Hunter") == 0)
			handle_hunter(u, core, opp_core);
		else if (strcmp(u->s_unit.name, "Striker") == 0)
			handle_striker(u, core, opp_core);
	}
}
