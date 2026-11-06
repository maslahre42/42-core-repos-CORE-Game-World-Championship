#include "bot.h"

static void ft_on_tick(unsigned long tick)
{
	(void)tick;
	t_obj *core = ft_get_core_own();
	t_obj *opp_core = ft_get_core_opponent();
	t_obj **my_units = ft_get_units_own();

	// 1. Initialize reservation and pending damage grids
	reservations_init_tick();

	// 2. Execute unit actions
	handle_gatherers(core, my_units);
	handle_miners(my_units);
	handle_brawlers(opp_core, my_units);
	handle_siegers(opp_core, my_units);

	// 3. Core spawner
	handle_spawner(core);

	if (my_units)
		free(my_units);
}

int main(int argc, char **argv)
{
	return core_startGame("WorkedOnMyMachin_Clone", argc, argv, ft_on_tick, false);
}
