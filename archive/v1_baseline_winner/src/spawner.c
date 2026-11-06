#include "bot.h"

void handle_spawner(t_obj *core)
{
	if (!core || core->s_core.spawn_cooldown > 0)
		return;

	unsigned long gems = core->s_core.gems;
	unsigned int n_miners = core_get_units_byName_count("Miner");
	unsigned int n_strikers = core_get_units_byName_count("Striker");
	unsigned int n_fighters = core_get_units_byName_count("Fighter");
	bool threatened = ft_is_core_threatened(core, 8);

	// Emergency defense: if enemy is near our core, spawn anti-unit defender immediately
	if (threatened && (n_fighters + n_strikers == 0 || ft_is_core_threatened(core, 5)))
	{
		if (gems >= COST_FIGHTER)
		{
			// Fighter: 2x combat (70) + health (40) -> 10 HP, 6 damage to units!
			core_action_createUnit("Fighter", "combat", "combat", "health", NULL);
			return;
		}
		else if (gems >= COST_MINER)
		{
			// Light Fighter: 1x combat (35) + health (40) -> 10 HP, 3 damage
			core_action_createUnit("Fighter", "combat", "health", NULL);
			return;
		}
	}

	// Early economic foundation: ensure at least 2 miners ASAP
	if (n_miners < 2)
	{
		if (gems >= COST_MINER)
		{
			// Miner: demolition (35) + max_gems (40) -> breaks deposits, carries 166 gems
			core_action_createUnit("Miner", "demolition", "max_gems", NULL);
		}
		return;
	}

	// Mid-game economic expansion: 3rd miner if we have at least 1 offensive unit or extra gems
	if (n_miners < 3 && game.elapsed_ticks < 1000 && (n_strikers + n_fighters >= 1))
	{
		if (gems >= COST_MINER + 40) // only spawn if we keep some reserves
		{
			core_action_createUnit("Miner", "demolition", "max_gems", NULL);
			return;
		}
	}

	// Army production: Heavy armored Strikers (demolish units, siege core, high durability)
	if (gems >= COST_STRIKER)
	{
		// Armored Striker: combat (35) + siege (35) + health (40) + armor (35)
		// 10 HP, 30% armor reduction, 3 unit dmg, 3 core dmg
		core_action_createUnit("Striker", "combat", "siege", "health", "armor", NULL);
	}
	else if (gems >= COST_BASIC_STRIKER && game.elapsed_ticks > 500)
	{
		// Fast Striker if waiting for 187 takes too long
		core_action_createUnit("Striker", "combat", "siege", "health", NULL);
	}
}
