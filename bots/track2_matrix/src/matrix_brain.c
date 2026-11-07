#define MATRIX_DATA_DEFINITION
#include "bot.h"
#include <stdio.h>
#include <stdlib.h>

int get_core_state_id(t_obj *core)
{
	int has_carrier = (core_get_units_byName_count("Carrier") > 0) ? 1 : 0;
	int miner_bucket = (int)core_get_units_byName_count("Miner");
	int own_combat = (int)ft_count_own_combat_units();
	int opp_combat = (int)ft_count_enemy_combat_units();
	int diff = own_combat - opp_combat;
	int army_diff = (diff < 0) ? 0 : ((diff == 0) ? 1 : 2);
	int threatened = (core && ft_is_core_threatened(core, 6)) ? 1 : 0;
	int has_loose = (core && ft_runner_opportunity(core) > 0) ? 1 : 0;

	int state = (has_carrier * 32)
		+ ((miner_bucket % 2) * 16)
		+ ((army_diff % 2) * 8)
		+ ((army_diff == 2) ? 4 : 0)
		+ (threatened * 2)
		+ has_loose;

	if (state < 0)
		state = 0;
	if (state >= NUM_CORE_STATES)
		state = NUM_CORE_STATES - 1;
	return state;
}

int select_core_action(t_obj *core, const bool legal_actions[5], int heuristic_default)
{
	if (!core)
		return -1;

	int state = get_core_state_id(core);
	int selected = heuristic_default;

#if ENABLE_MATRIX_TRACE
	// Targeted Exploration for Sparse States during training:
	// Only explore if state has < 4 visits, with 25% probability among legal actions
	if (g_core_visits[state] < 4 && (rand() % 4 == 0))
	{
		int legal_count = 0;
		int legal_list[NUM_CORE_ACTIONS];
		for (int a = 0; a < NUM_CORE_ACTIONS; a++)
		{
			if (legal_actions && legal_actions[a])
				legal_list[legal_count++] = a;
		}
		if (legal_count > 0)
			selected = legal_list[rand() % legal_count];
	}
	else
#endif
	if (g_core_visits[state] >= MIN_MATRIX_CONFIDENCE)
	{
		float threshold_q = (heuristic_default >= 0) ? g_core_q[state][heuristic_default] : 0.0f;
		float best_q = threshold_q;
		int best_a = -1;
		for (int a = 0; a < NUM_CORE_ACTIONS; a++)
		{
			if (legal_actions && legal_actions[a] && g_core_q[state][a] > best_q)
			{
				best_q = g_core_q[state][a];
				best_a = a;
			}
		}
		if (best_a != -1)
			selected = best_a;
	}

#if ENABLE_MATRIX_TRACE
	if (selected >= 0)
	{
		fprintf(stderr, "[MTRACE] R=%d S=%d A=%d T=%lu\n",
			ROLE_CORE, state, selected, (unsigned long)game.elapsed_ticks);
	}
#endif

	return selected;
}

int get_hunter_state_id(const t_obj *u, const t_obj *core, const t_obj *nearest_enemy)
{
	int high_hp = (u && u->hp >= 12) ? 1 : 0;
	int dist_bucket = (u && nearest_enemy && ft_distance(u->pos, nearest_enemy->pos) <= 3) ? 1 : 0;
	int ally_near = (u && ft_count_allies_near_pos(u->pos, 2) > 0) ? 1 : 0;
	int core_threat = (core && ft_is_core_threatened((t_obj *)core, 6)) ? 1 : 0;
	int cleared = ft_enemy_units_cleared() ? 1 : 0;

	int state = (high_hp << 4) | (dist_bucket << 3) | (ally_near << 2) | (core_threat << 1) | cleared;
	if (state < 0)
		state = 0;
	if (state >= NUM_HUNTER_STATES)
		state = NUM_HUNTER_STATES - 1;
	return state;
}

int select_hunter_action(const t_obj *u, const t_obj *core, const t_obj *nearest_enemy,
	const bool legal_actions[5], int heuristic_default)
{
	int state = get_hunter_state_id(u, core, nearest_enemy);
	int selected = heuristic_default;

#if ENABLE_MATRIX_TRACE
	// Targeted Exploration for Sparse States during training:
	// Only explore if state has < 4 visits, with 25% probability among legal actions
	if (g_hunter_visits[state] < 4 && (rand() % 4 == 0))
	{
		int legal_count = 0;
		int legal_list[NUM_HUNTER_ACTIONS];
		for (int a = 0; a < NUM_HUNTER_ACTIONS; a++)
		{
			if (legal_actions && legal_actions[a])
				legal_list[legal_count++] = a;
		}
		if (legal_count > 0)
			selected = legal_list[rand() % legal_count];
	}
	else
#endif
	if (g_hunter_visits[state] >= MIN_MATRIX_CONFIDENCE)
	{
		float threshold_q = (heuristic_default >= 0) ? g_hunter_q[state][heuristic_default] : 0.0f;
		float best_q = threshold_q;
		int best_a = -1;
		for (int a = 0; a < NUM_HUNTER_ACTIONS; a++)
		{
			if (legal_actions && legal_actions[a] && g_hunter_q[state][a] > best_q)
			{
				best_q = g_hunter_q[state][a];
				best_a = a;
			}
		}
		if (best_a != -1)
			selected = best_a;
	}

#if ENABLE_MATRIX_TRACE
	if (selected >= 0)
	{
		fprintf(stderr, "[MTRACE] R=%d S=%d A=%d T=%lu\n",
			ROLE_HUNTER, state, selected, (unsigned long)game.elapsed_ticks);
	}
#endif

	return selected;
}
