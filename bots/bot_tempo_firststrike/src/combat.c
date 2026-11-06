#include "bot.h"

// Computes a non-blocking defensive perimeter slot around our Core (distance 3-4)
static t_pos get_defensive_perimeter_slot(const t_obj *u, const t_obj *core)
{
	t_pos slot = core->pos;
	int dist = DEFENSE_PERIMETER_DIST;
	int offset = (int)(u->id % 4);

	if (core->pos.x < game.grid_size / 2)
	{
		slot.x = (unsigned short)(core->pos.x + (unsigned short)(dist - offset));
		slot.y = (unsigned short)(core->pos.y + (unsigned short)(offset + 1));
	}
	else
	{
		slot.x = (core->pos.x >= (unsigned short)(dist - offset))
			? (unsigned short)(core->pos.x - (unsigned short)(dist - offset)) : 0;
		slot.y = (core->pos.y >= (unsigned short)(offset + 1))
			? (unsigned short)(core->pos.y - (unsigned short)(offset + 1)) : 0;
	}

	if (!is_pos_in_bounds(slot))
		return core->pos;
	return slot;
}

static void handle_defender(t_obj *u, t_obj *core, t_obj *opp_core, t_game_phase phase)
{
	if (!core)
		return;

	// 1. Stage 3 Attack: strike best adjacent enemy immediately
	t_obj *adj_opp = ft_get_best_adjacent_enemy(u);
	if (adj_opp)
	{
		core_action_attack(u, adj_opp);
		return;
	}

	// 2. Intercept any enemy within intercept radius of our Core (using First-Strike Ambush!)
	t_obj *threat = ft_get_threat_nearest(core->pos, DEFENDER_INTERCEPT_RADIUS);
	if (threat)
	{
		if (ft_is_adjacent(u->pos, threat->pos))
			core_action_attack(u, threat);
		else if (!ft_should_hold_for_first_strike(u, threat))
			safe_travel(u, threat->pos);
		return;
	}

	// 3. In Phase 3 All-Out Assault (or if enemy Core is dead), push forward with the army!
	if (phase == PHASE_3_ALL_OUT_ASSAULT || !opp_core || opp_core->hp == 0)
	{
		t_obj *opp = ft_get_units_opponent_nearest(u->pos);
		if (opp)
		{
			if (ft_is_adjacent(u->pos, opp->pos))
				core_action_attack(u, opp);
			else if (!ft_should_hold_for_first_strike(u, opp))
				safe_travel(u, opp->pos);
			return;
		}
	}

	// 4. Guard Core Choke Point (2 tiles from Core, never blocking distance-1 spawn pads)
	t_pos choke1 = core->pos;
	t_pos choke2 = core->pos;
	if (core->pos.x == 0 && core->pos.y == 0)
	{
		choke1.x = 2; choke1.y = 0;
		choke2.x = 0; choke2.y = 2;
	}
	else
	{
		choke1.x = (core->pos.x >= 2) ? core->pos.x - 2 : 0;
		choke1.y = core->pos.y;
		choke2.x = core->pos.x;
		choke2.y = (core->pos.y >= 2) ? core->pos.y - 2 : 0;
	}

	t_pos choke = (u->id % 2 == 0) ? choke1 : choke2;
	t_obj *occ = core_get_obj_from_pos(choke);
	if (occ && occ->id != u->id)
		choke = (u->id % 2 == 0) ? choke2 : choke1;

	if (ft_distance(u->pos, choke) > 0)
		safe_travel(u, choke);
}

