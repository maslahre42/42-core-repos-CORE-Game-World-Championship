# Improvement Roadmap: Strategic Enhancements for Future Competitions

This document outlines high-impact architectural and tactical enhancements identified during the championship tournament run, intended to guide future development and competition play.

---

## 1. Online Opponent Meta-Classification (First 300 Ticks)

### Problem
Current bots rely on static opening rules. If an opponent opens with an unconventional composition (e.g. triple miners, 100% scouts, or extreme defensive turtling), our bot adapts slowly through general reactive checks.

### Proposed Solution
Implement an **Online Archetype Classifier** that tracks enemy spawns and movement during the first 300 ticks:

```c
typedef enum e_opponent_meta
{
    META_UNKNOWN = 0,
    META_FAST_RUSH,        // Enemy spawned combat unit before tick 100
    META_HEAVY_DEATHBALL,  // Enemy pumping 300g+ brawlers
    META_GREEDY_ECONOMY,   // Enemy has >= 2 miners with 0 combat units
    META_DEFENSIVE_TURTLE  // Enemy stacking units within 4 tiles of core
} t_opponent_meta;
```

#### Counter-Play Triggers:
1. **`META_FAST_RUSH`:** Immediately pause worker production; spawn 1 Fast Scout + 1 Heavy Brawler for permanent lane denial.
2. **`META_GREEDY_ECONOMY`:** Build 1 Sapper-Harasser (`speed`, `demolition`, `combat`) to assassinate undefended miners and starve their economy.
3. **`META_DEFENSIVE_TURTLE`:** Refrain from frontal melee; transition to triple-miner sapping, mine the entire map's gems, and win by 10,000-tick net worth or flank siege.

---

## 2. Resolving the "Skirmisher Trap" in Track 2 Q-Matrix

### Problem
In Track 2's reinforcement learning gauntlet, facing squishy opponents (`monir_bot` and speed brawlers) caused the Q-table to assign $+129.51$ to cheap `SKIRMISHER` (167g, 10 HP) because it achieved fast 800-tick wins. When matched against Champion v5's 22-HP Heavy Brawlers, the 10-HP Skirmishers were slaughtered via Lanchester's Square Law.

### Architectural Solution
Expand the **Core Spawner State Space** by 1 bit:

- **Bit 6 (Weight 64): `opp_heavy_mass` $\in \{0, 1\}$**
  - Set to `1` if the opponent has at least one combat unit with $\text{HP} \ge 16$.
  - Set to `0` if all enemy combat units are lightweight ($\text{HP} \le 10$) or unarmored.

#### Impact:
This doubles Core states from 64 to 128:
- Under State $(S + 64)$ (`opp_heavy_mass == 1`), `BRAWLER` will naturally accumulate positive returns ($+110.0$), while `SKIRMISHER` accumulates negative returns ($-80.0$).
- Under State $S$ (`opp_heavy_mass == 0`), `SKIRMISHER` will continue to exploit cheap speed wins.

---

## 3. Multi-Carrier Logistics & Dual-Relay Networks

### Problem
In late-game matches on large $20 \times 20$ grids, a single Carrier walking back and forth across 15 tiles wastes 30 ticks per round trip.

### Proposed Solution
1. **Dynamic 2nd Carrier Trigger:**
   Spawn a 2nd Carrier if:
   $$\text{TotalLooseGemsOnMap} \ge 250 \quad \text{AND} \quad \text{DistToNearestDeposit} \ge 12$$
2. **Relay Depot Partitioning:**
   - Carrier 1 harvests far-field deposits and deposits into an intermediate `OBJ_GEM_PILE` located 5 tiles from Core.
   - Carrier 2 acts as a shuttle, hauling 100-gem loads from the depot directly into the Core.
   - Eliminates travel bottlenecks and keeps mining outputs clear.

---

## 4. Full Gem Bait Trap Integration (Candidate v7 Evolution)

