#include "bot.h"

void ft_on_tick(unsigned long tick)
{
	(void)tick;
	t_obj *core = ft_get_core_own();
	t_obj *opp_core = ft_get_core_opponent();

	// 1. Maintain 2D reservation grid so friendly units never collide
	reservations_init_tick();

	// 2. Production: economy vs defense vs armored phalanxes
	handle_spawner(core);

	// 3. Economy: Stationary miners drilling deposits and transferring to couriers
	handle_miners(core);

	// 4. Logistics: High-speed couriers shuttling gems from miners to core
	handle_couriers(core);

	// 5. Citadel Choke Defense: Defenders anchored at choke points
	handle_defenders(core);

	// 6. Military: Armored Phalanxes marching to demolish opponent core
	handle_phalanxes(opp_core);
}

int main(int argc, char **argv)
{
	return core_startGame("Bot_Beta", argc, argv, ft_on_tick, false);
}
