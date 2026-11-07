---
name: track2-tabular-q-matrix
description: Embeds a Bit-Packed State-Bucket Tabular Q-Learning Matrix (policy_matrix.h & matrix_brain.c) with strict legal affordance masking and episode-trace training (train_matrix.py).
---

# Track 2: Bit-Packed Tabular Q-Matrix & Affordance Masking Skill

## When to Use
Use this skill to embed autonomous reinforcement learning decision tables (`g_core_q` and `g_hunter_q`) directly into a high-performance C micro-engine without runtime Python dependencies, neural inference overhead, or Action Failure risk.

---

## Core Engineering Rules

### 1. Compress State Spaces into 5–6 Bit Integers (`matrix_brain.c`)
Never attempt to train Q-learning across continuous coordinates or full board states. Compress tactical context into orthogonal bit-packed integers:

#### A. Core Decision State Space (`64 States: [0..63]`)
```c
int get_core_state_id(void)
{
    int state = 0;
    
    if (g_has_carrier)                        state |= (1 << 5); // Bit 5: Economy established
    if (g_miners_count >= 1)                  state |= (1 << 4); // Bit 4: Mining underway
    
    int army_diff = g_my_combat - g_opp_combat;
    int diff_bucket = (army_diff < -1) ? 0 : (army_diff <= 1) ? 1 : (army_diff <= 3) ? 2 : 3;
    state |= (diff_bucket << 2);                                 // Bits 3-2: Relative army balance
    
    if (g_threat_near_core)                   state |= (1 << 1); // Bit 1: Base under direct attack
    if (g_has_loose_gems)                     state |= (1 << 0); // Bit 0: Ground gems available
    
    return state;
}
```

#### B. Hunter Combat Decision State Space (`32 States: [0..31]`)
```c
int get_hunter_state_id(t_core_object *u, t_core_object *target)
{
    int state = 0;
    
    if (u->s_unit.hp >= 12)                   state |= (1 << 4); // Bit 4: High vs low health
    if (target && ft_distance(u->pos, target->pos) <= 3) 
                                              state |= (1 << 3); // Bit 3: Immediate engagement range
    if (ft_count_allies_near_pos(u->pos, 2) > 0)
                                              state |= (1 << 2); // Bit 2: Lanchester ally support
    if (g_threat_near_core)                   state |= (1 << 1); // Bit 1: Base defense priority
    if (ft_enemy_units_cleared())             state |= (1 << 0); // Bit 0: Opponent army destroyed
    
    return state;
}
```

---

### 2. Strict Legal Affordance Masks (`spawner.c` & `combat.c`)
Reinforcement learning must operate within strict physics-legal affordance masks. Unconstrained Q-argmax will exploit reward artifacts or select illegal actions during live play:

```c
// Construct legal action mask before selecting Hunter action
bool legal_actions[HUNTER_ACTIONS_COUNT] = {false};

legal_actions[HUNTER_ACTION_ATTACK_CORE] = (opp_core != NULL);
legal_actions[HUNTER_ACTION_ATTACK_FIGHTER] = (best_target != NULL);
legal_actions[HUNTER_ACTION_INTERCEPT_WORKER] = (best_worker != NULL && best_target == NULL);
legal_actions[HUNTER_ACTION_HOLD_DEFEND] = true;

// CRITICAL FIX: Never allow GUARD_LANE while armed enemy fighters are alive!
legal_actions[HUNTER_ACTION_GUARD_LANE] = (best_target == NULL && opp_core != NULL);
```

#### Safety Overrides:
1. **Deterministic Ambush Override:** When distance is 2 and `ft_has_open_melee_gap()` is true, force `HUNTER_ACTION_HOLD_DEFEND` deterministically. Never let noisy Q-exploration step into enemy range on cooldown.
2. **Deterministic Finisher Override:** When `ft_enemy_units_cleared()` is true and perimeter is secure, force `CORE_ACTION_FINISHER` deterministically. Mid-game combat Q-values must never starve the game-winning siege unit.
3. **Visit-Confidence Gate:** Only accept a learned Q-action if the state has been visited at least 3 times during training (`visits[state] >= 3`) and provides a positive return over the heuristic baseline:
```c
if (visits[state] < MIN_MATRIX_CONFIDENCE || q_val <= heuristic_q)
    return heuristic_default;
```

---

### 3. Episode-Trace Training with Opening Credit Protection (`train_matrix.py`)
Tabular Q-learning uses Monte Carlo episode traces logged to `stderr` during training matches:

```python
# Terminal reward calculation based on match speed
if winner == our_team:
    terminal_reward = +100.0 + 50.0 * ((2500.0 - match_ticks) / 2500.0)
else:
    terminal_reward = -100.0 + 40.0 * (match_ticks / 2500.0)

# Backward trace replay with Opening Credit Protection
for role, state, action, tick in reversed(episode_trace):
    # If the match was lost after surviving 1000+ ticks,
    # opening economy decisions at tick <= 250 were sound and must NOT be penalized!
    if winner != our_team and match_ticks >= 1000 and tick <= 250:
        continue
    
    q_table[state][action] += alpha * (terminal_reward - q_table[state][action])
```

#### Production Export:
Once training converges, `train_matrix.py` formats the tables into static C arrays inside `inc/policy_matrix.h`:
```c
static const float g_core_q[64][5] = { ... };
static const float g_hunter_q[32][5] = { ... };
```
Always set `#define ENABLE_MATRIX_TRACE 0` before tournament builds to avoid I/O latency.
