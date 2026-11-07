---
name: track3-combat-logistics-micro
description: Implements Hyper-Runner + Fast Digger logistics separation, Open-Corridor BFS deposit scoring, Distance-2 Open-Gap First-Strike ambushes, Lanchester 2v1 Squad Rallying, Highway Sapper, and Finisher Lane sieges.
---

# Track 3: Championship Combat & BFS Logistics Micro Skill

## When to Use
Use this skill to implement the core tactical and logistical micro-behaviors across `carrier.c`, `miners.c`, `combat.c`, `spawner.c`, `getters.c`, and `navigation.c`.

---

## Core Engineering Rules

### 1. Hyper-Runner & Fast Digger Division of Labor (`carrier.c` & `miners.c`)

#### Unit Specialization:
- **Hyper-Runner (`232g`: `"max_gems"`, `"gem_speed"`, `"speed"`, `"speed"`):**
  - Base cooldown: **1 tick** | Capacity: **166 gems** | Slowdown: **0**
  - Purpose: Full-map resource vacuums. Evaluates candidate gem piles using BFS corridor distance and cargo-weighted return time.
- **Fast Digger (`187g`: `"demolition"`, `"speed"`, `"speed"`):**
  - Base cooldown: **3 ticks** | Object Damage: **3** | Capacity: **16 gems**
  - Purpose: High-speed deposit cracker. Cracks a 50-HP deposit in **51 ticks** (17 hits $\times$ 3 ticks) compared to 119 ticks for an unaccelerated 117g miner.

#### Cargo-Weighted Delivery Scoring Formula:
Carriers select target piles by maximizing the net gems delivered per tick:

$$\text{Score} = \frac{\text{gain} \times 1000}{\text{trip} + (\text{home} + 1) \times \text{loaded\_cd} + 2}$$

Where:
$$\text{loaded\_cd} = \text{base\_cd} + \left\lfloor \frac{\text{carried\_gems} + \text{gain}}{\text{gems\_per\_cooldown\_step}} \right\rfloor$$

#### Strict Separation Rule:
When an active Carrier is alive (`n_carriers > 0`) and a Miner has small cargo capacity (`max_gems <= 20`), the Miner **must never** target or attack an `OBJ_GEM_PILE` that the Carrier can reach (`ft_carrier_can_reach_pile`):
- The Digger strikes the deposit until broken, drops the 150-gem pile on the ground, and immediately moves to the next deposit.
- The Hyper-Runner sweeps the 150 gems in a single step and returns to base.

#### Open-Corridor Deposit Scoring:
In `ft_get_resource_for_miner()`:
- Award a `+50000` score priority to deposits reachable through already open corridors.
- Set `OBJ_WALL` traversal weight in `safe_travelability()` to `60 + hits * 8`, ensuring Miners never bash 50-HP walls while unmined open deposits remain on the board.

---

### 2. Distance-2 Open-Gap First Strike & Lanchester 2v1 Rally (`combat.c`)

#### Bounded Open-Gap Hold:
When a Heavy Brawler (`307g`: 22 HP, 3 DMG, 3 CD) is at Manhattan distance `2` from an armed enemy unit (`damage_unit > 0`):
Hold position **only if all three conditions hold**:
1. **Open Melee Gap:** `ft_has_open_melee_gap(u, e)` is true (there is an unobstructed traversable tile between both units).
2. **No Friendly Gang-Up Active:** `!ft_is_enemy_engaged_by_ally(e, u->id)` is true (if an ally is already at distance $\le 2$, do not wait—attack immediately for a 2v1 kill).
3. **Bounded Wait Horizon:** `get_unit_wait_ticks(u->id, raw_hold) < 6` (strictly bounded under 6 ticks to prevent the negative readiness wrap bug).

#### Lanchester 2v1 Squad Rally:
Under **Lanchester's Square Law**, combat strength scales with the square of unit numbers:
$$\text{Fighting Power} \propto N^2 \times (\text{HP} \times \text{DPS})$$

Two 22-HP Brawlers engaging one enemy Brawler take only $\approx 25\%$ proportional damage compared to two isolated 1v1 duels:
- If a Hunter is alone (`ft_count_allies_near_pos(u->pos, 2) == 0`) at distance `3–5` from an armed enemy and an allied Hunter is within `6` tiles closing the gap (`ally_closing`), pause movement for up to `5` ticks.
- Once the second Hunter arrives, both units advance simultaneously to execute an overwhelming 2v1 crossfire.

---

### 3. Highway Sapper & Finisher Lane Sieges (`navigation.c`, `miners.c`, `combat.c`)

#### Highway Sapper:
When all safe mineral deposits are exhausted:
1. Run Dijkstra's algorithm (`nav_find_highway_obstacle`) between `core->pos` and `opp_core->pos`.
2. Identify the single wall or deposit tile that obstructs the shortest straight path to the enemy Core.
3. Dispatch Fast Diggers to demolish that blocking tile, creating a direct highway for allied combat forces.

#### Unobstructed Finisher Lane:
When all enemy fighters are cleared (`ft_enemy_units_cleared() == true`):
1. **Trace Lane:** `nav_compute_finisher_lane()` calculates the shortest cardinal path from the `Striker` to `opp_core->pos`.
2. **Clear Corridor:** `position_hunter_around_enemy_core()` assigns all friendly Hunters to guard posts at distance `2–4` **outside** the computed finisher lane.
3. **Execute:** The High-Velocity Sieger (`262g`: 17 HP, 6 Core DMG, 3 CD) marches directly through the open corridor to distance `1`, destroying the 500-HP enemy Core in ~83 ticks while allied Hunters spawn-camp the opposing base exits.
