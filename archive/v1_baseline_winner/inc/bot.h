#ifndef BOT_H
#define BOT_H

#include "core_lib.h"
#include <string.h>

#define COST_MINER 117      // base (42) + demolition (35) + max_gems (40)
#define COST_FIGHTER 152    // base (42) + combat (35) + combat (35) + health (40)
#define COST_STRIKER 187    // base (42) + combat (35) + siege (35) + health (40) + armor (35)
#define COST_BASIC_STRIKER 152 // base (42) + combat (35) + siege (35) + health (40)

// Spatial & distance helpers
unsigned int ft_distance(t_pos a, t_pos b);
bool ft_is_adjacent(t_pos a, t_pos b);

// Core getters
t_obj *ft_get_core_own(void);
t_obj *ft_get_core_opponent(void);

// Resource getters
t_obj *ft_get_gems_nearest(t_pos pos);
t_obj *ft_get_deposit_nearest(t_pos pos);
t_obj *ft_get_deposit_gems_nearest(t_pos pos);
t_obj *ft_get_best_resource(t_pos unit_pos, t_pos core_pos, unsigned long unit_id);

// Combat getters
t_obj **ft_get_units_own(void);
t_obj **ft_get_units_opponent(void);
t_obj *ft_get_units_opponent_nearest(t_pos pos);
t_obj *ft_get_threat_nearest(t_pos pos, unsigned int max_dist);
t_obj *ft_get_adjacent_enemy(t_pos pos);
bool ft_is_core_threatened(t_obj *core, unsigned int radius);

// Module handlers
void handle_spawner(t_obj *core);
void handle_miners(t_obj *core, t_obj **units);
void handle_combat(t_obj *core, t_obj *opp_core, t_obj **units);

#endif /* BOT_H */
