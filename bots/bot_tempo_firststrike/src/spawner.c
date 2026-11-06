#include "bot.h"

static void spawn_hunter_variant(unsigned long gems, unsigned int n_hunters, unsigned int enemy_speed_warriors)
{
	// Speed Assassin ("combat", "combat", "speed"): 4 HP, 6 DMG, fast cooldown!
	// Combined with Distance-2 First-Strike Ambush, it one-shots 5-HP enemies before they can swing!
	bool use_speed = false;
	if (HUNTER_LOADOUT == 1)
		use_speed = true;
	else if (HUNTER_LOADOUT == 2 || enemy_speed_warriors > 0)
		use_speed = (n_hunters % 2 == 0);

	if (use_speed)
	{
		if (gems >= COST_HUNTER_SPEED)
			core_action_createUnit("Hunter", "combat", "combat", "speed", NULL);
		else if (gems >= COST_HUNTER_TANK)
			core_action_createUnit("Hunter", "combat", "combat", "health", NULL);
	}
	else
	{
		if (gems >= COST_HUNTER_TANK)
			core_action_createUnit("Hunter", "combat", "combat", "health", NULL);
	}
}

static void spawn_striker_variant(unsigned long gems)
{
	unsigned int walls = ft_count_walls();

	// Priority: 3-Tick Speed Sieger ("siege", "speed", "speed", "health", 227g)
	// Or Demolition Breach Sieger (262g) if walls exist on the map
	if (walls > 0)
	{
		if (gems >= COST_BREACH_SIEGER)
			core_action_createUnit("Striker", "siege", "demolition", "speed", "speed", "health", NULL);
		else if (gems >= COST_STRIKER)
			core_action_createUnit("Striker", "combat", "siege", "demolition", "health", NULL);
		return;
	}

	if (gems >= COST_SPEED_SIEGER)
		core_action_createUnit("Striker", "siege", "speed", "speed", "health", NULL);
	else if (STRIKER_LOADOUT == 0 && gems >= COST_STRIKER_SPEED)
		core_action_createUnit("Striker", "combat", "siege", "speed", NULL);
	else if (gems >= COST_STRIKER_BRUISE)
		core_action_createUnit("Striker", "combat", "siege", "health", NULL);
}

void handle_spawner(t_obj *core)
{
	if (!core || core->s_core.spawn_cooldown > 0)
		return;
	if (!has_free_spawn_tile(core->pos))
		return;

	t_obj *opp_core = ft_get_core_opponent();
	t_game_phase phase = ft_get_game_phase(core, opp_core);

	unsigned long gems = core->s_core.gems + get_pending_core_gems();
	unsigned int n_carriers = core_get_units_byName_count("Carrier");
	unsigned int n_miners = core_get_units_byName_count("Miner");
	unsigned int n_scouts = core_get_units_byName_count("Scout");
	unsigned int n_defenders = core_get_units_byName_count("Defender");
	unsigned int n_hunters = core_get_units_byName_count("Hunter");
	unsigned int n_strikers = core_get_units_byName_count("Striker");
	unsigned int active_res = ft_count_active_resources();
	unsigned int enemy_combat = ft_count_enemy_combat_units();
	unsigned int enemy_speed = ft_count_enemy_speed_warriors();
	bool threatened = ft_is_core_threatened(core, CORE_THREAT_RADIUS);

	// 1. Emergency Core Defense: spawn 6-DMG Hunter or fast Scout immediately if threatened
	if (threatened && (n_defenders + n_hunters + n_scouts < enemy_combat + 1))
	{
		if (gems >= COST_HUNTER_TANK)
			spawn_hunter_variant(gems, n_hunters, enemy_speed);
		else if (gems >= COST_SCOUT)
			core_action_createUnit("Scout", "combat", "speed", NULL);
		return;
	}

	// 2. Phase 1 Economy: High-speed Carriers & Miners (only while resources remain!)
	if (phase == PHASE_1_AGGRESSIVE_ECONOMY && active_res > 0)
	{
		if (n_carriers < 1)
		{
			if (gems >= COST_CARRIER)
				core_action_createUnit("Carrier", "speed", "max_gems", NULL);
			return;
		}
		if (n_miners < 1)
		{
			if (gems >= COST_MINER)
				core_action_createUnit("Miner", "demolition", "max_gems", NULL);
			return;
		}
		// Fast Early Scout/Hunter to harass enemy economy & protect our first Miner
		if (n_scouts + n_hunters < 1)
		{
			if (gems >= COST_HUNTER_SPEED)
				spawn_hunter_variant(gems, n_hunters, enemy_speed);
			else if (gems >= COST_SCOUT)
				core_action_createUnit("Scout", "combat", "speed", NULL);
			return;
		}
		if (n_miners < TARGET_MINERS && active_res >= 2)
		{
			if (gems >= COST_MINER)
				core_action_createUnit("Miner", "demolition", "max_gems", NULL);
			return;
		}
		if (n_carriers < MAX_CARRIERS && active_res >= 4)
		{
			if (gems >= COST_CARRIER)
				core_action_createUnit("Carrier", "speed", "max_gems", NULL);
			return;
		}
	}

	// 3. Defensive Anchor: 1 Defender in Phase 1/3, or 2 Defenders in Phase 2 Standoff
	unsigned int target_defenders = (phase == PHASE_2_TACTICAL_DEFENSE) ? 2 : 1;
	if (n_defenders < target_defenders)
	{
		if (gems >= COST_DEFENDER)
			core_action_createUnit("Defender", "combat", "health", "armor", NULL);
		return;
	}

	// 4. Phase 1 Late Miner Expansion if map is still rich
	if (phase == PHASE_1_AGGRESSIVE_ECONOMY && n_miners < MAX_MINERS && active_res >= 6)
	{
		if (gems >= COST_MINER)
			core_action_createUnit("Miner", "demolition", "max_gems", NULL);
		return;
	}

	// 5. Phase 2 Tactical Defense Buildup: prioritize 6-DMG Hunters to win unit trades & build HP lead
	if (phase == PHASE_2_TACTICAL_DEFENSE)
	{
		if (n_hunters < 4 || n_hunters <= enemy_combat)
		{
			spawn_hunter_variant(gems, n_hunters, enemy_speed);
			return;
		}
	}

	// 6. Maintain Hunter Escort Ratio for Phase 1 & Phase 3
	unsigned int min_hunters = 1 + (enemy_speed / 2) + (n_strikers / STRIKER_TO_HUNTER_RATIO);
	if (min_hunters > 6)
		min_hunters = 6;

	if (n_hunters < min_hunters)
	{
		spawn_hunter_variant(gems, n_hunters, enemy_speed);
		return;
	}

	// 7. Spawn 3-Tick Speed Siegers / Strikers to crush the enemy Core!
	spawn_striker_variant(gems);
}
