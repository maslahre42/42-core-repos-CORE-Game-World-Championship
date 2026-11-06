#ifndef BOT_H
#define BOT_H

#include "core_lib.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>

// Dynamic Component Costs (from game.config.json)
#define COST_BASE        42
#define COST_HEALTH      40
#define COST_SPEED       55
#define COST_ARMOR       35
#define COST_COMBAT      35
#define COST_SIEGE       35
#define COST_DEMOLITION  35
#define COST_MAX_GEMS    40

// Unit Archetype Costs
#define COST_MINER    (COST_BASE + COST_DEMOLITION + COST_MAX_GEMS)           // 117
#define COST_COURIER  (COST_BASE + COST_SPEED + COST_MAX_GEMS)              // 137
#define COST_DEFENDER (COST_BASE + COST_COMBAT + COST_HEALTH + COST_ARMOR) // 152
#define COST_PHALANX  (COST_BASE + COST_COMBAT + COST_SIEGE + COST_HEALTH + COST_ARMOR) // 187

// Unit Role Names
#define ROLE_DEFENDER "Defender"
#define ROLE_COURIER  "Courier"
#define ROLE_MINER    "Miner"
#define ROLE_PHALANX  "Phalanx"

#define MAX_GRID_SIZE 64

// 2D Collision Reservation Grid
extern bool g_reserved_tiles[MAX_GRID_SIZE][MAX_GRID_SIZE];

// Reservations API
void reservations_init_tick(void);
void reserve_tile(t_pos pos);
void unreserve_tile(t_pos pos);
bool is_tile_reserved(t_pos pos);
bool is_pos_in_bounds(t_pos pos);
bool is_tile_walkable_for_move(t_pos pos);
bool is_tile_walkable_neighbor(t_pos pos);

// Spatial helpers
unsigned int ft_distance(t_pos a, t_pos b);
bool ft_is_adjacent(t_pos a, t_pos b);

// Pathfinding API
t_pos path_find_step(t_pos start, t_pos target, bool target_adjacent);

// Getters & Spatial Queries
t_obj *ft_get_core_own(void);
t_obj *ft_get_core_opponent(void);
t_pos ft_get_choke_point(int choke_index);
t_obj *ft_get_nearest_gem_pile(t_pos pos);
t_obj *ft_get_best_deposit_for_miner(t_pos miner_pos, unsigned long miner_id);
t_obj *ft_get_adjacent_deposit_or_pile(t_pos pos);
t_obj *ft_get_adjacent_enemy(t_pos pos);
t_obj *ft_get_adjacent_courier(t_pos pos);
t_obj *ft_get_adjacent_miner_with_gems(t_pos pos);
t_obj *ft_get_miner_with_gems_nearest(t_pos pos);
t_obj *ft_get_any_miner_nearest(t_pos pos);
t_obj *ft_get_threat_nearest(t_pos pos, unsigned int max_dist);
bool ft_is_core_threatened(unsigned int radius);
unsigned int count_owned_units_by_role(const char *role);

// Module Handlers
void handle_spawner(t_obj *core);
void handle_defenders(t_obj *core);
void handle_couriers(t_obj *core);
void handle_miners(t_obj *core);
void handle_phalanxes(t_obj *opp_core);

#endif /* BOT_H */
