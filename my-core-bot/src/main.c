#include "bot.h"

void ft_on_tick(unsigned long tick)
{
	(void)tick;
	reservations_init_tick();

	t_obj *core = ft_get_core_own();
	t_obj *opp_core = ft_get_core_opponent();
	t_obj **units = ft_get_units_own();

	// 1. Run Carrier & Miner logistics first
	handle_carrier(core, units);
	handle_miners(core, units);

	// 2. Yield worker corridors before combat units take standard moves
	nav_yield_for_workers(core, opp_core, units);

	// 3. Military Operations: Hunters, Defenders, and Finisher Strikers
	handle_combat(core, opp_core, units);

	// 4. Spawner decision tree (uses confirmed + same-tick transferred Core gems)
	handle_spawner(core);

	if (units)
		free(units);
}

int main(int argc, char **argv)
{
	return core_startGame("No_Plan_Just_Prayers",
						  argc,
						  argv,
						  ft_on_tick,
						  false);
}
