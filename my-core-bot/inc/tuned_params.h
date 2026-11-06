// Auto-generated evolutionary genome + 3-Phase & First-Strike Tactics
#ifndef TUNED_PARAMS_H
#define TUNED_PARAMS_H

// --- ECONOMY & LOGISTICS WEIGHTS (Gen 20 Evolved) ---
#define MAX_CARRIERS                 2
#define TARGET_MINERS                2
#define MAX_MINERS                   3
#define CARRIER_EARLY_TICKS          205
#define CARRIER_EARLY_THRESHOLD      21
#define CARRIER_MID_THRESHOLD        51
#define CARRIER_MINER_PICKUP_MIN     16
#define MINER_SOLO_RETURN_GEMS       56

// --- RESOURCE SCORING WEIGHTS ---
#define RES_DIST_UNIT_WEIGHT         2
#define RES_DIST_CORE_WEIGHT         3
#define RES_CLAIMED_PENALTY          120
#define RES_PILE_NEAR_RADIUS         5

// --- DEFENSE & COMBAT WEIGHTS ---
#define CORE_THREAT_RADIUS           8
#define DEFENDER_INTERCEPT_RADIUS    6
#define HUNTER_UNARMED_BOUNTY        5774
#define HUNTER_GEM_CARRIER_BOUNTY    983
#define HUNTER_LOW_HP_WEIGHT         45
#define HUNTER_CORE_THREAT_BOUNTY    1482
#define SQUAD_RALLY_SIZE             3
#define SQUAD_RALLY_DIST             5

// --- UNIT DESIGN & ARMY COMPOSITION GENES ---
#define HUNTER_LOADOUT               2
#define STRIKER_LOADOUT              0
#define STRIKER_TO_HUNTER_RATIO      2

// --- 3-PHASE STRATEGY & FIRST-STRIKE MICRO GENES ---
#define PHASE2_RESOURCE_CUTOFF       2
#define DEFENSE_PERIMETER_DIST       4
#define ASSAULT_POWER_RATIO_PCT      130
#define MIN_ASSAULT_ARMY_SIZE        4

#endif /* TUNED_PARAMS_H */
