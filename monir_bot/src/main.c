#include "bot.h"

static bool is_unit_own(const t_obj *obj)
{
	return obj->type == OBJ_UNIT && obj->s_unit.team_id == game.my_team_id;
}

void ft_on_tick(unsigned long tick)
{
	(void)tick;
	nav_init_tick();
	t_obj *core = ft_get_core_own();
	t_obj *opponent = ft_get_core_opponent();
	t_obj **units = core_get_objs_filter(is_unit_own);
	handle_collectors(core, units);
	handle_miners(core, units);
	nav_yield_for_workers(core, opponent, units);
	handle_hunters(core, opponent, units);
	handle_swords(core, opponent, units);
	handle_flank_breakers(core, opponent, units);
	handle_spawner(core);
	free(units);
}

int main(int argc, char **argv)
{
	return core_startGame("Draft", argc, argv, ft_on_tick, false);
}
