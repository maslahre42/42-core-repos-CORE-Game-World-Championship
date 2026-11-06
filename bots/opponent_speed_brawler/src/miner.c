#include "bot.h"

void handle_miners(t_obj **units)
{
	if (!units)
		return;

	for (int i = 0; units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, "Miner") != 0)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		t_obj *dep = ft_get_nearest_deposit(u->pos);
		if (dep)
		{
			if (ft_is_adjacent(u->pos, dep->pos))
			{
				core_action_attack(u, dep);
			}
			else
			{
				safe_travel(u, dep->pos);
			}
			continue;
		}

		// If no deposits remain, drill walls or clear obstacles
		t_obj *adj_enemy = ft_get_best_adjacent_enemy(u);
		if (adj_enemy)
			core_action_attack(u, adj_enemy);
	}
}