### Candidate v7 Implementation Review
As prototyped in [`bots/candidate_v7/src/carrier.c`](file:///goinfre/malshare/malshare/Core_game_world/CORE-Game-World-Championship-No_Plan_Just_Prayers/bots/candidate_v7/src/carrier.c#L250-L275):
```c
// Carrier Bait Drop
if (bait_pile && carried >= 2 && ft_is_adjacent(u->pos, bait_pile->pos))
{
    core_action_transferGems(u, bait_pile->pos, 1);
}
```

### Next Steps for Tournament Play:
1. **Dynamic Ambush Geometry:**
   - Ensure the bait pile is placed in a narrow 1-tile corridor.
   - Position two friendly Hunters in distance-2 ambush hold along the flanks.
   - When an enemy Carrier enters the corridor tile, the two Hunters close in simultaneously from opposite sides, executing an inescapable crossfire kill.
2. **Bait Recovery:**
   - Once the enemy Carrier dies, our Carrier immediately sweeps in to recover both the 1 bait gem and the enemy's dropped cargo (often 50–100 gems).

---

## 5. Tactical Fallback & HP Preservation

### Current Behavior
Hunters engage until death. If a 3-HP Hunter faces an enemy 22-HP Brawler, it stays in melee and dies in 1 hit, giving the enemy free kill value.

### Proposed Tactical Fallback:
```c
// combat.c: Tactical Retreat
if (u->hp <= 3 && nearest_enemy && nearest_enemy->hp > 6)
{
    // If ally is within 3 tiles, fall back behind ally!
    if (ally && ft_distance(u->pos, ally->pos) <= 3)
    {
        safe_travel(u, core->pos); // Step back toward base/allies
        return;
    }
}
```
Preserving low-HP fighters allows them to serve as bait, screen for incoming siegers, or assist in 2v1 focus fire without feeding enemy kill stats.


---

## 6. Runtime Dynamic Component Solver (`unit_designer.c`)

### Problem
Current implementations rely on hardcoded component recipes and cost macros (`#define COST_BRAWLER_HEAVY 307` in `bot.h`). When tournament organizers modify component costs or stat values right before round 1 (as occurred in this championship), static bots require manual recompilation and risk deploying suboptimal builds.

### Proposed Solution
Implement a dynamic component solver at tick 0 that parses `game.config` component arrays:
- **Solver Objective:** Solve the integer linear program (ILP) or combinatorial bounded knapsack problem over available components (max 6 per frame):
  - **Runner Objective:** Maximize $\text{Capacity} / \text{Cooldown}$ subject to $\text{Cooldown} = 1$ and $\text{CargoSlowdown} = 0$.
  - **Digger Objective:** Maximize $\text{Object\_Damage} / \text{Cooldown}$ subject to $\text{Cost} \le 200$.
  - **Brawler Objective:** Maximize $\text{Effective\_HP} \times (\text{Unit\_Damage} / \text{Cooldown})$ (Lanchester index).
  - **Finisher Objective:** Maximize $\text{Core\_Damage} / \text{Cooldown}$.
- The solver dynamically populates runtime recipe strings and cost constants before tick 1, guaranteeing optimal unit templates under any arbitrary server configuration.

---

## 7. Kuhn-Munkres (Hungarian) Bipartite Assignment

### Problem
Current resource and guard post allocations execute greedily: the first Miner in the iteration loop selects the globally highest-scoring deposit, potentially claiming a deposit 12 tiles away and leaving a closer deposit to a lagging worker.

### Proposed Solution
Model multi-worker and multi-combat coordination as a minimum-weight bipartite matching problem:
$$\min \sum_{i=1}^N \sum_{j=1}^M C_{i, j} X_{i, j} \quad \text{subject to} \quad \sum_{j} X_{i, j} \le 1, \; \sum_{i} X_{i, j} \le 1$$
- Let set $U = \{u_1, \dots, u_N\}$ be friendly units.
- Let set $V = \{v_1, \dots, v_M\}$ be targets (deposits, guard posts, enemy targets).
- Weight matrix $C_{i, j} = \text{BFS\_Distance}(u_i, v_j) - \text{Strategic\_Value}(v_j)$.
- Solve via the $O(N^3)$ Kuhn-Munkres (Hungarian) algorithm every 5 ticks.
- Eliminates target swapping, route crossing, and guarantees globally minimal travel overhead.

---

## 8. 2-Ply Alpha-Beta Melee Combat Micro

### Problem
Rule-based distance-2 holds perform exceptionally well in 1v1 and 2v1 engagements, but multi-unit confrontations (e.g. 3v3 or 4v4 clashes) suffer from greedy action selection where units may step into simultaneous enemy focus-fire.

### Proposed Solution
When 2–4 friendly combat units are within Manhattan distance $\le 4$ of 2–4 enemy combat units, trigger a bounded 2-ply minimax search:
1. **Branching Factor:** For each unit with `action_cooldown <= 0`, consider cardinal moves and attacks: $\mathcal{A} \in \{\text{North}, \text{South}, \text{East}, \text{West}, \text{Stay}, \text{Attack}\}$.
2. **Evaluation Function at Depth 2:**
   $$\text{Eval} = \sum_{u \in \text{Allies}} \left(\text{HP}_u \times \text{DPS}_u\right) - \sum_{e \in \text{Enemies}} \left(\text{HP}_e \times \text{DPS}_e\right)$$
3. **Pruning:** Alpha-beta pruning with move ordering prioritizing focus-fire attacks and transit yields.
4. **Execution Time:** Restrict search to $< 1.5$ ms per tick to comfortably remain within the server's per-tick packet deadline.

---

## 9. Eligibility-Trace $\text{TD}(\lambda)$ Q-Matrix with Phase Bucketing

### Problem
The current Track 2 Q-Matrix uses episodic Monte Carlo returns applied at the end of the match. While effective for macro economy, episodic rewards provide delayed feedback for tactical combat exchanges.

### Proposed Solution
1. **State Space Phase Expansion:**
   Add explicit phase and cleared bits to `get_core_state_id()`:
   - Bit 6: `enemy_cleared` (distinguishes mid-game fighting from endgame siege).
   - Bit 7: `game_phase` ($T < 350$ early expansion vs $T \ge 350$ mid-late deathball).
2. **Step-Wise $\text{TD}(\lambda)$ Temporal Difference Learning:**
   Replace terminal-only rewards with incremental rewards after combat trades:
   $$R_{\text{combat}} = \Delta \text{HP}_{\text{enemy}} \times \text{Value}_{\text{damage}} - \Delta \text{HP}_{\text{ally}} \times \text{Value}_{\text{cost}}$$
   Maintain an eligibility trace vector $e(s, a)$ decayed by $\gamma \lambda$ each tick:
   $$\delta_t = R_{t+1} + \gamma Q(S_{t+1}, A_{t+1}) - Q(S_t, A_t)$$
   $$Q(s, a) \leftarrow Q(s, a) + \alpha \delta_t e(s, a)$$
   Accelerates Q-value convergence by $10\times$ and directly aligns combat actions with local tactical advantages.
