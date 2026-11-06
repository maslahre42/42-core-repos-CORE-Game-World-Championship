#include "bot.h"

void handle_miners(t_obj *core, t_obj **units)
{
	if (!core)
		return;

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, "Miner") != 0)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		unsigned long carried = u->s_unit.gems;
		unsigned long capacity = u->s_unit.properties.max_gems;

		// 1. Returning home: full, or holding profitable amount (>= 70)
		if (carried >= 70 || (carried > 0 && carried >= capacity))
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, carried);
				core_debug_addObjectInfo(u, "Unloading %lu gems to Core", carried);
			}
			else
			{
				core_action_travel(u, core->pos, NULL);
				core_debug_addObjectInfo(u, "Carrying %lu gems back to Core", carried);
			}
			continue;
		}

		// 2. Resource harvesting
		t_obj *target = ft_get_best_resource(u->pos, core->pos, u->id);
		if (target)
		{
			if (ft_is_adjacent(u->pos, target->pos))
			{
				core_action_attack(u, target);
				core_debug_addObjectInfo(
					u, "Harvesting %s at (%u, %u)",
					(target->type == OBJ_DEPOSIT ? "Deposit" : "GemPile"),
					target->pos.x, target->pos.y);
			}
			else
			{
				core_action_travel(u, target->pos, NULL);
				core_debug_addObjectInfo(
					u, "Traveling to %s at (%u, %u)",
					(target->type == OBJ_DEPOSIT ? "Deposit" : "GemPile"),
					target->pos.x, target->pos.y);
			}
			continue;
		}

		// 3. Fallback: if no resource found but carrying any gems, return them
		if (carried > 0)
		{
			if (ft_is_adjacent(u->pos, core->pos))
				core_action_transferGems(u, core->pos, carried);
			else
				core_action_travel(u, core->pos, NULL);
		}
	}
}
