#include "bot.h"

void ft_on_tick(unsigned long tick)
{
	(void)tick;
	// 1. Clear tick-level movement reservations & overkill damage tables
	reservations_init_tick();

	t_obj *core = ft_get_core_own();
	t_obj *opp_core = ft_get_core_opponent();
	t_obj **units = ft_get_units_own();

	// 2. Run Carrier & Miner logistics FIRST so gem transfers to Core
	//    are counted before the Spawner makes creation decisions!
	handle_carrier(core, units);
	handle_miners(core, units);

	// 3. Military Operations: 3-Phase Tactical Combat with First-Strike Ambush
	handle_combat(core, opp_core, units);

	// 4. Spawner decision tree (uses current + newly transferred Core gems)
	handle_spawner(core);

	if (units)
		free(units);
}

int main(int argc, char **argv)
{
	return core_startGame("No_Plan_Just_Prayers", argc, argv, ft_on_tick, false);
}
