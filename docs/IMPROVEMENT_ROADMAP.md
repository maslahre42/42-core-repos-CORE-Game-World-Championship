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
