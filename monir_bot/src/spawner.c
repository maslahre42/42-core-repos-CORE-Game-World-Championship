#include "bot.h"

static bool g_had_economy;
static bool g_attacked;

static void spawn_sword(void)
{
	/* 22 HP, damage 3 every 3 ticks, rather than a 16-tick glass cannon. */
	core_action_createUnit(UNIT_NAME_SWORD, "combat", "health", "health", "health", "speed", "speed", NULL);
}

void handle_spawner(t_obj *core)
{
	if (!core || core->s_core.spawn_cooldown > 0)
		return;
	bool spawn_open = false;
	const int dx[4] = {1, 0, -1, 0}, dy[4] = {0, 1, 0, -1};
	for (int d = 0; d < 4; d++)
	{
		int x = core->pos.x + dx[d], y = core->pos.y + dy[d];
		if (x < 0 || y < 0 || x >= game.grid_size || y >= game.grid_size)
			continue;
		t_pos p = {(unsigned short)x, (unsigned short)y};
		const t_obj *o = core_get_obj_from_pos(p);
		if ((!o && !nav_is_reserved(p)) || (o && o->type == OBJ_UNIT &&
			o->s_unit.team_id == game.my_team_id && nav_unit_yielded(o)))
			spawn_open = true;
	}
	if (!spawn_open)
		return;
	unsigned long gems = core->s_core.gems;
	unsigned int miners = core_get_units_byName_count(UNIT_NAME_MINER);
	unsigned int runners = core_get_units_byName_count(UNIT_NAME_RUNNER);
	unsigned int swords = core_get_units_byName_count(UNIT_NAME_SWORD);
	unsigned int finishers = core_get_units_byName_count(UNIT_NAME_FLANK_BREAKER);
	if (miners || runners)
		g_had_economy = true;
	if (swords)
		g_attacked = true;
	unsigned long loose = ft_runner_opportunity(core), deposits = 0;
	unsigned int sites = 0;
	bool local_income = false;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *o = game.objects[i];
		if (ft_is_local_resource(o, core))
			local_income = true;
		if (o->type == OBJ_DEPOSIT && (game.elapsed_ticks > 350 ?
			ft_is_harvest_resource(o, core) : ft_is_safe_resource(o, core)))
		{
			deposits += o->s_deposit_gems_pile.gems;
			sites++;
		}
	}
	/* Wait for the fast runner only when corridors lead to useful loose gems. */
	if (!g_had_economy && !miners && !runners && loose > 0 && game.elapsed_ticks <= 350 &&
		!ft_is_core_threatened(core, 6))
	{
		if (gems >= COST_RUNNER)
			core_action_createUnit(UNIT_NAME_RUNNER, "max_gems", "gem_speed", "speed", "speed", NULL);
		return;
	}
	/* An unaffordable emergency fighter must not block economic recovery. */
	if (ft_is_core_threatened(core, 6) && !swords && gems >= COST_SWORD)
	{
		spawn_sword();
		return;
	}
	if (!runners && loose > 0)
	{
		if (gems >= COST_RUNNER)
		{
			core_action_createUnit(UNIT_NAME_RUNNER, "max_gems", "gem_speed", "speed", "speed", NULL);
			return;
		}
		if (game.elapsed_ticks > 350 && gems >= COST_RECOVERY_RUNNER)
		{
			core_action_createUnit(UNIT_NAME_RUNNER, "speed", "speed", NULL);
			return;
		}
	}
	if (!miners && ((sites && (runners || g_had_economy || game.elapsed_ticks > 350 || local_income)) ||
		(!runners && local_income)))
	{
		if (runners && gems >= COST_DIGGER)
			core_action_createUnit(UNIT_NAME_MINER, "demolition", "speed", "speed", NULL);
		else if (!runners && gems >= COST_MINER)
			core_action_createUnit(UNIT_NAME_MINER, "demolition", "max_gems", NULL);
		else if (game.elapsed_ticks > 350 && gems >= 77)
			core_action_createUnit(UNIT_NAME_MINER, "demolition", NULL);
		return;
	}
	if (miners == 1 && runners && g_attacked && sites >= 2 && deposits >= 600 &&
		!ft_is_core_threatened(core, 10))
	{
		if (gems >= COST_DIGGER)
			core_action_createUnit(UNIT_NAME_MINER, "demolition", "speed", "speed", NULL);
		return;
	}
	if (swords && ft_enemy_units_cleared())
	{
		if (!finishers && gems >= COST_FINISHER)
			core_action_createUnit(UNIT_NAME_FLANK_BREAKER, "siege", "siege", "demolition", "health", NULL);
		return;
	}
	if (gems >= COST_SWORD)
		spawn_sword();
}
