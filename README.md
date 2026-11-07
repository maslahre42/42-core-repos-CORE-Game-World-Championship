<p align="center">
  <img src="https://avatars.githubusercontent.com/u/147154505?s=200&v=4" alt="CORE Logo" width="130">
</p>

# 🏆 No_Plan_Just_Prayers — 42 CORE Game World Championship

[![Tournament Rank](https://img.shields.io/badge/Swiss%20Group%20Phase-Rank%204%20(5.0%20Pts)-gold?style=for-the-badge&logo=trophy)](docs/TOURNAMENT_RUN_AND_RESULTS.md)
[![Campus](https://img.shields.io/badge/Campus-42%20Heilbronn-black?style=for-the-badge)](docs/TOURNAMENT_RUN_AND_RESULTS.md)
[![Playoff Finish](https://img.shields.io/badge/Knockout%20Stage-Top%208%20Quarterfinalist-purple?style=for-the-badge)](docs/TOURNAMENT_RUN_AND_RESULTS.md)
[![Action Failures](https://img.shields.io/badge/Action%20Failures-0%20(100%25%20Protocol%20Safe)-brightgreen?style=for-the-badge&logo=shield)](skills/track0-engine-conformance-audit/SKILL.md)
[![Sanitizer](https://img.shields.io/badge/Memory%20Safety-AddressSanitizer%20Clean-blue?style=for-the-badge&logo=c)](my-core-bot/)
[![Engine](https://img.shields.io/badge/Architecture-Deterministic%20C%20%2B%20Q--Matrix%20RL-crimson?style=for-the-badge)](docs/ARCHITECTURE_AND_METHODS.md)

Welcome to the official repository of **Team No_Plan_Just_Prayers** (42 Heilbronn) for the **42 CORE Game World Championship** (November 6–7, 2026).

This repository contains the complete C source code, autonomous evolutionary training harnesses, bit-packed tabular reinforcement learning (Q-Matrix) brains, and forensic reverse-engineering tools developed over an intense 24-hour engineering sprint. Our production bot propelled our team to a **4th-place international ranking in the Swiss Group Phase (5.0 / 6.0, 17.0 Buchholz)**—finishing as the **#1 ranked team across all 42 Heilbronn squads**—and reached the **Top 8 Quarterfinals** of the Championship Playoff bracket!

---

## 🌟 Tournament Highlights & Results

### 1. Swiss Group Phase (Rank 4 International)
Competing against teams across all global 42 campuses in the official Swiss tournament system, **`No_Plan_Just_Prayers`** achieved **Rank 4** with a **5–1 record** (Buchholz Score: 17.0):

<p align="center">
  <img src="docs/assets/Screenshot%20from%202026-11-07%2021-33-27.png" alt="Group Phase Rankings" width="850">
</p>

### 2. Championship Playoff Bracket (Top 8 Quarterfinalist)
Advancing into the knockout stage, **`No_Plan_Just_Prayers`** defeated **`Banana`** (42 Porto) in the Round of 16 before battling semifinalist **`L_oves_Coffee`** in a high-stakes playoff quarterfinal:

<p align="center">
  <img src="docs/assets/Screenshot%20from%202026-11-07%2021-30-43.png" alt="Playoff Bracket" width="850">
</p>

### 3. Reliability & Conformance Record
- **Strictly 0 Action Failures (0 AF):** Achieved 100% protocol safety across all official tournament matches and automated 20-match side-swapped evaluation gauntlets.
- **Zero Memory Leaks:** 100% AddressSanitizer clean across full 10,000-tick matches with zero buffer overruns and zero dangling pointers.

---

## 🧠 Multi-Track Engineering Architecture

When the official tournament opened, the organizers deployed a brand-new `configs/game.config.json` and updated `core_lib.h`, invalidating pre-event unit recipes. Over 24 hours, our team constructed a 4-track development pipeline:

```
                            ┌────────────────────────────────────────┐
                            │    42 CORE World Championship Repo     │
                            └───────────────────┬────────────────────┘
                                                │
         ┌──────────────────────────────┬───────┴──────────────────────┬──────────────────────────────┐
         ▼                              ▼                              ▼                              ▼
  Track 0: Linker Guards        Track 1: Self-Play Auto-Evolve   Track 2: Tabular Q-Matrix      Track 3: Micro & Logistics
  ──────────────────────        ──────────────────────────────   ─────────────────────────      ──────────────────────────
  • Linker `-Wl,--wrap` guards  • 22 tunable genome macros       • 64-state Core Q-Matrix       • 1-tick Hyper-Runner
  • Single-action locks         • `scripts/evolve.py` trainer    • 32-state Hunter Q-Matrix     • 3-tick Fast Digger
  • Mersenne Twister yields     • Multi-archetype gauntlet       • Affordance legal masking     • 22-HP Heavy Brawlers
  • Universal overkill tracking • Smoothed re-baselining         • Episode-trace protection     • Lanchester 2v1 rally
  • Same-tick gem accounting    • Evolved miner/carrier ratios   • Zero runtime ML overhead     • Orthogonal pincer siege
```

---

## 🔬 Reverse-Engineering the CORE Engine Physics

Winning deterministic grid RTS matches requires aligning every line of C code with the exact server execution rules.

### 1. The 5-Stage Server Tick Resolution Order
At the conclusion of every tick, `/core/server` collects queued actions from both teams and executes them in five strict stages:

$$\text{1. transfer\_gems} \longrightarrow \text{2. build} \longrightarrow \text{3. attack} \longrightarrow \text{4. move} \longrightarrow \text{5. create}$$

- **Same-Tick Deposit & Spawn (`Stage 1 → Stage 5`):** In `main.c`, we execute `handle_carrier()` and `handle_miners()` *before* `handle_spawner()`. When a Carrier transfers cargo to the Core in Stage 1, `__wrap_core_action_transferGems()` records the deposit in `g_pending_core_gems`. `handle_spawner()` then spends `core->s_core.gems + get_pending_core_gems()` in Stage 5 on the **exact same tick**, eliminating the standard 1-tick reaction delay.
- **Attack-Before-Move Priority (`Stage 3 → Stage 4`):** Because `attack` resolves before `move`, any unit already at Manhattan distance 1 with `action_cooldown <= 0` lands its blow before the target can step away.
- **The Distance-2 First-Strike Ambush (`Stage 4 → Next Tick Stage 3`):** When two melee fighters approach each other, whichever unit steps from distance 2 to distance 1 first does so in Stage 4 (`move`), resetting its cooldown to `base_action_cooldown` (e.g. 3 ticks). On the next tick, the stationary unit at distance 1 has `action_cooldown <= 0` and strikes first in Stage 3.

### 2. Action Cooldowns, Cargo Weight & Negative Readiness Wrap
1. **Cargo Slowdown Formula:** Moving, attacking, or transferring resets a unit's cooldown to:
   $$\text{cooldown} = \text{base\_action\_cooldown} + \left\lfloor \frac{\text{carried\_gems}}{\text{gems\_per\_cooldown\_step}} \right\rfloor$$
   Our Carrier (`carrier.c`) computes this exact loaded cooldown (`base_cd + (carried + gain) / step`) for every candidate gem pile on the map, selecting the target that maximizes **net gems delivered per tick**:
   $$\text{Score} = \frac{\text{gain} \times 1000}{\text{trip} + (\text{home} + 1) \times \text{loaded\_cd} + 2}$$
2. **Negative Idle Cooldowns & Readiness Wrap:** Standing idle decrements `action_cooldown` into negative numbers. However, once `action_cooldown` exceeds the internal standing limit, the unit forfeits readiness and wraps back to a positive cooldown penalty. In `combat.c`, our `g_waits[64]` timestamp table bounds all Distance-2 ambushes to `< 6` ticks so units never wrap their cooldowns.

### 3. Server Disassembly: The Mersenne Twister Tie-Breaker
By disassembling `/core/server` (`Game::tick` at `0x0006f8f0` and `shuffle_actions_vector` at `0x0009b8b0`), we discovered that within each stage, the server shuffles actions from both teams using a **64-bit Mersenne Twister PRNG seeded with `worldGeneratorSeed + current_tick`**.
- **Why Static Team-ID Assumptions Fail:** Neither Player 1 nor Player 2 has deterministic priority when two units contest the same empty tile or attack a 1-HP deposit on the same tick. Static checks (`is_p1_team()`) fail 50% of the time.
- **Our Deconfliction Fixes (`navigation.c`):**
  - **Unconditional Lethal Deposit Yield:** Before attacking a Deposit or Wall, `__wrap_core_action_attack()` sums the `damage_object` of all adjacent enemy units with `action_cooldown <= 0`. If `t->hp <= enemy_pending_dmg`, our unit yields its attack—avoiding an Action Failure if the enemy shuffles first—and scoops the resulting gem pile on the next tick.
  - **Corridor Transit Yield:** In `__wrap_core_action_move()`, if an armed enemy is adjacent to our target tile with `action_cooldown == 0`, our unit yields for 1 tick rather than risking a simultaneous-move collision, forcing the enemy to step into our melee range on cooldown.

---

## ⚔️ Tournament Unit Recipes & Mathematical Breakpoints

Under the tournament configuration (Base unit: `42g`, `7 HP`, `16 max_gems`, `4 base_cd`, `16 spawn_cd`; Combat: `+35g`, `+3 dmg`, `+3 cd`; Speed: `+55g`, `-2 cd`; Health: `+40g`, `+5 HP`; Demolition: `+35g`, `+3 obj dmg`, `+3 cd`; Siege: `+40g`, `+6 core dmg`, `+6 cd`), our bot deployed seven specialized unit templates:

| Unit Role (`name`) | Component Recipe | Cost | HP | Dmg | Base CD | Tactical Purpose |
| :--- | :--- | :---: | :---: | :---: | :---: | :--- |
| **Hyper-Runner** (`"Carrier"`) | `"max_gems"`, `"gem_speed"`, `"speed"`, `"speed"` | **232g** | 7 | 0 | **1 tick** | 166 gem capacity & 0 cargo slowdown; sweeps loose piles and 150-gem deposit hauls across the map. |
| **Fast Digger** (`"Miner"`) | `"demolition"`, `"speed"`, `"speed"` | **187g** | 7 | 3 (Obj) | **3 ticks** | Cracks a 50-HP deposit in **51 ticks** (vs 119 ticks for unaccelerated miner) and leaves 150-gem pile for the Runner. |
| **Solo Miner** (`"Miner"`) | `"demolition"` | **117g** | 7 | 3 (Obj) | 7 ticks | Early budget miner deployed before economy is fully online. |
| **Heavy Brawler** (`"Hunter"`) | `"combat"`, 3x `"health"`, 2x `"speed"` | **307g** | **22** | **3 (Unit)** | **3 ticks** | Core frontline fighter. Wins 2v1 and 1v1 melee exchanges via Lanchester's Square Law. |
| **High-Velocity Sieger** (`"Striker"`) | `"siege"`, 2x `"speed"`, 2x `"health"` | **262g** | **17** | **6 (Core)** | **3 ticks** | High-speed endgame finisher; dismantles the 500-HP enemy Core in ~83 ticks. |
| **Heavy Breaker** (`"Striker"`) | `"siege"`, `"demolition"`, `"health"` | **187g** | 12 | **6 (Core) / 3 (Obj)** | 10 ticks | Dual-purpose sapper-finisher deployed when fortified walls block direct line to base. |
| **Light Scout / Harasser** (`"Hunter"`) | `"combat"`, 2x `"speed"` | **167g** | 7 | 3 (Unit) | **1 tick** | Fast interceptor deployed at $T \le 250$ to crush enemy worker expansion. |

---

## 📈 Chronological Evolution of Our Engineering Tracks

```
v1 Baseline  ──►  v2 Memory/ASan  ──►  v3 10-0 Scrimmage  ──►  v4 Lanchester  ──►  v5 Deathball  ──►  v6 Champion (Rank 4)
  (Initial)         (Zero Leaks)       (Base Defense)         (2v1 Ambush)      (307g Brawlers)       (Orthogonal Siege)
```

### Track 0: Zero-Failure Linker Architecture (`navigation.c`)
- Intercepted all client actions at link time using `-Wl,--wrap=core_action_move -Wl,--wrap=core_action_attack -Wl,--wrap=core_action_transferGems`.
- Enforces single-action locks (`g_acted_unit_ids`), non-positive cooldown verification (`action_cooldown <= 0`), transit reservations (`g_moved_to_tile`), and deposit damage accounting (`get_pending_damage`).

### Track 1: Autonomous Evolutionary Self-Play (`scripts/evolve.py` & `tuned_params.h`)
- Parameterized 22 macro and micro thresholds in `inc/tuned_params.h`.
- Automated $(1 + \lambda)$ evolutionary trainer evaluating candidates across a multi-archetype gauntlet (`champion_best`, `opponent_speed_brawler`, `gridmaster`).
- Key evolutionary discovery: lowered carrier return threshold (`CARRIER_EARLY_THRESHOLD: 29 → 21`) to accelerate first-Miner timing and boosted rally mass (`SQUAD_RALLY_SIZE: 2 → 3`).

### Track 2: Bit-Packed Tabular Q-Matrix (`policy_matrix.h`, `matrix_brain.c`, `train_matrix.py`)
- Compressed board states into 6-bit Core states (64 buckets) and 5-bit Hunter states (32 buckets).
- Trained over 300+ matches using episode-trace Monte Carlo updates with Opening Credit Protection (protecting sound early decisions in long games).
- Verified zero Action Failures with instant static array lookups (`g_core_q[s][a]`).

### Track 3: Championship Heuristic Engine (`Champion v5` / `Candidate v6`)
- **Digger / Carrier Division of Labor:** Fast Diggers never scoop piles that a Carrier can reach via BFS (`ft_carrier_can_reach_pile`), more than doubling mining throughput.
- **Open-Corridor BFS Deposit Scoring:** `+50000` bonus for deposits reachable via open paths; walls given heavy traversal weight (`60 + hits * 8`).
- **Lanchester 2v1 Squad Rally:** Solitary Hunters pause up to 5 ticks at distance 3–5 when an allied Hunter is closing the gap, ensuring local numerical superiority.
- **Finisher Lane & Guard Posts:** Traces shortest path from Striker to enemy Core via `nav_compute_finisher_lane()`; positions Hunters at guard posts distance 2–4 *outside* that corridor so the Striker moves unobstructed.
- **Highway Sapper:** Dijkstra's algorithm between Cores identifies blocking obstacles; dispatches miners to carve open direct invasion highways.

---

## 🛠️ Forensic Catalog: Top 10 Bugs Solved During the Event

| # | Bug Name | Symptom in Replays | Root Cause & Surgical Fix |
| :---: | :--- | :--- | :--- |
| **1** | **Corner Spawn-Pad Corking** | Match timeouts at 10,000 ticks with 4,800+ gems unspent. | Idle workers parked at distance 1 from corner Core `(0,0)`, blocking both spawn exits. Fixed via `step_away_from_core()` and `nav_yield_for_workers()`. |
| **2** | **Digger 16-Gem Self-Haul** | 187g Digger took 9 slow round trips to haul broken deposit. | Digger attacked dropped 150-gem pile, filled its 16 capacity, and walked home. Fixed by barring Diggers from attacking piles reachable by a Carrier. |
| **3** | **Endgame Finisher Lane Block** | Striker got stuck behind allied Hunters at distance 2 from enemy Core. | Hunters clustered at distance 2 inside the main corridor. Fixed via `nav_compute_finisher_lane()` and `position_hunter_around_enemy_core()`. |
| **4** | **Hardcoded `team_id != 42` Freeze** | Combat units froze during live tournament matches (`team_id = 57690`). | `__wrap_core_action_move` checked `game.my_team_id != 42`. Fixed by replacing static ID checks with dynamic transit cooldown yielding. |
| **5** | **Miner Wall-Bashing** | Miners spent 50+ ticks hitting 50-HP walls while open deposits sat idle. | Deposit scoring used Manhattan distance through walls. Fixed via `+50000` open-corridor bonus and `60 + hits * 8` wall weight. |
| **6** | **Diagonal Wall Ambush Freeze** | Hunters froze at distance 2 across solid diagonal walls. | Distance-2 hold checked distance without checking line-of-step. Fixed by requiring `ft_has_open_melee_gap(u, e)`. |
| **7** | **1v1 Trickle into Enemy Swords** | Single Hunters walked one-by-one into 2v1 enemy traps. | Distance-2 hold lacked squad awareness. Fixed via `!ft_is_enemy_engaged_by_ally()` and 5-tick Lanchester Squad Rally. |
| **8** | **Multi-Unit Corridor Gridlock** | Trailing Brawlers detoured across the entire map. | `safe_travelability()` returned `CORE_TRAVEL_BLOCK` for all friendly units. Fixed by blocking only distance $\le 1$ and passing distance $\ge 2$ with weight 8. |
| **9** | **Dead-End Spawn Exit Runner** | Core spawned a 232g Runner into a walled-off pocket with 0 gems. | `ft_runner_opportunity()` took max gems across exits instead of min. Fixed by taking the minimum guaranteed gems across all open spawn exits. |
| **10** | **Q-Matrix Pacifist `GUARD_LANE`** | Track 2 Hunters ignored enemy Brawlers when `ally_near == 1`. | `legal_actions[GUARD_LANE]` was enabled while enemies were alive. Fixed by strictly gating `GUARD_LANE` to `best_target == NULL`. |

---

## 📋 Playbook for Future CORE Tournaments & Next-Gen Roadmap

### 1. Day-Zero Kickoff Checklist (First 30 Minutes)
1. **Diff the Live Config Immediately:** Run `./scripts/check_update_configs.sh` and inspect `configs/game.config.json`. Note base unit cost, HP, cooldowns, and all component deltas.
2. **Recompute the 4 Unit Breakpoints:**
   - 1-tick Runner (`base_cd - 2 * N_speed == 1` + `max_gems` + `gem_speed`).
   - Fast Digger (`demolition` + `speed` stack).
   - Max-Component Brawler (highest $\text{Effective\_HP} \times \text{DPS}$ product at 6 components).
   - High-DPS Core Finisher (`siege` stack + `speed` / `demolition`).
3. **Verify Linker Wrappers & Zero-AF Guard:** Compile with `-Wl,--wrap=...` and run a 2-game side-swapped scrimmage to confirm strictly 0 Action Failures.

### 2. Next-Generation Roadmap (v7+)
1. **Runtime Dynamic Component Solver (`unit_designer.c`):** Instead of static `#define` macros, parse the component table from `game.config` at tick 0 and solve for optimal Runner, Digger, Brawler, and Finisher templates dynamically.
2. **Kuhn-Munkres (Hungarian) Bipartite Assignment:** Replace greedy sequential resource/post claiming with an exact $O(N^3)$ bipartite matching pass for multi-Carrier, multi-Miner, and multi-Hunter target allocation.
3. **2-Ply Alpha-Beta Melee Combat Micro:** When 2–4 friendly Brawlers face 2–4 enemy Brawlers within distance $\le 4$, execute a 2-ply minimax search over joint moves/attacks to compute the optimal focus-fire and kiting combination.
4. **Eligibility-Trace $\text{TD}(\lambda)$ Q-Matrix with Phase Bucketing:** Expand `get_core_state_id()` with explicit `enemy_cleared` and game-phase bits ($T < 350$ vs $T \ge 350$), and replace end-of-match Monte Carlo updates with $\text{TD}(\lambda)$ step-wise combat trade rewards.

---

## 📁 Repository Directory Structure

```text
CORE-Game-World-Championship-No_Plan_Just_Prayers/
├── my-core-bot/                  # 🏆 Production Tournament Bot (Champion v6 - Rank 4 in Swiss)
│   ├── Makefile                  # Compiles with -Wl,--wrap linker guards & AddressSanitizer
│   ├── inc/                      # Headers: bot.h, core_lib.h, tuned_params.h
│   └── src/                      # Implementation: carrier, combat, getters, miners, navigation, spawner
├── SKILLS.md                     # 🧠 Master AI Agent Skills Registry
├── skills/                       # 📚 Reusable Autonomous Agent Skills
│   ├── track0-engine-conformance-audit/   # Zero-AF Linker Wrappers & Server Physics
│   ├── track1-evolutionary-self-play/     # Autonomous Evolutionary Parameter Tuning
│   ├── track2-tabular-q-matrix/           # Bit-Packed Tabular Q-Matrix & Legal Affordance Masks
│   ├── track3-combat-logistics-micro/     # Grandmaster Combat & BFS Logistics Micro
│   ├── core-game-reverse-engineering/     # Disassembly Traces & Tick Resolution Order
│   └── tournament-strategy-and-tactics/   # Swiss Buchholz Optimization & 20-Match Protocols
├── docs/                         # 📖 In-Depth Technical Manuals & Whitepapers
│   ├── ARCHITECTURE_AND_METHODS.md        # Comprehensive algorithmic analysis
│   ├── TOURNAMENT_RUN_AND_RESULTS.md      # Swiss standings, playoff chronicles, and match archives
│   ├── IMPROVEMENT_ROADMAP.md             # Architectural enhancement specifications
│   └── assets/                            # Official tournament ranking & bracket screenshots
├── bots/                         # 🔬 Development Archive & Research Prototypes
│   ├── candidate_v6/             # Evolved Gen 6 candidate (75% WR over v5)
│   ├── candidate_v7/             # Gen 7 research: Carrier Bait Trap, Harasser & Hybrid Sieger
│   ├── track1_gemini_evolution/  # Track 1 evolutionary testing branch
│   └── track2_matrix/            # Track 2 Q-Matrix RL engine, training scripts, and state logs
├── archive/                      # 🏛️ Historical Champion Milestones
│   ├── champion_v5/              # Baseline Champion v5 (95% win rate over early bots)
│   ├── champion_v4_lanchester/   # First Lanchester distance-2 hold implementation
│   ├── champion_v3_10_0/         # 10-0 scrimmage benchmark champion
│   └── v1_baseline_winner/       # Original baseline tournament bot
├── scripts/                      # ⚙️ Benchmark & Simulation Infrastructure
│   ├── isolated_scrimmage.py     # Containerized Docker scrimmage runner with log parsing
│   ├── run_20_benchmark.py       # 20-match side-swapped benchmark (Track 2 vs v5)
│   ├── run_20_v6_vs_v5.py        # 20-match side-swapped benchmark (v6 vs v5)
│   └── evolve.py                 # (1 + λ) autonomous evolutionary trainer
└── configs/                      # Official tournament match configurations
```

---

## 🚀 Quick Start & Reproduction Guide

### 1. Build the Production Bot
Compile the championship bot under GCC/Clang with AddressSanitizer enabled:
```bash
make -C my-core-bot re
```
*Guaranteed to build with `-Wall -Wextra -Werror -g -fsanitize=address` with 0 warnings and 0 errors.*

### 2. Run an Isolated Head-to-Head Scrimmage
Execute a side-swapped scrimmage inside an isolated Docker container:
```bash
python3 -c "
import sys
sys.path.append('.')
from scripts.isolated_scrimmage import run_match

# Run my-core-bot (P1) vs archived champion_v5 (P2)
res = run_match('my-core-bot/bot', 'archive/champion_v5/bot', p1_id=42, p2_id=43, tag='test_match')
print(f'Winner: {res.get(\"winner\")}, Ticks: {res.get(\"ticks\")}, P1 AF: {res.get(\"p1_af\")}, P2 AF: {res.get(\"p2_af\")}')
"
```

### 3. Run the Official 20-Match Side-Swapped Benchmark
To reproduce our 75.0% empirical win-rate verification suite:
```bash
python3 scripts/run_20_v6_vs_v5.py
```

### 4. Train the Track 2 Q-Matrix
To launch an autonomous Q-learning training run across the multi-opponent gauntlet:
```bash
python3 bots/track2_matrix/train_matrix.py --matches 90 --alpha 0.2
```

### 5. Dev Container & DevPod Setup
Set up the competition development environment using [Devpod](https://devpod.sh/):
```bash
make devcontainer
# Or specify your preferred IDE:
make devcontainer IDE=cursor
```

---

## 📜 Complete Documentation Index

For detailed technical analysis, consult the manuals in [`docs/`](docs/) and [`skills/`](skills/):

- 📖 **[Architecture & Methods Manual](docs/ARCHITECTURE_AND_METHODS.md):** Deep-dive analysis of algorithms, state machines, and wrapper mechanics.
- 🏆 **[Tournament Run & Results](docs/TOURNAMENT_RUN_AND_RESULTS.md):** Match-by-match breakdown of our Swiss Group Phase and Playoff runs.
- 🗺️ **[Improvement Roadmap](docs/IMPROVEMENT_ROADMAP.md):** Future strategies: online meta-adaptation, bipartite assignment, and 2-ply minimax melee search.
- 🧠 **[AI Agent Skills Registry](SKILLS.md):** Master index for all modular skills.
- 🛡️ **[Track 0: Engine Conformance & Zero-AF](skills/track0-engine-conformance-audit/SKILL.md):** Linker wrappers, tick resolution pipeline, and Mersenne Twister tie-breaking.
- 🧬 **[Track 1: Autonomous Evolutionary Self-Play](skills/track1-evolutionary-self-play/SKILL.md):** Genome parameterization, multi-archetype gauntlet, and anti-trap design.
- 🤖 **[Track 2: Bit-Packed Tabular Q-Matrix](skills/track2-tabular-q-matrix/SKILL.md):** Orthogonal state compression, legal affordance masks, and credit protection.
- ⚔️ **[Track 3: Championship Combat & BFS Logistics Micro](skills/track3-combat-logistics-micro/SKILL.md):** Hyper-Runner/Fast-Digger division, Lanchester 2v1 rallies, and finisher lanes.
- 🔬 **[Core Game Reverse Engineering](skills/core-game-reverse-engineering/SKILL.md):** Server disassembly, tick lifecycle, and component physics.
- 🎯 **[Tournament Strategy & Tactics](skills/tournament-strategy-and-tactics/SKILL.md):** Swiss tournament optimization, Buchholz scoring, and archetype counters.

---

<p align="center">
  <b>Developed with passion by Team No_Plan_Just_Prayers — 42 Heilbronn</b><br>
  <i>"In code we trust, in prayers we push."</i>
</p>
