---
name: track2-matrix-reinforcement-learning
description: Compressed State-Bucket Q-Matrix Reinforcement Learning architecture embedded in a zero-latency C execution core for 42 CORE.
---

# Track 2: State-Bucket Q-Matrix Reinforcement Learning

## 1. Overview & Architectural Philosophy

Track 2 investigated whether reinforcement learning could autonomously discover high-level macro-economy and tactical combat decisions inside a zero-latency, embedded C micro-execution framework.

Rather than running heavy neural networks (which are prohibited by tournament runtime latency constraints and external dependency rules), Track 2 pioneered a **Compressed State-Bucket Q-Matrix** that compiles directly into static C tables (`float g_core_q[64][5]`, `float g_hunter_q[32][5]`).

```mermaid
graph TD
    subgraph Offline / Training Loop (Python)
        Sim["Simulated Match in Docker\n(scripts/isolated_scrimmage.py)"]
        Trace["Parse [MTRACE] Action Logs\n(R=Role, S=State, A=Action, T=Tick)"]
        Reward["Compute Continuous Speed Reward\n+ Opening Credit Protection"]
        Bellman["Q-Update: Q(s, a) <- Q(s, a) + alpha * (R - Q(s, a))"]
        Export["Generate inc/policy_matrix.h\n(C Array Export)"]
        Compile["Rebuild with gcc -O3"]
        
        Sim --> Trace --> Reward --> Bellman --> Export --> Compile --> Sim
    end

    subgraph Live Game Execution (C Micro-Engine)
        StateBit["get_core_state_id() / get_hunter_state_id()\n(Bitwise flags: 6-bit / 5-bit)"]
        QTable["Direct Array Indexing: g_core_q[s][a]\n(0 runtime overhead, O(1))"]
        Mask["Apply Legal Action Mask\n(Physics safety & budget checks)"]
        Action["Execute Argmax Q Action\n(Or heuristic fallback if unvisited)"]
        
        StateBit --> QTable --> Mask --> Action
    end
```

---

## 2. Orthogonal Bit-Bucket State Compression

To ensure tractable exploration and rapid convergence without neural approximation, the continuous game state was compressed into small, orthogonal bit-vectors.

### 2.1 Core Spawner State ($S_{\text{core}} \in [0..63]$, 6 Bits)

| Bit | Weight | Name | Condition | Significance |
| :---: | :---: | :--- | :--- | :--- |
| **Bit 5** | 32 | `has_carrier` | `n_carriers >= 1` | Basic collection infrastructure exists |
| **Bit 4** | 16 | `has_miner` | `n_miners >= 1` | Primary extraction infrastructure exists |
| **Bit 3** | 8 | `army_lead` | `n_own_combat > n_opp_combat` | Army size parity / advantage |
| **Bit 2** | 4 | `army_ahead_2`| `n_own_combat >= n_opp_combat + 2` | Decisive military dominance |
| **Bit 1** | 2 | `threatened` | Enemy armed unit within 6 tiles of Core | Immediate base defense required |
| **Bit 0** | 1 | `has_loose` | Harvestable loose gems $\ge 1$ within 4 tiles | Free resource pickup available |

$$\text{State}_{\text{core}} = \sum_{i=0}^5 \text{Bit}_i \times 2^i \quad \in [0, 63]$$

#### Core Actions (5):
`0: RUNNER`, `1: DIGGER`, `2: BRAWLER`, `3: SKIRMISHER`, `4: FINISHER`

---

### 2.2 Hunter Combat State ($S_{\text{hunter}} \in [0..31]$, 5 Bits)

| Bit | Weight | Name | Condition | Significance |
| :---: | :---: | :--- | :--- | :--- |
| **Bit 4** | 16 | `high_hp` | `unit->hp > 10` | High survivability vs squishy threshold |
| **Bit 3** | 8 | `in_range` | Nearest enemy within distance $\le 3$ | Immediate tactical threat zone |
| **Bit 2** | 4 | `ally_near` | Allied Hunter within distance $\le 2$ | Lanchester 2v1 gang-up opportunity |
| **Bit 1** | 2 | `worker_near`| Enemy unarmed worker within distance $\le 4$ | Economy assassination target |
| **Bit 0** | 1 | `cleared` | `enemy_combat_units == 0` | Board cleared; transition to siege |

