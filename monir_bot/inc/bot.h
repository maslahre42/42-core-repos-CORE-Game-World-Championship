#ifndef BOT_H
#define BOT_H
#include "core_lib.h"
#include <string.h>
#define UNIT_NAME_SWORD "Sword"
#define COST_SWORD 307UL
#define UNIT_NAME_RUNNER "Gem Runner"
#define COST_RUNNER 232UL
#define COST_RECOVERY_RUNNER 152UL
#define COST_DIGGER 187UL
unsigned long ft_runner_opportunity(const t_obj *core);
void handle_collectors(t_obj *core, t_obj **units);
bool ft_runner_can_collect(const t_obj *pile, const t_obj *core);
bool ft_is_harvest_resource(const t_obj *resource, const t_obj *core);
bool nav_can_reach(const t_obj *unit, t_pos target, bool adjacent, bool allow_friendly);
void nav_yield_for_workers(const t_obj *core, const t_obj *enemy_core, t_obj **units);
bool nav_unit_yielded(const t_obj *unit);
#define COST_FINISHER 187UL
bool ft_enemy_units_cleared(void);
void handle_swords(t_obj *core, t_obj *opp_core, t_obj **units);
#define UNIT_NAME_CARRIER "Carrier"
#define UNIT_NAME_MINER "Miner"
#define UNIT_NAME_HUNTER "Hunter"
#define UNIT_NAME_FLANK_BREAKER "Flank Breaker"
/* Costs verified against configs/game.config.json. */
#define COST_CARRIER 137UL
#define COST_MINER 117UL
#define COST_HUNTER 152UL
#define COST_FLANK_BREAKER 222UL
#define COST_ASSAULT 187UL
#define COST_ARMORED_ASSAULT 257UL
#define COST_ARMORED_HUNTER 187UL
bool ft_is_safe_resource(const t_obj *resource, const t_obj *core);
#define LOCAL_RESOURCE_RADIUS 10U
bool ft_is_local_resource(const t_obj *resource, const t_obj *core);
extern bool g_reserved_tiles[64][64];
unsigned int ft_distance(t_pos a, t_pos b);
bool ft_is_adjacent(t_pos a, t_pos b);
t_obj *ft_get_core_own(void);
t_obj *ft_get_core_opponent(void);
t_obj *ft_get_nearest_gem_pile(t_pos pos);
t_obj *ft_get_deposit_for_miner(const t_obj *miner, const t_obj *core);
t_obj *ft_get_adjacent_carrier(t_pos pos);
t_obj *ft_get_miner_needing_carrier(t_pos pos);
t_obj *ft_get_best_hunter_target(const t_obj *hunter, const t_obj *core);
t_obj *ft_get_adjacent_enemy(t_pos pos);
bool ft_is_core_threatened(t_obj *core, unsigned int radius);
bool nav_guard_lane(const t_obj *core, const t_obj *enemy_core, t_obj **units, bool lane[64][64]);
void nav_init_tick(void);
bool nav_is_valid_tile(t_pos pos);
bool nav_is_reserved(t_pos pos);
void nav_reserve_tile(t_pos pos);
bool nav_move_adjacent_to(const t_obj *unit, t_pos target, bool hug_borders);
bool nav_move_to_exact(const t_obj *unit, t_pos target, bool hug_borders);
void handle_spawner(t_obj *core);
void handle_miners(t_obj *core, t_obj **units);
void handle_carriers(t_obj *core, t_obj **units);
void handle_hunters(t_obj *core, t_obj *opp_core, t_obj **units);
void handle_flank_breakers(t_obj *core, t_obj *opp_core, t_obj **units);
#endif
