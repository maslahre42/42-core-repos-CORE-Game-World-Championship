#ifndef BOT_H
#define BOT_H

#include "core_lib.h"
#include "tuned_params.h"
#include <string.h>

#define MAX_GRID_SIZE 64
#define MAX_TRACKED_TARGETS 512

// Unit component costs
#define COST_CARRIER 137          // base (42) + speed (55) + max_gems (40)
#define COST_MINER 117            // base (42) + demolition (35) + max_gems (40)
#define COST_DEFENDER 152         // base (42) + combat (35) + health (40) + armor (35)
#define COST_HUNTER 152           // base (42) + combat (35) + combat (35) + health (40)
#define COST_SPEED_HUNTER 227     // base (42) + combat (35) + speed (55) + speed (55) + health (40) [3-tick, 10 HP, 3 dmg]
#define COST_STRIKER 187          // base (42) + combat (35) + siege (35) + demolition (35) + health (40)
#define COST_HEAVY_STRIKER 222    // base (42) + combat (35) + siege (35) + demolition (35) + health (40) + armor (35)
#define COST_SPEED_STRIKER 227    // base (42) + siege (35) + speed (55) + speed (55) + health (40) [3-tick, 10 HP, 3 siege]
#define COST_BREACH_STRIKER 262   // base (42) + siege (35) + demolition (35) + speed (55) + speed (55) + health (40) [6-tick, 10 HP]

// Spatial & distance helpers
unsigned int ft_distance(t_pos a, t_pos b);
bool ft_is_adjacent(t_pos a, t_pos b);
bool is_pos_in_bounds(t_pos pos);
bool has_free_spawn_tile(t_obj *core);
void step_away_from_core(const t_obj *u, t_obj *core);

// Tile reservations, overkill tracking & collision-free navigation
void reservations_init_tick(void);
void record_core_gem_transfer(unsigned long amount);
unsigned long get_pending_core_gems(void);
unsigned long get_pending_damage(unsigned long target_id);
void add_pending_damage(unsigned long target_id, unsigned long dmg);
t_tile_travelability safe_travelability(t_pos pos, const t_obj *unit);
void safe_travel(const t_obj *u, t_pos target);

// Core getters
t_obj *ft_get_core_own(void);
t_obj *ft_get_core_opponent(void);

// Object getters
t_obj **ft_get_units_own(void);
t_obj **ft_get_units_opponent(void);
t_obj *ft_get_units_opponent_nearest(t_pos pos);
t_obj *ft_get_threat_nearest(t_pos pos, unsigned int max_dist);
t_obj *ft_get_best_adjacent_enemy(const t_obj *attacker);
bool ft_is_core_threatened(t_obj *core, unsigned int radius);
unsigned int ft_count_active_resources(void);
unsigned int ft_count_enemy_combat_units(void);
unsigned int ft_count_enemy_speed_warriors(void);
unsigned int ft_count_walls(void);
t_obj *ft_get_wall_nearest(t_pos pos);

// Resource getters with miner deconfliction
t_obj *ft_get_gems_nearest(t_pos pos);
t_obj *ft_get_deposit_nearest(t_pos pos);
t_obj *ft_get_resource_for_miner(t_pos unit_pos, t_pos core_pos, unsigned long unit_id);

// Role handlers
void handle_spawner(t_obj *core);
void handle_carrier(t_obj *core, t_obj **units);
void handle_miners(t_obj *core, t_obj **units);
void handle_combat(t_obj *core, t_obj *opp_core, t_obj **units);

#endif /* BOT_H */
