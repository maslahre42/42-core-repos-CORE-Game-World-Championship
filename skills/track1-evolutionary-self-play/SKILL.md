---
name: track1-evolutionary-self-play
description: Parameterizes C bot heuristics into inc/tuned_params.h and evolves optimal weights via a multi-archetype Hall-of-Fame gauntlet in scripts/evolve.py.
---

# Track 1: Autonomous Evolutionary Self-Play Skill

## When to Use
Use this skill once the deterministic C state machine compiles cleanly with 0 Action Failures and you have 2+ hours of compute available to autonomously tune economic, spatial scoring, and combat thresholds (`inc/tuned_params.h`).

---

## Core Engineering Rules

### 1. Isolate the Genome (`inc/tuned_params.h`)
Extract all hardcoded magic numbers into macro definitions within `inc/tuned_params.h`:

```c
#ifndef TUNED_PARAMS_H
# define TUNED_PARAMS_H

// Macro-economy thresholds
# define MAX_CARRIERS               1
# define TARGET_MINERS              1
# define CARRIER_EARLY_THRESHOLD    21   // Evolved from 29 -> 21 (early first Miner)
# define CARRIER_HAUL_THRESHOLD     120

// Spatial deposit scoring weights
# define RES_DIST_UNIT_WEIGHT       1
# define RES_DIST_CORE_WEIGHT       2
# define RES_OPEN_CORRIDOR_BONUS    50000
# define RES_WALL_WEIGHT_BASE       60
# define RES_WALL_WEIGHT_HIT_MULT   8

// Tactical combat thresholds
# define CORE_THREAT_RADIUS         6
# define SQUAD_RALLY_SIZE           3    // Evolved from 2 -> 3 (Lanchester local mass)
# define SQUAD_RALLY_MAX_TICKS      5
# define DISTANCE_HOLD_MAX_TICKS    5    // Strictly < 6 to avoid readiness wrap

#endif
```

By concentrating all numerical hyperparameters into a single header, the Python evolutionary driver can mutate values, inject them, and recompile `my-core-bot` without touching core game logic.

---

### 2. Prevent the 3 Classic Self-Play Traps in `scripts/evolve.py`

#### Trap 1: Startup Genome Overwrite
In `setup_environment()`, always inspect `hall_of_fame/best_genome.json` before initializing Gen 0:
- If a saved best genome exists, load it immediately and compile it into `hall_of_fame/champion_best`.
- Never reset evolutionary progress back to default parameters upon script restarts.

#### Trap 2: The 1v1 Vacuum (Overfitting to Self)
Evaluating mutants exclusively against `champion_best` causes circular rock-paper-scissors cycles or degenerate passive play. Always evaluate mutants across a **Multi-Archetype Gauntlet**:
1. **Self-Play Baseline:** `hall_of_fame/champion_best` (evaluates general capability).
2. **Aggressive Rusher:** `bots/opponent_speed_brawler` (tests anti-rush resilience).
3. **Macro Engine:** `bots/gridmaster` or `monir_bot` (tests economic expansion speed).

#### Trap 3: Single-Match Noise & Position Bias
Due to map asymmetry (spawn orientation, mineral cluster proximity), running 1 or 2 matches produces misleading results:
- **Mandatory Side-Swapping:** Every evaluation must play each opponent as both Player 1 and Player 2.
- **Smoothed Re-Baselining:** A candidate replaces `champion_best` only if it achieves a win rate $\ge 60\%$ across the full gauntlet with strictly 0 Action Failures.

---

### 3. Continuous Fitness Scoring Function
Evaluate candidates using a continuous fitness formula rewarding win quality and speed:

$$\text{Fitness} = W \times \left(1000 + \frac{2500 - \text{ticks}}{2.5}\right) - (1 - W) \times \left(500 + \frac{\text{ticks}}{5}\right) - 10000 \times \text{AF}$$

- **Win Term ($W = 1$):** Awards up to 2000 points for fast knockouts.
- **Loss Term ($W = 0$):** Penalizes fast losses more severely than long survival.
- **Action Failure Penalty:** Imposes a catastrophic penalty for any protocol violation, guaranteeing that only 100% compliant bots achieve promotion.
