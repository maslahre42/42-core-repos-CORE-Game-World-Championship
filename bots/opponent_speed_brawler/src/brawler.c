#include "bot.h"

void handle_brawlers(t_obj *opp_core, t_obj **units)
{
	if (!units)
		return;

	for (int i = 0; units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, "Brawler") != 0)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		// 1. Attack adjacent enemy unit immediately
		t_obj *adj_opp = ft_get_best_adjacent_enemy(u);
		if (adj_opp)
		{
			core_action_attack(u, adj_opp);
			continue;
		}

		// 2. Hunt nearest enemy unit across the board
		t_obj *target = ft_get_units_opponent_nearest(u->pos);
		if (target)
		{
			safe_travel(u, target->pos);
			continue;
		}

		// 3. If all enemy units eliminated, assault opponent Core
		if (opp_core)
		{
			if (ft_is_adjacent(u->pos, opp_core->pos))
				core_action_attack(u, opp_core);
			else
				safe_travel(u, opp_core->pos);
		}
	}
}
