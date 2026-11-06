#include "bot.h"

static bool is_unit_own(const t_obj *obj)
{
	return (obj->type == OBJ_UNIT && obj->s_unit.team_id == game.my_team_id);
}

void ft_on_tick(unsigned long tick)
{
	(void)tick;

	// 1. Initialize collision-free 2D reservation grid for this tick
	nav_init_tick();

	t_obj *core = ft_get_core_own();
	t_obj *opp_core = ft_get_core_opponent();
	t_obj **units = core_get_objs_filter(is_unit_own);

	// 2. Production: Spawner evaluates economy, defense & army
	handle_spawner(core);

	// 3. Economy: On-site Miners mine deposits and transfer gems to adjacent Carriers
	handle_miners(core, units);

	// 4. Logistics: Super-speed Carriers collect natural piles, meet miners & relay to Core
	handle_carriers(core, units);

	// 5. Military - Hunters: Stalk and eliminate enemy units (unarmed carriers first!)
	handle_hunters(core, opp_core, units);

	// 6. Military - Flank Breakers: Hug outer borders to bypass defenders & siege Core
	handle_flank_breakers(core, opp_core, units);

	if (units)
		free(units);
}

int main(int argc, char **argv)
{
	return core_startGame("Bot_Alpha", argc, argv, ft_on_tick, false);
}
