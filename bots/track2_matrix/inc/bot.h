#ifndef BOT_H
#define BOT_H

#include "core_lib.h"
#include "tuned_params.h"
#include "policy_matrix.h"
#include <string.h>
#include <limits.h>

#define MAX_GRID_SIZE 64
#define MAX_TRACKED_TARGETS 512

typedef enum e_game_phase
{
	PHASE_1_AGGRESSIVE_ECONOMY = 1,
	PHASE_2_TACTICAL_DEFENSE   = 2,
	PHASE_3_ALL_OUT_ASSAULT    = 3
} t_game_phase;

// Unit component costs
#define COST_RUNNER           232 // base (42) + max_gems (40) + gem_speed (40) + 2x speed (110) -> 166 cap, 1cd, 0 cargo slowdown!
#define COST_CARRIER          137 // base (42) + speed (55) + max_gems (40)
#define COST_DIGGER           187 // base (42) + demolition (35) + 2x speed (110) -> 3cd, 3 obj dmg!
#define COST_MINER            187 // Upgraded miner recipe ("demolition", "speed", "speed")
#define COST_MINER_STARTER    172 // base (42) + demolition (35) + speed (55) + max_gems (40) -> 5cd, 166 cap, solo opener
#define COST_MINER_SOLO       117 // base (42) + demolition (35) + max_gems (40)
#define COST_SCOUT            132 // base (42) + combat (35) + speed (55)
#define COST_DEFENDER         152 // base (42) + combat (35) + health (40) + armor (35)
#define COST_HUNTER_TANK      152 // base (42) + combat (35) + combat (35) + health (40) -> 12 HP, 6 DMG
#define COST_HUNTER_SPEED     167 // base (42) + combat (35) + combat (35) + speed (55) -> 7 HP, 6 DMG, fast!
#define COST_BRAWLER_MED      247 // base (42) + 2x combat (70) + 2x health (80) + speed (55) -> 17 HP, 6 DMG, 5cd (5 components)
#define COST_BRAWLER_HEAVY    307 // base (42) + combat (35) + 3x health (120) + 2x speed (110) -> 22 HP, 3 DMG, 3cd (6 components - MAX)
#define COST_SKIRMISHER       227 // base (42) + combat (35) + health (40) + 2x speed (110) -> 10 HP, 3 DMG, 3cd (4 components)
#define COST_FAST_SCOUT       187 // base (42) + combat (35) + 2x speed (110) -> 4 HP, 3 DMG, 3cd (3 components)
#define COST_SWORD            307 // Monir counter-loadout: 22 HP, 3cd, 3 DMG (6 components)
#define COST_FINISHER         187 // base (42) + 2x siege (70) + demolition (35) + health (40) -> 6 core dmg, 3 obj dmg
#define COST_STRIKER_SPEED    167 // base (42) + combat (35) + siege (35) + speed (55)
#define COST_STRIKER_BRUISE   152 // base (42) + combat (35) + siege (35) + health (40)
#define COST_STRIKER          187 // base (42) + combat (35) + siege (35) + demolition (35) + health (40)
#define COST_SPEED_SIEGER     227 // base (42) + siege (35) + 2x speed (110) + health (40) -> 3cd!
#define COST_BREACH_SIEGER    262 // base (42) + siege (35) + demolition (35) + 2x speed (110) + health (40)

// Spatial & distance helpers
unsigned int ft_distance(t_pos a, t_pos b);
bool ft_is_adjacent(t_pos a, t_pos b);
bool is_pos_in_bounds(t_pos pos);
bool is_tile_occupied(t_pos pos);
bool has_free_spawn_tile(t_pos core_pos);
bool has_unit_acted(unsigned long id);
void step_away_from_core(const t_obj *u, t_obj *core);

