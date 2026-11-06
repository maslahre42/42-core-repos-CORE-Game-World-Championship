#include "bot.h"

void handle_flank_breakers(t_obj *core, t_obj *opp_core, t_obj **units)
{
	if (!core || !opp_core)
		return;
	bool cleared = ft_enemy_units_cleared();
	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, UNIT_NAME_FLANK_BREAKER) != 0)
			continue;
		if (!cleared)
		{
			core_debug_addObjectInfo(u, "WAIT for Sword to clear enemy units");
			continue;
		}
		core_debug_addObjectInfo(u, "FINISH enemy Core #%lu; enemy units cleared", opp_core->id);
		if (u->s_unit.action_cooldown > 0)
			continue;
		if (ft_distance(u->pos, opp_core->pos) == 1)
			core_action_attack(u, opp_core);
		else
			nav_move_adjacent_to(u, opp_core->pos, false);
	}
}
