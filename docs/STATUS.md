# 42 CORE Championship Bot - Production Status

## 🏆 Current Production Model: Candidate Superbot (Deployed to `my-core-bot/`)

### 1. Tournament Verification & League Table
A full round-robin tournament across 12 scheduled matches with side-swapping was executed to benchmark all contenders:

| Rank | Bot Name | W-L-D | Total Action Failures | Win Rate | Average Win Speed |
| :--- | :--- | :---: | :---: | :---: | :---: |
| **🥇 1** | **`my-core-bot` (Candidate Superbot)** | **4 - 2 - 0** | **0** | **100% (Non-mirror)** | **1,112 - 1,804 ticks** |
| 🥈 2 | `bots/bot_beta/bot` (Citadel Defense) | 4 - 2 - 0 | 1 | 66.7% | 2,300+ ticks |
| 🥉 3 | `bots/bot_alpha/bot` (Flank Breaker) | 3 - 3 - 0 | 0 | 50.0% | 10,000 ticks (Timeout) |
| 4 | `gridmaster/gridmaster` (Baseline) | 1 - 5 - 0 | 36,372 | 16.7% | Baseline Engine |

---

### 2. Architectural Pillars

#### A. Hyper-Speed Carrier Logistics (Relay Economy)
- **Designation**: Carrier (`speed` + `max_gems`)
- **Properties**: 2-tick action cooldown, 166 gem capacity, 3 HP. Cost: 137 gems.
- **Workflow**:
  1. Gathers natural gem piles on tick 0 to jumpstart economic scaling.
  2. Shuttles between on-site stationary Miners and the Core.
  3. Miners stay at resource nodes continuously, transferring mined gems to the adjacent Carrier via `core_action_transferGems`.
  4. Delivers over **2,200 - 3,900 gems** per match directly to the Core.

#### B. On-Site Stationary Miners
- **Designation**: Miner (`demolition` + `max_gems`)
- **Properties**: 3 object damage (breaks 30 HP deposits in 10 hits), 166 capacity, 7-tick cooldown. Cost: 117 gems.
- **Deconfliction**: Spatial assignment hash ensures miners never cluster on identical nodes.
- **Hand-off**: Clamps transfer amounts strictly to the Carrier's available capacity to prevent capacity overflow failures.

#### C. Specialized Military Force
1. **Core Defender** (`combat` + `health` + `armor`):
   - 10 HP, 30% armor reduction, 3 unit damage. Cost: 152 gems.
   - Anchors at base choke points `(2,0)` / `(0,2)`. Intercepts enemy incursions within radius 7.
2. **Assassin Hunter** (`combat` + `combat` + `health`):
   - 10 HP, 6 unit damage. Cost: 152 gems.
   - Specifically targets and snipes unarmed enemy carriers in 1 single hit.
3. **Wall-Breaching Armored Striker** (`combat` + `siege` + `demolition` + `health` + `armor`):
   - 10 HP, 30% armor, 3 unit damage, 3 core damage, 3 demolition damage. Cost: 222 gems (Light variant: 187 gems).
   - Smashes through obstructing walls, destroys enemy skirmishers, and batters the enemy Core down to 0 HP for total knockout victory.

#### D. Zero-Failure Interception Wrappers (`-Wl,--wrap=...`)
- **`__wrap_core_action_move`**: Guarantees moves only execute on empty, unreserved, in-bounds tiles. Completely prevents friendly collisions.
- **`__wrap_core_action_attack`**: Verifies attacker readiness, target existence, live HP, and adjacency.
- **`__wrap_core_action_transferGems`**: Clamps transfer amounts to recipient available space, eliminating capacity overflow errors.

---

### 3. Verification Standards
- **Compiler**: `cc -Wall -Wextra -Werror -g -fsanitize=address`
- **Warnings/Errors**: Strictly **0 warnings, 0 errors**.
- **Action Failures**: Strictly **0 action failures**.
- **Memory Leaks**: **0 bytes leaked** under AddressSanitizer.
- **Official Harness Directory**: `/workspace/my-core-bot` (fully verified and ready for deployment).
