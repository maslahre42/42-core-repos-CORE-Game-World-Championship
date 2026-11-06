#ifndef BOT_H
#define BOT_H

#include "core_lib.h"
#include <string.h>

#define MAX_GRID_SIZE 64
#define MAX_TRACKED_TARGETS 512

// WorkedOnMyMachin unit component costs:
// 1. Irradiated Rogue: ['max_gems', 'gem_speed', 'speed', 'speed']
//    base (42) + max_gems (40) + gem_speed (40) + 2x speed (110) = 232
#define COST_GATHERER 232

// 2. Bloodstained Agent / Miner: ['demolition', 'speed', 'speed']
//    base (42) + demolition (35) + 2x speed (110) = 187
#define COST_MINER 187

// 3. Raging Fugitive / Brawler: ['combat', 'health', 'health', 'health', 'speed', 'speed']
//    base (42) + combat (35) + 3x health (120) + 2x speed (110) = 307
#define COST_BRAWLER 307

// 4. Ashen Scavenger / Rapid Sieger: ['siege', 'speed', 'speed', 'health']
//    base (42) + siege (35) + 2x speed (110) + health (40) = 227
#define COST_SIEGER 227

#define MAX_BRAWLERS 3

// Spatial & distance helpers
unsigned int ft_distance(t_pos a, t_pos b);
bool ft_is_adjacent(t_pos a, t_pos b);
bool is_pos_in_bounds(t_pos pos);

// Tile reservations & collision-free navigation
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

// Entity getters
t_obj **ft_get_units_own(void);
t_obj **ft_get_units_opponent(void);
t_obj *ft_get_units_opponent_nearest(t_pos pos);
t_obj *ft_get_best_adjacent_enemy(const t_obj *attacker);
t_obj *ft_get_nearest_gem_pile(t_pos pos);
t_obj *ft_get_nearest_deposit(t_pos pos);

// Role handlers
void handle_spawner(t_obj *core);
void handle_gatherers(t_obj *core, t_obj **units);
void handle_miners(t_obj **units);
void handle_brawlers(t_obj *opp_core, t_obj **units);
void handle_siegers(t_obj *opp_core, t_obj **units);

#endif /* BOT_H */
