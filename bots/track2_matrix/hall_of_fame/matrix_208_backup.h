#ifndef POLICY_MATRIX_H
#define POLICY_MATRIX_H

#include <stdbool.h>

#define MIN_MATRIX_CONFIDENCE 3
#define ENABLE_MATRIX_TRACE 1

// Roles
#define ROLE_CORE 0
#define ROLE_HUNTER 1
#define ROLE_CARRIER 2
#define ROLE_MINER 3

// Core Spawner Actions (5): RUNNER=0, DIGGER=1, BRAWLER=2, SKIRMISHER=3, FINISHER=4. States: 64.
#define CORE_ACTION_RUNNER 0
#define CORE_ACTION_DIGGER 1
#define CORE_ACTION_BRAWLER 2
#define CORE_ACTION_SKIRMISHER 3
#define CORE_ACTION_FINISHER 4
#define NUM_CORE_ACTIONS 5
#define NUM_CORE_STATES 64

// Hunter Combat Actions (5): SHARED=0, LOWEST_HP=1, WORKER=2, HOLD_D2=3, GUARD_LANE=4. States: 32.
#define HUNTER_ACTION_SHARED 0
#define HUNTER_ACTION_LOWEST_HP 1
#define HUNTER_ACTION_WORKER 2
#define HUNTER_ACTION_HOLD_D2 3
#define HUNTER_ACTION_GUARD_LANE 4
#define NUM_HUNTER_ACTIONS 5
#define NUM_HUNTER_STATES 32

#ifdef MATRIX_DATA_DEFINITION
float g_core_q[NUM_CORE_STATES][NUM_CORE_ACTIONS] = {
	{0.0000f, -0.5904f, -0.3600f, 0.0000f, 0.0000f}, // State 0
	{-0.9960f, 0.0000f, 0.0000f, -0.2000f, 0.0000f}, // State 1
	{0.0000f, -0.2000f, -0.4880f, 0.0000f, 0.0000f}, // State 2
	{-0.2000f, 0.0000f, -0.3600f, 0.0000f, 0.0000f}, // State 3
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 4
	{0.5673f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 5
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 6
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 7
	{0.0000f, -0.5346f, 0.0000f, 0.0000f, -0.5904f}, // State 8
	{-0.2859f, -0.9997f, 0.0000f, 0.0000f, 0.0000f}, // State 9
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 10
	{0.0400f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 11
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 12
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 13
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 14
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 15
	{0.0000f, 0.0000f, -0.8658f, 0.0000f, 0.0000f}, // State 16
	{-0.9998f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 17
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 18
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 19
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 20
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 21
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 22
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 23
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 24
	{-0.0178f, -0.9998f, 0.0000f, 0.0000f, 0.0000f}, // State 25
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 26
	{-0.2000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 27
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 28
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 29
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 30
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 31
	{-0.7902f, -0.9718f, -0.7236f, -0.3600f, 0.0000f}, // State 32
	{-0.9313f, -0.9313f, -0.3644f, 0.0000f, -0.3600f}, // State 33
	{0.0000f, 0.0000f, 0.3600f, 0.0000f, 0.0000f}, // State 34
	{0.0000f, 0.0000f, -0.2233f, -0.8658f, 0.0000f}, // State 35
	{0.0000f, 0.0000f, 0.3913f, 0.0000f, 0.3450f}, // State 36
	{0.0000f, 0.0000f, 1.1045f, 0.0000f, 1.1230f}, // State 37
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 38
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 39
	{0.0000f, -0.1791f, -0.0005f, 0.0000f, 0.0000f}, // State 40
	{0.0000f, 0.2082f, 0.5726f, 0.0000f, 0.0000f}, // State 41
	{0.0000f, 0.2000f, 0.0000f, 0.0000f, 0.0000f}, // State 42
	{0.0000f, -0.2000f, 0.6666f, 0.0000f, 0.0000f}, // State 43
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 44
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 45
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 46
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 47
	{-0.9141f, -0.9077f, -0.0856f, 0.0000f, -0.5904f}, // State 48
	{-0.9450f, -0.9926f, 0.0674f, 0.0000f, -0.5904f}, // State 49
	{0.0000f, 0.0000f, -0.2000f, 0.0000f, 0.0000f}, // State 50
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 51
	{0.0000f, 0.4253f, 0.0000f, 0.0000f, 0.0000f}, // State 52
	{0.0000f, 0.5736f, 0.0000f, 0.0000f, 0.0000f}, // State 53
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 54
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 55
	{0.0000f, -0.1419f, -0.1809f, 0.0000f, 0.0000f}, // State 56
	{0.0000f, 0.1130f, 0.3410f, 0.0000f, 0.0000f}, // State 57
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 58
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 59
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 60
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 61
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 62
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f} // State 63
};

int g_core_visits[NUM_CORE_STATES] = {
	6, 28, 4, 3, 0, 3, 0, 0, 17, 198, 0, 2, 0, 0, 0, 0,
	9, 37, 0, 0, 0, 0, 0, 0, 0, 86, 0, 1, 0, 0, 0, 0,
	37, 87, 2, 32, 10, 67, 0, 0, 93, 93, 1, 5, 0, 0, 0, 0,
	59, 47, 1, 0, 2, 7, 0, 0, 108, 43, 0, 0, 0, 0, 0, 0
};

float g_hunter_q[NUM_HUNTER_STATES][NUM_HUNTER_ACTIONS] = {
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 0
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 1
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 2
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 3
	{0.1486f, -0.3059f, -0.3938f, 0.0000f, 0.0000f}, // State 4
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.7375f}, // State 5
	{-0.0190f, -0.2715f, 0.3554f, 0.0000f, 0.0000f}, // State 6
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 7
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 8
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 9
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 10
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 11
	{-0.1565f, -0.2667f, 0.2243f, 0.0257f, 0.0000f}, // State 12
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.9699f}, // State 13
	{-0.0594f, -0.0115f, -0.4210f, -0.1934f, 0.0000f}, // State 14
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 15
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 16
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 17
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 18
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 19
	{-0.2465f, -0.0806f, -0.9263f, 0.0000f, 0.2000f}, // State 20
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 1.0496f}, // State 21
	{0.5458f, 0.4910f, -0.6875f, 0.0000f, 0.0000f}, // State 22
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 23
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 24
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 25
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 26
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f}, // State 27
	{0.2148f, -0.9365f, -0.2283f, 0.1678f, 0.0000f}, // State 28
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 1.0832f}, // State 29
	{0.3522f, 0.4758f, -0.7557f, 0.2286f, 0.0000f}, // State 30
	{0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f} // State 31
};

int g_hunter_visits[NUM_HUNTER_STATES] = {
	0, 0, 0, 0, 37, 47, 11, 0, 0, 0, 0, 0, 43, 38, 13, 0,
	0, 0, 0, 0, 140, 74, 67, 0, 0, 0, 0, 0, 141, 62, 75, 0
};
#else
extern float g_core_q[NUM_CORE_STATES][NUM_CORE_ACTIONS];
extern int g_core_visits[NUM_CORE_STATES];
extern float g_hunter_q[NUM_HUNTER_STATES][NUM_HUNTER_ACTIONS];
extern int g_hunter_visits[NUM_HUNTER_STATES];
#endif

#endif /* POLICY_MATRIX_H */
