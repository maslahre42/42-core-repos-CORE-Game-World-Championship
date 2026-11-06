#ifndef BOT_H
#define BOT_H

#include "core_lib.h"
#include <string.h>

#define UNIT_NAME_CARRIER       "Carrier"
#define UNIT_NAME_MINER         "Miner"
#define UNIT_NAME_HUNTER        "Hunter"
#define UNIT_NAME_FLANK_BREAKER "FlankBreaker"

// Exact costs based on game configuration:
// Carrier:       base (42) + speed (55) + max_gems (40) = 137
// Miner:         base (42) + demolition (35) + max_gems (40) = 117
// Hunter:        base (42) + combat (35) + combat (35) + health (40) = 152
// Flank Breaker: base (42) + combat (35) + siege (35) + health (40) + armor (35) = 187
#define COST_CARRIER        137
#define COST_MINER          117
#define COST_HUNTER         152
#define COST_FLANK_BREAKER  187

// 2D collision-free reservation grid
extern bool g_reserved_tiles[64][64];

// Navigation & Tile Reservation functions
void nav_init_tick(void);
bool nav_is_valid_tile(t_pos pos);
bool nav_is_reserved(t_pos pos);
void nav_reserve_tile(t_pos pos);
bool nav_move_to_exact(const t_obj *unit, t_pos target, bool hug_borders);
bool nav_move_adjacent_to(const t_obj *unit, t_pos target, bool hug_borders);

// Spatial & distance helpers
unsigned int ft_distance(t_pos a, t_pos b);
bool ft_is_adjacent(t_pos a, t_pos b);

// Core getters
t_obj *ft_get_core_own(void);
t_obj *ft_get_core_opponent(void);

// Resource & unit getters
t_obj *ft_get_nearest_gem_pile(t_pos pos);
t_obj *ft_get_deposit_for_miner(const t_obj *miner, const t_obj *core);
t_obj *ft_get_adjacent_carrier(t_pos pos);
t_obj *ft_get_miner_needing_carrier(t_pos pos);
t_obj *ft_get_best_hunter_target(const t_obj *hunter, const t_obj *core);
t_obj *ft_get_adjacent_enemy(t_pos pos);
bool ft_is_core_threatened(t_obj *core, unsigned int radius);

// Module handlers
void handle_spawner(t_obj *core);
void handle_carriers(t_obj *core, t_obj **units);
void handle_miners(t_obj *core, t_obj **units);
void handle_hunters(t_obj *core, t_obj *opp_core, t_obj **units);
void handle_flank_breakers(t_obj *core, t_obj *opp_core, t_obj **units);

#endif /* BOT_H */
