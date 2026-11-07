#include "bot.h"

static unsigned long g_sticky_highway_id = 0;

static bool miner_evade_threat(t_obj *u, t_obj *core, t_obj **units)
{
	(void)core;
	if (!u || u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
		return false;

	const t_obj *closest_threat = NULL;
	unsigned int min_threat_dist = UINT_MAX;

	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		const t_obj *o = game.objects[i];
		if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id &&
			o->s_unit.properties.damage_unit > 0 && o->hp > 0)
		{
			unsigned int d = ft_distance(u->pos, o->pos);
			if (d < min_threat_dist)
			{
				min_threat_dist = d;
				closest_threat = o;
			}
		}
	}

	if (!closest_threat || min_threat_dist > 3)
		return false;

	if (ft_count_allies_near_pos(closest_threat->pos, 1) > 0)
		return false;

	if (u->s_unit.gems > 0)
	{
		for (int j = 0; units && units[j]; j++)
		{
			t_obj *c = units[j];
			if (c->s_unit.name && strcmp(c->s_unit.name, "Carrier") == 0 &&
				ft_is_adjacent(u->pos, c->pos))
			{
				unsigned long max_g = c->s_unit.properties.max_gems;
				unsigned long curr_g = c->s_unit.gems;
				if (curr_g < max_g)
				{
					unsigned long space = max_g - curr_g;
					unsigned long amt = (u->s_unit.gems < space) ? u->s_unit.gems : space;
					core_action_transferGems(u, c->pos, amt);
					return true;
				}
			}
		}
	}

	const int dx[4] = {1, -1, 0, 0};
	const int dy[4] = {0, 0, 1, -1};
	t_pos best_step = u->pos;
	long best_score = -999999;
	bool found = false;

	for (int d = 0; d < 4; d++)
	{
		int nx = (int)u->pos.x + dx[d];
		int ny = (int)u->pos.y + dy[d];
		if (nx < 0 || ny < 0 || nx >= (int)game.grid_size || ny >= (int)game.grid_size)
			continue;
		t_pos p = {(unsigned short)nx, (unsigned short)ny};
		if (is_tile_occupied(p))
			continue;

		unsigned int step_min_threat = UINT_MAX;
		for (int i = 0; game.objects && game.objects[i]; i++)
		{
			const t_obj *o = game.objects[i];
			if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id &&
				o->s_unit.properties.damage_unit > 0 && o->hp > 0)
			{
				unsigned int ed = ft_distance(p, o->pos);
				if (ed < step_min_threat)
					step_min_threat = ed;
			}
		}

		long score = (long)(step_min_threat * 1000);
		for (int i = 0; game.objects && game.objects[i]; i++)
		{
			const t_obj *o = game.objects[i];
			if (o->type == OBJ_UNIT && o->s_unit.team_id != game.my_team_id &&
				o->s_unit.properties.damage_unit > 0 && o->hp > 0)
			{
				if (ft_distance(p, o->pos) <= 1 && o->s_unit.action_cooldown <= 0)
					score -= 50000;
			}
		}

		if (score > best_score)
		{
			best_score = score;
			best_step = p;
			found = true;
		}
	}

	if (found && (best_step.x != u->pos.x || best_step.y != u->pos.y) && best_score > -40000)
	{
		core_action_move(u, best_step);
		return true;
	}

	return false;
}

void handle_miners(t_obj *core, t_obj **units)
{
	if (!core)
		return;

	unsigned int n_carriers = core_get_units_byName_count("Carrier");
	t_obj *opp_core = ft_get_core_opponent();

	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!u->s_unit.name || strcmp(u->s_unit.name, "Miner") != 0)
			continue;
		if (u->s_unit.action_cooldown > 0 || has_unit_acted(u->id))
			continue;
		if (miner_evade_threat(u, core, units))
			continue;

		unsigned long carried = u->s_unit.gems;

		// 1. If holding any gems and adjacent to Carrier or Core, transfer them immediately
		if (carried > 0)
		{
			if (ft_is_adjacent(u->pos, core->pos))
			{
				core_action_transferGems(u, core->pos, carried);
				continue;
			}
			for (int j = 0; units && units[j]; j++)
			{
				t_obj *c = units[j];
				if (c->s_unit.name && strcmp(c->s_unit.name, "Carrier") == 0 &&
					ft_is_adjacent(u->pos, c->pos) &&
					c->s_unit.gems < c->s_unit.properties.max_gems)
				{
					unsigned long space = c->s_unit.properties.max_gems - c->s_unit.gems;
					unsigned long amt = (carried < space) ? carried : space;
					core_action_transferGems(u, c->pos, amt);
					break;
				}
			}
			if (has_unit_acted(u->id))
				continue;

			// Return to Core if holding gems (from a walled-off pile or solo miner mode)
			if (n_carriers == 0 ? (carried >= 48 || carried >= u->s_unit.properties.max_gems) : (carried > 0))
			{
				safe_travel(u, core->pos);
				continue;
			}
		}

		// 2. Attack adjacent Deposit FIRST
		t_obj *dep = ft_get_deposit_nearest(u->pos);
		if (dep && ft_is_adjacent(u->pos, dep->pos))
		{
			core_action_attack(u, dep);
			continue;
		}

		// 3. BUG #1 FIX: Only attack an adjacent Gem Pile if NO Carrier can collect it!
		t_obj *pile = ft_get_gems_nearest(u->pos);
		if (pile && ft_is_adjacent(u->pos, pile->pos) &&
			u->s_unit.gems < u->s_unit.properties.max_gems &&
			pile->s_deposit_gems_pile.gems > get_pending_pile_gems(pile->id) &&
			(n_carriers == 0 || !ft_carrier_can_reach_pile(pile, core)))
		{
			core_action_attack(u, pile);
			continue;
		}

		// 4. Find assigned deconflicted resource
		t_obj *target = ft_get_resource_for_miner(u->pos, core->pos, u->id);
		if (target)
		{
			if (ft_is_adjacent(u->pos, target->pos))
				core_action_attack(u, target);
			else
				safe_travel(u, target->pos);
			continue;
		}

		// 5. Highway Sapper: clear obstacles blocking the path between Cores
		t_obj *highway_obs = NULL;
		if (g_sticky_highway_id != 0)
		{
			t_obj *st = core_get_obj_from_id(g_sticky_highway_id);
			if (st && st->hp > 0 && (st->type == OBJ_WALL || st->type == OBJ_DEPOSIT))
				highway_obs = st;
			else
				g_sticky_highway_id = 0;
		}
		if (!highway_obs && opp_core)
		{
			highway_obs = nav_find_highway_obstacle(core->pos, opp_core->pos);
			if (highway_obs)
				g_sticky_highway_id = highway_obs->id;
		}
		if (highway_obs)
		{
			if (ft_is_adjacent(u->pos, highway_obs->pos))
				core_action_attack(u, highway_obs);
			else
				safe_travel(u, highway_obs->pos);
			continue;
		}

		if (carried > 0)
		{
			if (ft_is_adjacent(u->pos, core->pos))
				core_action_transferGems(u, core->pos, carried);
			else
				safe_travel(u, core->pos);
			continue;
		}

		if (ft_is_adjacent(u->pos, core->pos))
			step_away_from_core(u, core);
	}
}
