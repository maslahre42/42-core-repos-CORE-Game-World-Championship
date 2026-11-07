# 🧠 AI Agent Skills Registry — 42 CORE Championship

This repository provides modular, battle-tested AI agent skills in the [`skills/`](skills/) directory, encapsulating every engineering discipline and competitive strategy developed during the **42 CORE Game World Championship** by Team **`No_Plan_Just_Prayers`**.

---

## 📑 Master Skill Registry

| Track / Domain | Skill Location | Primary Purpose & Activation Criteria |
| :--- | :--- | :--- |
| **Track 0** | [`skills/track0-engine-conformance-audit/SKILL.md`](skills/track0-engine-conformance-audit/SKILL.md) | **Zero-AF Linker Wrappers & Server Physics:** Load first in any CORE tournament to set up `--wrap` action interceptors, Mersenne Twister tie-breaking, deposit/move collision yields, and same-tick gem spending. |
| **Track 1** | [`skills/track1-evolutionary-self-play/SKILL.md`](skills/track1-evolutionary-self-play/SKILL.md) | **Evolutionary Parameter Optimizer (`evolve.py`):** Load when tuning macro, spatial, and combat thresholds in `inc/tuned_params.h` using multi-archetype Hall-of-Fame gauntlets and smoothed re-baselining. |
| **Track 2** | [`skills/track2-tabular-q-matrix/SKILL.md`](skills/track2-tabular-q-matrix/SKILL.md) | **Bit-Packed Tabular Q-Matrix (`policy_matrix.h`):** Load when building or training state-bucket reinforcement learning brains with strict legal affordance masks and opening credit protection. |
| **Track 3** | [`skills/track3-combat-logistics-micro/SKILL.md`](skills/track3-combat-logistics-micro/SKILL.md) | **Grandmaster Combat & BFS Logistics Micro:** Load when implementing Hyper-Runner/Fast-Digger separation, open-corridor deposit scoring, Distance-2 first-strike ambushes, Lanchester 2v1 rallies, and Finisher Lane sieges. |
| **Diagnostics** | [`skills/core-game-reverse-engineering/SKILL.md`](skills/core-game-reverse-engineering/SKILL.md) | **Deep Engine Reverse-Engineering:** Disassembly traces of `/core/server`, 64-bit PRNG seeding math, component stat mechanics, and tick serialization internals. |
| **Playbook** | [`skills/tournament-strategy-and-tactics/SKILL.md`](skills/tournament-strategy-and-tactics/SKILL.md) | **Championship Strategy & Swiss Playbook:** Swiss Buchholz optimization, 20-match side-swapped evaluation protocols, and competitive archetype counter-strategies. |

---

## 🛠️ Usage Guidelines for Autonomous Coding Agents

When working on this codebase or competing in future CORE tournaments, autonomous coding agents should reference these skills based on task context:

1. **Before writing any action calls (`core_action_*`)**:
   Consult **Track 0** to ensure all server calls are guarded by linker wrappers and single-action locks.
2. **When tuning bot numbers, timings, or radii**:
   Consult **Track 1** to isolate parameters in `tuned_params.h` and run side-swapped gauntlets rather than guessing values.
3. **When modifying Q-learning state encodings or tables**:
   Consult **Track 2** to maintain bit-packing efficiency, affordance masks, and ensure pacifist bugs are never reintroduced.
4. **When writing unit movement, mining paths, or combat positioning**:
   Consult **Track 3** for proven algorithmic formulas (cargo-weighted BFS, Lanchester squad rally, finisher lane reservation).
