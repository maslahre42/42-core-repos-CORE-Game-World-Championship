<p align="center">
  <img src="https://avatars.githubusercontent.com/u/147154505?s=200&v=4" alt="CORE Logo" width="130">
</p>

# 🏆 42 CORE Game World Championship — Team: No_Plan_Just_Prayers

[![Tournament Rank](https://img.shields.io/badge/Swiss%20Group%20Phase-Rank%204%20(5.0%20Pts)-gold?style=for-the-badge&logo=trophy)](docs/TOURNAMENT_RUN_AND_RESULTS.md)
[![Campus](https://img.shields.io/badge/Campus-42%20Heilbronn-black?style=for-the-badge)](docs/TOURNAMENT_RUN_AND_RESULTS.md)
[![Action Failures](https://img.shields.io/badge/Action%20Failures-0%20(100%25%20Protocol%20Safe)-brightgreen?style=for-the-badge&logo=shield)](skills/track1-deterministic-evolution/SKILL.md)
[![Sanitizer](https://img.shields.io/badge/Memory%20Safety-AddressSanitizer%20Clean-blue?style=for-the-badge&logo=c)](my-core-bot/)
[![Architecture](https://img.shields.io/badge/Engine-Deterministic%20C%20%2B%20Q--Matrix%20RL-purple?style=for-the-badge)](docs/ARCHITECTURE_AND_METHODS.md)

Welcome to the official repository of **Team No_Plan_Just_Prayers** (42 Heilbronn) for the **42 CORE Game World Championship**.

This repository houses the full development history, production bots, reverse-engineered server physics, autonomous reinforcement learning pipelines, and comprehensive documentation that propelled our team to a **4th-place international ranking in the Swiss Group Phase (5–1 score)** and deep into the Championship Playoff bracket!

---

## 🌟 Tournament Highlights & Results

### 1. Swiss Group Phase (International Leaderboard: Rank 4)
Competing against teams across all global 42 campuses in the official Swiss tournament system, **`No_Plan_Just_Prayers`** finished **Rank 4** with **5 Wins and 1 Loss** (Buchholz Score: 17.0):

<p align="center">
  <img src="docs/assets/Screenshot%20from%202026-11-07%2021-33-27.png" alt="Group Phase Rankings" width="850">
</p>

### 2. Championship Playoff Bracket
Advancing into the elimination bracket, **`No_Plan_Just_Prayers`** defeated **`Banana`** (42 Porto) in the Round of 16 before battling semifinalist **`L_oves_Coffee`** in a hard-fought playoff quarterfinal:

<p align="center">
  <img src="docs/assets/Screenshot%20from%202026-11-07%2021-30-43.png" alt="Playoff Bracket" width="850">
</p>

---

## 🧠 Dual-Track Engineering Architecture

Our team approached the championship through two parallel, rigorously benchmarked engineering tracks:

```
                            ┌────────────────────────────────────────┐
                            │    42 CORE World Championship Repo     │
                            └───────────────────┬────────────────────┘
                                                │
                 ┌──────────────────────────────┴──────────────────────────────┐
                 ▼                                                             ▼
  Track 1: Deterministic C Micro-Engine                         Track 2: Embedded State-Bucket Q-Matrix
  ─────────────────────────────────────                         ───────────────────────────────────────
  • Production Bot: `my-core-bot/` (Champion v6)                • Research Prototype: `bots/track2_matrix/`
  • Lanchester Square Law Melee Deathball                       • 6-bit Core (64 buckets) & 5-bit Hunter (32 buckets)
  • Orthogonal Pincer Siege around enemy Core                   • Continuous speed-weighted terminal return ($R \in [-100..+150]$)
  • Dynamic Flank Sapper (demolishing border walls)             • Opening Credit Protection ($T \le 250$ loss shield)
  • Early Warrior Rush Counter at $T \le 250$                   • Multi-archetype training gauntlet (Mirror, Flank, Rusher)
  • Unconditional Transit Yielding (**Strictly 0 AF**)          • Zero-overhead static C table inference
  • **Outcome:** 75.0% Win Rate (15–5) over v5 🏆              • **Outcome:** 0 AF guarantee; analyzed Skirmisher Trap
```

---

## ⚔️ Key Tactical & Engineering Innovations

### 1. The 307g Heavy Brawler Deathball (`src/spawner.c`)
- **Formula:** `frame` (42g) + `combat` (35g) + 3x `health` (120g) + 2x `speed` (110g)
- **Stats:** **22 HP**, **3-tick base cooldown**, **3 Unit DMG**, **0 Cargo slowdown**
- **Physics:** Dominates sequential melee exchanges via **Lanchester's Square Law** ($\text{Fighting Power} \propto \text{HP} \times \text{DPS}$). Eliminating slow 7-tick fighters guaranteed a $2\times$ attack rate advantage.

### 2. Orthogonal Pincer Siege (`src/combat.c`)
- Prevents the common bottleneck where friendly fighters crowd onto one side of the enemy Core.
- Coordinating Hunters track ally cardinal posts: one Hunter claims the horizontal cardinal axis ($y = y_{\text{core}}$), while the second Hunter claims the perpendicular vertical axis ($x = x_{\text{core}}$).
- Encircles the opponent's base and shuts down opposing spawn escapes.

### 3. Dynamic Flank Sapper (`src/miners.c`)
- When central corridors deadlock in a Brawler standoff, our Fast Digger (`demolition`, `speed`, `speed`) navigates to safe outer partition walls ($x \le 4$ or $y \le 4$).
- Breaches the wall in 15 ticks, establishing a stealth highway directly into the enemy Core and unlocking isolated mineral deposits.

### 4. Early Warrior Rush Interception (`src/spawner.c`)
- Forensic replay analysis of loss replays (e.g. `replayv5.json`) revealed vulnerability to opening 1-tick Fast Scout rushes.
- Champion v6 introduces an immediate opening interceptor at $T \le 250$: if an armed hostile approaches on an open highway without prior economy, a combat defender spawns immediately before any worker investment.

### 5. Unconditional Transit Yielding for 0 Action Failures (`src/navigation.c`)
- Binary disassembly of `/core/server` revealed that simultaneous step contests into the same tile are resolved by a 64-bit Mersenne Twister shuffle (`std::shuffle`). Static priority checks (`is_p1_team()`) fail $50\%$ of the time.
- Our custom linker wrappers (`-Wl,--wrap=core_action_move`, `-Wl,--wrap=core_action_attack`) enforce **unconditional transit yields**: combat units pause for 1 tick if an enemy with cooldown 0 is adjacent to the destination.
- This lets the enemy step into adjacent melee range first—granting our unit a **free first strike** and producing **strictly 0 Action Failures across hundreds of matches**.

### 6. Carrier Gem Bait Trap (`bots/candidate_v7/`)
- Carriers drop a single bait gem on a corridor tile 2–4 spaces from our Core.
- Enemy collectors path towards the bait gem, where allied Hunters waiting in distance-2 ambush execute an inescapable crossfire kill, harvesting both the bait gem and the enemy's dropped cargo.

---

## 📁 Repository Directory Structure

```text
├── my-core-bot/               # 🏆 ACTIVE PRODUCTION BOT (Champion v6 - 4th in Swiss)
│   ├── inc/                   # Header definitions, component costs, tuned parameters
│   └── src/                   # C source files (carrier, combat, getters, miners, navigation, spawner)
├── skills/                    # 📚 REUSABLE KNOWLEDGE SKILLS (Antigravity Format)
│   ├── core-game-reverse-engineering/   # Server disassembly, tick lifecycle, component chemistry
│   ├── track1-deterministic-evolution/  # Lanchester holds, orthogonal siege, flank sapper, 0-AF wrappers
│   ├── track2-matrix-reinforcement-learning/ # State compression, reward shaping, embedded Q-Matrix
│   └── tournament-strategy-and-tactics/ # Swiss optimization, archetype counters, baiting manual
├── docs/                      # 📖 TECHNICAL DOCUMENTATION & WHITEPAPERS
│   ├── ARCHITECTURE_AND_METHODS.md     # Deep-dive system architecture and algorithm analysis
│   ├── TOURNAMENT_RUN_AND_RESULTS.md   # Official Swiss standings, playoff chronicle, match archives
│   ├── IMPROVEMENT_ROADMAP.md          # Strategic roadmap for future competitions and bot play
│   └── assets/                         # Tournament bracket and ranking screenshots
├── bots/                      # 🔬 DEVELOPMENT ARCHIVE & EXPERIMENTAL TRACKS
│   ├── candidate_v6/          # Evolved Gen 6 candidate (75% win rate over v5)
│   ├── candidate_v7/          # Gen 7 research: Carrier Bait Trap, Harasser & Hybrid Sieger
│   ├── track1_gemini_evolution/ # Track 1 evolution testing branch
│   └── track2_matrix/         # Track 2 Q-Matrix RL engine, training scripts, and state logs
├── archive/                   # 🏛️ HISTORICAL CHAMPION RELEASES
│   ├── champion_v5/           # Baseline Champion v5 (95% win rate over early bots)
│   ├── champion_v4_lanchester/# First Lanchester distance-2 hold implementation
│   ├── champion_v3_10_0/      # 10-0 scrimmage benchmark champion
│   └── v1_baseline_winner/    # Original baseline tournament bot
├── scripts/                   # ⚙️ BENCHMARK & SIMULATION INFRASTRUCTURE
│   ├── isolated_scrimmage.py  # Containerized Docker scrimmage runner with log parsing
│   ├── run_20_benchmark.py    # 20-match side-swapped tournament benchmark (Track 2 vs v5)
│   └── run_20_v6_vs_v5.py     # 20-match side-swapped tournament benchmark (v6 vs v5)
└── configs/                   # Official tournament match configurations
```

---

## 🚀 Quick Start Guide

### 1. Build the Production Bot
To compile the championship bot under GCC/Clang with AddressSanitizer enabled:
```bash
make -C my-core-bot re
```
*Guaranteed to build with `-Wall -Wextra -Werror -g -fsanitize=address` with zero warnings and zero errors.*

### 2. Run an Isolated Head-to-Head Scrimmage
Execute a fully side-swapped scrimmage inside an isolated Docker container with AddressSanitizer monitoring:
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
To launch an autonomous Q-learning training run across the 3-opponent gauntlet:
```bash
python3 bots/track2_matrix/train_matrix.py --matches 90 --alpha 0.2
```

---

## 🛠️ Dev Container & DevPod Setup

To set up the competition development environment using [Devpod](https://devpod.sh/):

```bash
make devcontainer
```

You can specify your preferred editor (VS Code, Zed, CLion, Cursor):
```bash
make devcontainer IDE=cursor
```

---

## 📜 Complete Documentation Index

For exhaustive technical deep-dives, please consult the manuals in [`docs/`](docs/) and [`skills/`](skills/):

- 📖 **[Architecture & Methods Manual](docs/ARCHITECTURE_AND_METHODS.md):** Detailed analysis of algorithms, spawner logic, and wrapper mechanics.
- 🏆 **[Tournament Run & Results](docs/TOURNAMENT_RUN_AND_RESULTS.md):** Match-by-match breakdown of our Swiss Group Phase and Playoff runs.
- 🗺️ **[Improvement Roadmap](docs/IMPROVEMENT_ROADMAP.md):** Future strategies: online meta-adaptation, multi-carrier relays, and bait traps.
- 🔬 **[Core Game Reverse Engineering Skill](skills/core-game-reverse-engineering/SKILL.md):** Full server disassembly, Mersenne Twister math, and component physics.
- ⚔️ **[Track 1 Deterministic Evolution Skill](skills/track1-deterministic-evolution/SKILL.md):** Lanchester Square Law, focus fire, and pincer siege design.
- 🤖 **[Track 2 Q-Matrix Reinforcement Learning Skill](skills/track2-matrix-reinforcement-learning/SKILL.md):** 6-bit/5-bit compression, speed returns, and the Skirmisher Trap.
- 🎯 **[Tournament Strategy & Tactics Skill](skills/tournament-strategy-and-tactics/SKILL.md):** Swiss tournament playbook and archetype counters.

---

<p align="center">
  <b>Developed with passion by Team No_Plan_Just_Prayers — 42 Heilbronn</b><br>
  <i>"In code we trust, in prayers we push."</i>
</p>