$$\text{State}_{\text{hunter}} = \sum_{i=0}^4 \text{Bit}_i \times 2^i \quad \in [0, 31]$$

#### Hunter Actions (5):
`0: SHARED_FOCUS`, `1: LOWEST_HP`, `2: WORKER_HUNT`, `3: HOLD_D2`, `4: GUARD_LANE`

---

## 3. Reward Shaping & Credit Assignment

### 3.1 Continuous Speed-Weighted Terminal Return
To incentivize rapid game closure and penalize drawn-out attrition, terminal returns are scaled continuously by match duration:

$$R = \begin{cases} 
+100 + \left\lfloor 50 \times \frac{\max(0, 2500 - \text{ticks})}{2500} \right\rfloor & \text{if Victory} \\ 
-100 + \left\lfloor 40 \times \frac{\min(2500, \text{ticks})}{2500} \right\rfloor & \text{if Defeat} \\ 
0 & \text{if Draw} 
\end{cases}$$

- A lightning victory at tick 1,000 yields $R = +130.0$.
- A grinding victory at tick 2,500 yields $R = +100.0$.
- A quick loss at tick 800 yields $R = -87.2$.
- A prolonged survival loss at tick 2,500 yields $R = -60.0$.

### 3.2 Opening Credit Protection
In standard Monte Carlo Q-learning without discounting, a match lost at tick 1,500 would penalize all decisions taken since tick 0—including the opening build order that successfully established economy.

**The Solution:**
```python
# If match is a defeat but lasted >= 1,000 ticks, do NOT penalize T <= 250
skip_opening_penalty = (opp_won and match_res.get("ticks", 0) >= 1000)
for s, a, t in events:
    if skip_opening_penalty and t <= 250:
        continue  # Protect opening book from late-game corruption!
```

---

## 4. Multi-Archetype Training Gauntlet

Training exclusively against a self-play mirror causes catastrophic overfitting to a single tempo. Track 2 rotates through 3 distinct opponent archetypes in each training batch:

1. **`Champion v5` (`my-core-bot/bot`):** Ruthless 307g Heavy Brawler deathball and Lanchester holds.
2. **`monir_bot/bot` (`Draft`):** Flank breaker with heavy 30-DMG units and perimeter posts.
3. **`opponent_speed_brawler`:** Hyper-aggressive early scout/warrior rushes.

---

## 5. Machine Learning Analysis: The "Skirmisher Trap"

During the final 90-match retraining run, Track 2 converged cleanly with 0 Action Failures, yet lost 19–1 against Champion v5. The forensic diagnosis provides a profound reinforcement learning lesson:

```
[TRAINING GAUNTLET]
Opponents: [Champion v5 (33%), Monir Bot (33%), Speed Brawler (33%)]
                              │
               ┌──────────────┴──────────────┐
               ▼                             ▼
       Against Squishy Bots          Against Champion v5
    (Monir / Speed Brawler)          (Pure 307g Brawlers)
               │                             │
    Cheap 167g Skirmishers         22 HP Heavy Brawlers
     win in 700-900 ticks!         demolish 10 HP Skirmishers
               │                   via Lanchester Square Law!
      Yields High Reward                     │
        (R = +135.0)                         │
               │                             │
               └──────────────┬──────────────┘
                              ▼
            Q-Matrix heavily favors SKIRMISHER:
            State 36: Q(SKIRMISHER) = +129.51
            State 36: Q(BRAWLER)    = +45.20
                              ▼
        Result: Bot is optimized for the average opponent,
        but under-indexed on the apex mirror archetype!
```

### The Takeaway for Future Systems:
To prevent the Skirmisher Trap, the state space must include an **Opponent Combat Archetype bit** (e.g. `opp_heavy_mass >= 1`), allowing the Q-matrix to select Skirmishers against squishy rushers, while forcing Heavy Brawlers against deathball opponents.