static void handle_hunter_or_scout(t_obj *u, t_obj *core, t_obj *opp_core, t_game_phase phase)
{
	// 1. Stage 3 Attack: always strike adjacent enemy immediately (executes before enemy move!)
	t_obj *adj_opp = ft_get_best_adjacent_enemy(u);
	if (adj_opp)
	{
		core_action_attack(u, adj_opp);
		return;
	}

	// 2. Phase 2 Tactical Defense Mode:
	// Hold defensive perimeter around our Core and only engage enemies entering our side of the map
	// (or snipe unarmed enemy Carriers/Miners that wander close)
	t_obj **opps = ft_get_units_opponent();
	t_obj *best_target = NULL;
	long best_priority = -999999;

	for (int i = 0; opps && opps[i]; i++)
	{
		t_obj *e = opps[i];
		if (get_pending_damage(e->id) >= e->hp)
			continue;

		unsigned int dist_me = ft_distance(u->pos, e->pos);
		unsigned int dist_core = core ? ft_distance(core->pos, e->pos) : 20;
		bool is_unarmed = (e->s_unit.properties.damage_unit == 0 && e->s_unit.properties.damage_core == 0);

		// In Phase 2 Defense Mode, ignore armed enemies sitting far away in their own base;
		// let them walk into our defensive perimeter!
		if (phase == PHASE_2_TACTICAL_DEFENSE && !is_unarmed &&
			dist_core > CORE_THREAT_RADIUS + 3 && dist_me > 4)
		{
			continue;
		}

		long priority = 1000 - (long)(dist_me * 18);
		priority -= (long)(e->hp * HUNTER_LOW_HP_WEIGHT);

		// Bonus if we can one-shot this enemy unit in a single blow!
		if (u->s_unit.properties.damage_unit >= e->hp)
			priority += 900;

		// High bounty on unarmed enemy Carriers & Miners
		if (is_unarmed)
			priority += HUNTER_UNARMED_BOUNTY;

		// Bounty on enemies carrying gems
		if (e->s_unit.gems > 0)
			priority += HUNTER_GEM_CARRIER_BOUNTY;

		// Urgent bounty on enemies threatening our Core
		if (core && dist_core <= CORE_THREAT_RADIUS)
			priority += HUNTER_CORE_THREAT_BOUNTY;

		// Escort Breach Bonus: clear enemy units corking the opponent's Core for our Speed Siegers!
		if (opp_core && core_get_units_byName_count("Striker") > 0 &&
			ft_distance(e->pos, opp_core->pos) <= 2)
		{
			priority += 2500;
		}

		if (priority > best_priority)
		{
			best_priority = priority;
			best_target = e;
		}
	}
	free(opps);

	if (best_target)
	{
		if (ft_is_adjacent(u->pos, best_target->pos))
		{
			core_action_attack(u, best_target);
		}
		else if (ft_should_hold_for_first_strike(u, best_target))
		{
			// FIRST-STRIKE COOLDOWN AMBUSH:
			// We are at distance 2 with cooldown <= 0 and the armed enemy is about to step to distance 1.
			// Hold position (unless standing on our Core's spawn pad) so they step in and take the first hit!
			step_away_from_core(u, core);
		}
		else
		{
			safe_travel(u, best_target->pos);
		}
		return;
	}

	// 3. In Phase 2 Tactical Defense with no nearby targets, hold the defensive perimeter arc!
	if (phase == PHASE_2_TACTICAL_DEFENSE && core)
	{
		t_pos def_slot = get_defensive_perimeter_slot(u, core);
		if (ft_distance(u->pos, def_slot) > 1)
			safe_travel(u, def_slot);
		else
			step_away_from_core(u, core);
		return;
	}

	// 4. Fallback: march toward opponent Core to escort Siegers (without corking enemy Core slots)
	if (opp_core)
	{
		if (u->s_unit.properties.damage_core > 0 && ft_is_adjacent(u->pos, opp_core->pos))
			core_action_attack(u, opp_core);
		else if (ft_distance(u->pos, opp_core->pos) > 2)
			safe_travel(u, opp_core->pos);
		else
			step_away_from_core(u, opp_core);
	}
}

static void handle_striker(t_obj *u, t_obj *core, t_obj *opp_core, unsigned int n_strikers, t_game_phase phase)
{
	if (!opp_core || opp_core->hp == 0)
	{
		t_obj *opp_unit = ft_get_units_opponent_nearest(u->pos);
		if (opp_unit && u->s_unit.properties.damage_unit > 0)
		{
			if (ft_is_adjacent(u->pos, opp_unit->pos))
				core_action_attack(u, opp_unit);
			else
				safe_travel(u, opp_unit->pos);
		}
		return;
	}

	// 1. Win Condition: attack opponent Core immediately if adjacent!
	if (ft_is_adjacent(u->pos, opp_core->pos))
	{
		core_action_attack(u, opp_core);
		return;
	}

	// 2. If this Striker has unit damage, clear adjacent enemy blocking our advance
	if (u->s_unit.properties.damage_unit > 0)
	{
		t_obj *adj_opp = ft_get_best_adjacent_enemy(u);
		if (adj_opp)
		{
			core_action_attack(u, adj_opp);
			return;
		}
	}

	// 3. In Phase 2 Tactical Defense (or while rallying a squad < SQUAD_RALLY_SIZE),
	// stage safely at our defensive perimeter until Phase 3 All-Out Assault triggers!
	bool should_rally = (n_strikers < SQUAD_RALLY_SIZE &&
		core && ft_distance(u->pos, core->pos) <= SQUAD_RALLY_DIST);

	if ((phase == PHASE_2_TACTICAL_DEFENSE || should_rally) && core &&
		ft_distance(u->pos, opp_core->pos) > 6)
	{
		t_pos rally_slot = get_defensive_perimeter_slot(u, core);
		if (ft_distance(u->pos, rally_slot) > 1)
			safe_travel(u, rally_slot);
		else
			step_away_from_core(u, core);
		return;
	}

	// 4. Phase 3 All-Out Blitz (or Squad Ready): sprint directly to opponent Core!
	safe_travel(u, opp_core->pos);
}

void handle_combat(t_obj *core, t_obj *opp_core, t_obj **units)
{
	unsigned int n_strikers = core_get_units_byName_count("Striker");
	t_game_phase phase = ft_get_game_phase(core, opp_core);

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || u->s_unit.action_cooldown > 0)
			continue;

		if (strcmp(u->s_unit.name, "Defender") == 0)
			handle_defender(u, core, opp_core, phase);
		else if (strcmp(u->s_unit.name, "Hunter") == 0 || strcmp(u->s_unit.name, "Scout") == 0)
			handle_hunter_or_scout(u, core, opp_core, phase);
		else if (strcmp(u->s_unit.name, "Striker") == 0)
			handle_striker(u, core, opp_core, n_strikers, phase);
	}
}
