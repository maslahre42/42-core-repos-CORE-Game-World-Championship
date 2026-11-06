#include "bot.h"

void ft_on_tick(unsigned long tick)
{
	(void)tick;
	t_obj *core = ft_get_core_own();
	t_obj *opp_core = ft_get_core_opponent();
	t_obj **units = ft_get_units_own();

	// 1. Spawner decision: Economy vs Emergency Defense vs Army
	handle_spawner(core);

	// 2. Economy: Harvesters mining deposits and carrying wealth home
	handle_miners(core, units);

	// 3. Military: Anti-unit Fighters and Armored Siege Strikers
	handle_combat(core, opp_core, units);

	free(units);
}

int main(int argc, char **argv)
{
	return core_startGame("No_Plan_Just_Prayers", argc, argv, ft_on_tick, false);
}
