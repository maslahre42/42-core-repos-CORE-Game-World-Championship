#include "bot.h"

static void handle_single_fighter(t_obj *u, t_obj *core, t_obj *opp_core)
{
	(void)core;
	// 1. Is there an adjacent enemy unit?
	t_obj *adj_opp = ft_get_adjacent_enemy(u->pos);
	if (adj_opp)
	{
		core_action_attack(u, adj_opp);
		core_debug_addObjectInfo(u, "Fighter slashing adjacent enemy unit #%lu", adj_opp->id);
		return;
	}

	// 2. Hunt down closest enemy unit
	t_obj *closest_opp = ft_get_units_opponent_nearest(u->pos);
	if (closest_opp)
	{
		core_action_travel(u, closest_opp->pos, NULL);
		core_debug_addObjectInfo(u, "Fighter hunting enemy unit at (%u, %u)", closest_opp->pos.x, closest_opp->pos.y);
		return;
	}

	// 3. If all enemy units eliminated, assist siege on opponent core
	if (opp_core)
	{
		if (ft_is_adjacent(u->pos, opp_core->pos))
			core_action_attack(u, opp_core);
		else
			core_action_travel(u, opp_core->pos, NULL);
	}
}

static void handle_single_striker(t_obj *u, t_obj *core, t_obj *opp_core)
{
	if (!opp_core)
		return;

	// 1. Win condition: attack opponent Core if adjacent
	if (ft_is_adjacent(u->pos, opp_core->pos))
	{
		core_action_attack(u, opp_core);
		core_debug_addObjectInfo(u, "Battering opponent Core!");
		return;
	}

	// 2. Tactical defense: if adjacent enemy unit is hitting us, hit back
	t_obj *adj_opp = ft_get_adjacent_enemy(u->pos);
	if (adj_opp)
	{
		core_action_attack(u, adj_opp);
		core_debug_addObjectInfo(u, "Eliminating adjacent threat #%lu", adj_opp->id);
		return;
	}

	// 3. Core defense: if our core is under direct siege, intercept attacker if nearby
	if (core && ft_is_core_threatened(core, 4) && ft_distance(u->pos, core->pos) <= 6)
	{
		t_obj *threat = ft_get_threat_nearest(core->pos, 4);
		if (threat)
		{
			if (ft_is_adjacent(u->pos, threat->pos))
				core_action_attack(u, threat);
			else
				core_action_travel(u, threat->pos, NULL);
			core_debug_addObjectInfo(u, "Defending our Core!");
			return;
		}
	}

	// 4. March on opponent Core
	core_action_travel(u, opp_core->pos, NULL);
	core_debug_addObjectInfo(u, "Marching toward opponent Core at (%u, %u)", opp_core->pos.x, opp_core->pos.y);
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

		if (strcmp(u->s_unit.name, "Fighter") == 0)
			handle_single_fighter(u, core, opp_core);
		else if (strcmp(u->s_unit.name, "Striker") == 0)
			handle_single_striker(u, core, opp_core);
	}
}