// Tile reservations, overkill tracking & collision-free navigation
void reservations_init_tick(void);
void record_core_gem_transfer(unsigned long amount);
unsigned long get_pending_core_gems(void);
unsigned long get_pending_damage(unsigned long target_id);
void add_pending_damage(unsigned long target_id, unsigned long dmg);
unsigned long get_pending_pile_gems(unsigned long pile_id);
void add_pending_pile_gems(unsigned long pile_id, unsigned long gems);
unsigned long get_pending_unit_gems(unsigned long unit_id);
void add_pending_unit_gems(unsigned long unit_id, unsigned long gems);
t_tile_travelability safe_travelability(t_pos pos, const t_obj *unit);
void safe_travel(const t_obj *u, t_pos target);
void nav_compute_corridor_distances(t_pos start, unsigned int dist[MAX_GRID_SIZE][MAX_GRID_SIZE], bool is_unarmed, unsigned long ignore_id);
unsigned int nav_approach_distance(t_pos target, const unsigned int dist[MAX_GRID_SIZE][MAX_GRID_SIZE]);
unsigned long ft_runner_opportunity(const t_obj *core);
bool ft_carrier_can_reach_pile(const t_obj *pile, const t_obj *core);
void nav_compute_finisher_lane(t_pos start, t_pos goal, bool lane[MAX_GRID_SIZE][MAX_GRID_SIZE]);
void nav_request_yield(t_pos pos);
void nav_yield_for_workers(t_obj *core, t_obj *enemy_core, t_obj **units);
bool ft_enemy_units_cleared(void);
t_obj *nav_find_highway_obstacle(t_pos start, t_pos goal);
bool nav_is_path_to_core_open(t_pos start, t_pos goal);

// Core getters
t_obj *ft_get_core_own(void);
t_obj *ft_get_core_opponent(void);

// Object & tactical getters
t_obj **ft_get_units_own(void);
t_obj **ft_get_units_opponent(void);
t_obj *ft_get_units_opponent_nearest(t_pos pos);
t_obj *ft_get_threat_nearest(t_pos pos, unsigned int max_dist);
t_obj *ft_get_best_adjacent_enemy(const t_obj *attacker);
bool ft_is_core_threatened(t_obj *core, unsigned int radius);
unsigned int ft_count_active_resources(void);
unsigned int ft_count_safe_resources(t_pos core_pos);
unsigned int ft_count_enemy_combat_units(void);
unsigned int ft_count_own_combat_units(void);
unsigned int ft_count_enemy_speed_warriors(void);
unsigned long ft_get_enemy_max_hp(void);
unsigned int ft_count_walls(void);
t_obj *ft_get_wall_nearest(t_pos pos);
bool ft_is_pos_near_armed_enemy(t_pos pos, unsigned int radius);
bool ft_is_enemy_adjacent_to_pos(t_pos pos);
unsigned int ft_count_allies_near_pos(t_pos pos, unsigned int radius);

// 3-Phase & First-Strike tactical analyzers
t_game_phase ft_get_game_phase(t_obj *core, t_obj *opp_core);
bool ft_has_open_melee_gap(const t_obj *u, const t_obj *enemy);
bool ft_should_hold_for_first_strike(const t_obj *u, const t_obj *target);
bool ft_is_enemy_engaged_by_ally(const t_obj *enemy, unsigned long exclude_unit_id);

// Resource getters with miner deconfliction
t_obj *ft_get_gems_nearest(t_pos pos);
t_obj *ft_get_deposit_nearest(t_pos pos);
t_obj *ft_get_resource_for_miner(t_pos unit_pos, t_pos core_pos, unsigned long unit_id);

// Role handlers
void handle_spawner(t_obj *core);
void handle_carrier(t_obj *core, t_obj **units);
void handle_miners(t_obj *core, t_obj **units);
void handle_combat(t_obj *core, t_obj *opp_core, t_obj **units);

// Matrix Brain: Compressed State-Bucket Q-Matrix with Heuristic Fallback
int get_core_state_id(t_obj *core);
int select_core_action(t_obj *core, const bool legal_actions[5], int heuristic_default);
int get_hunter_state_id(const t_obj *u, const t_obj *core, const t_obj *nearest_enemy);
int select_hunter_action(const t_obj *u, const t_obj *core, const t_obj *nearest_enemy,
	const bool legal_actions[5], int heuristic_default);

#endif /* BOT_H */
