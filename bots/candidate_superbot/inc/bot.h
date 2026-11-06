#ifndef BOT_H
#define BOT_H

#include "core_lib.h"
#include <string.h>

#define MAX_GRID_SIZE 64

// Unit component costs
#define COST_CARRIER 137       // base (42) + speed (55) + max_gems (40) -> 2-tick cooldown, 166 cap
#define COST_MINER 117         // base (42) + demolition (35) + max_gems (40) -> 3 dmg objects, 166 cap
#define COST_DEFENDER 152      // base (42) + combat (35) + health (40) + armor (35) -> 10 HP, 30% armor
#define COST_HUNTER 152        // base (42) + combat (35) + combat (35) + health (40) -> 10 HP, 6 dmg units
#define COST_STRIKER 187       // base (42) + combat (35) + siege (35) + demolition (35) + health (40) -> breaches walls!
#define COST_HEAVY_STRIKER 222 // base (42) + combat (35) + siege (35) + demolition (35) + health (40) + armor (35)

// Spatial & distance helpers
unsigned int ft_distance(t_pos a, t_pos b);
bool ft_is_adjacent(t_pos a, t_pos b);
bool is_pos_in_bounds(t_pos pos);

// Tile reservations & collision-free navigation
void reservations_init_tick(void);
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
t_obj *ft_get_adjacent_enemy(t_pos pos);
bool ft_is_core_threatened(t_obj *core, unsigned int radius);

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
