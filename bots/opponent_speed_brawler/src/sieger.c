#include "bot.h"

void handle_siegers(t_obj *opp_core, t_obj **units)
{
	if (!opp_core || !units)
		return;

	for (int i = 0; units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, "Sieger") != 0)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		// 1. Melt Core if adjacent
		if (ft_is_adjacent(u->pos, opp_core->pos))
		{
			core_action_attack(u, opp_core);
			continue;
		}

		// 2. Attack any enemy unit adjacent
		t_obj *adj_opp = ft_get_best_adjacent_enemy(u);
		if (adj_opp)
		{
			core_action_attack(u, adj_opp);
			continue;
		}

		// 3. March directly toward opponent Core
		safe_travel(u, opp_core->pos);
	}
}
