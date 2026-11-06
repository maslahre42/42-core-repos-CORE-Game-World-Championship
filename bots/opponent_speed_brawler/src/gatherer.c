#include "bot.h"

void handle_gatherers(t_obj *core, t_obj **units)
{
	if (!core || !units)
		return;

	for (int i = 0; units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, "Gatherer") != 0)
			continue;
		if (u->s_unit.action_cooldown > 0)
			continue;

		t_obj *nearest_pile = ft_get_nearest_gem_pile(u->pos);

		// If carrying substantial gems (>= 100) or full or no piles remain on map
		if (u->s_unit.gems >= 100 || (u->s_unit.gems > 0 && !nearest_pile))
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, u->s_unit.gems);
			}
			else
			{
				safe_travel(u, core->pos);
			}
			continue;
		}

		// Otherwise, aggressively gather natural Gem Piles across the map
		if (nearest_pile)
		{
			if (ft_is_adjacent(u->pos, nearest_pile->pos))
			{
				core_action_attack(u, nearest_pile);
			}
			else
			{
				safe_travel(u, nearest_pile->pos);
			}
			continue;
		}

		// Fallback: return to base if carrying any gems
		if (u->s_unit.gems > 0)
		{
			if (ft_is_adjacent(u->pos, core->pos))
				core_action_transferGems(u, core->pos, u->s_unit.gems);
			else
				safe_travel(u, core->pos);
		}
	}
}
