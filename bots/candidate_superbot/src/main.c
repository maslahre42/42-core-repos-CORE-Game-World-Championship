#include "bot.h"

void ft_on_tick(unsigned long tick)
{
	(void)tick;
	// 1. Clear tick-level movement reservations
	reservations_init_tick();

	t_obj *core = ft_get_core_own();
	t_obj *opp_core = ft_get_core_opponent();
	t_obj **units = ft_get_units_own();

	// 2. Spawner decision tree
	handle_spawner(core);

	// 3. High-speed Carrier Logistics (2-tick cooldown relay)
	handle_carrier(core, units);

	// 4. On-Site Harvesters (Deposit mining + carrier transfer)
	handle_miners(core, units);

	// 5. Military Operations: Defenders, Unit Hunters, and Armored Strikers
	handle_combat(core, opp_core, units);

	if (units)
		free(units);
}

int main(int argc, char **argv)
{
	return core_startGame("Candidate_Superbot", argc, argv, ft_on_tick, false);
}
