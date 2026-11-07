---
name: track0-engine-conformance-audit
description: Implements strictly 0-Action-Failure linker wrappers (--wrap), Mersenne Twister collision/deposit yields, universal overkill tracking, and 5-stage server tick alignment for 42 CORE C bots.
---

# Track 0: Engine Conformance & Zero-Action-Failure Wrapper Skill

## When to Use
Use this skill at the kickoff of any 42 CORE tournament or whenever a bot suffers from server `Action Failure` (AF) penalties, friendly collision dropouts, wasted overkill attacks on dying units/deposits, or corner spawn-pad corking.

---

## Core Engineering Rules

### 1. Linker Interception (`Makefile` & `navigation.c`)
Add the following linker flags to `LDFLAGS` in your `Makefile`:
```makefile
LDFLAGS += -Wl,--wrap=core_action_move -Wl,--wrap=core_action_attack -Wl,--wrap=core_action_transferGems
```

Implement the three wrapper entry points in `navigation.c`:

#### A. Single-Action-Per-Tick Lock (`has_unit_acted`)
Maintain a bitset or boolean lookup array `g_acted_unit_ids[512]`, reset at the start of each tick in `reservations_init_tick()`:
```c
static bool g_acted_unit_ids[512];

void reservations_init_tick(void)
{
    ft_memset(g_acted_unit_ids, 0, sizeof(g_acted_unit_ids));
    ft_memset(g_moved_to_tile, 0, sizeof(g_moved_to_tile));
    ft_memset(g_dmg_amounts, 0, sizeof(g_dmg_amounts));
    ft_memset(g_pile_harvest_gems, 0, sizeof(g_pile_harvest_gems));
    g_pending_core_gems = 0;
}
```
Whenever a unit issues `core_action_move`, `core_action_attack`, or `core_action_transferGems`, record its ID. Any subsequent command for the same unit in the same tick is dropped immediately.

#### B. Strict Cooldown Gate (`action_cooldown <= 0`)
Standing idle causes `action_cooldown` to decrement into negative numbers (down to `-50`):
- **Never check `action_cooldown != 0`**.
- **Always check:** `if (u->s_unit.action_cooldown > 0) return;`

---

### 2. Server 5-Stage Tick Alignment (`main.c`)
The 42 CORE server resolves queued actions in the following strict order at the conclusion of each tick:

$$\text{1. transfer\_gems} \longrightarrow \text{2. build} \longrightarrow \text{3. attack} \longrightarrow \text{4. move} \longrightarrow \text{5. create}$$

#### Same-Tick Deposit & Spawn Pipeline (`Stage 1 -> Stage 5`):
In `ft_on_tick()`, execute the roles in strict priority order:
```c
void ft_on_tick(void)
{
    reservations_init_tick();
    
    // 1. Logistics: Transfer gems to Core (resolves in Stage 1)
    handle_carrier();
    handle_miners();
    
    // 2. Combat: Attack & reposition (resolves in Stage 3 & 4)
    handle_combat();
    
    // 3. Spawner: Spends banked + pending gems on the exact same tick (Stage 5)
    handle_spawner();
}
```
Inside `__wrap_core_action_transferGems()`:
```c
if (dest->type == OBJ_CORE && dest->s_core.team_id == game.my_team_id)
{
    g_pending_core_gems += amount;
}
```
`handle_spawner()` calculates available budget as `core->s_core.gems + get_pending_core_gems()`. This eliminates the 1-tick delay between a carrier returning cargo and a defender spawning.

---

### 3. Mersenne Twister Tie-Breaker Deconfliction (`navigation.c`)
Disassembly of `/core/server` (`Game::tick` at `0x0006f8f0` and `shuffle_actions_vector` at `0x0009b8b0`) reveals that conflicting commands within a stage are shuffled using a 64-bit Mersenne Twister engine:
$$\text{Seed}_{\text{tick}} = \text{worldGeneratorSeed} + \text{current\_tick}$$

Static team-ID assumptions (such as expecting Player 1 to always win simultaneous moves or attacks) fail in 50% of matches. Enforce deterministic safety through the following deconfliction rules:

#### A. Unconditional Lethal Deposit Yield
In `__wrap_core_action_attack()`, if target is an `OBJ_DEPOSIT` or `OBJ_WALL`:
```c
int enemy_pending_dmg = 0;
for (int i = 0; i < game.objects_count; i++)
{
    t_core_object *e = game.objects[i];
    if (e && e->team_id != game.my_team_id && e->type == OBJ_UNIT)
    {
        if (ft_distance(e->pos, t->pos) == 1 && e->s_unit.action_cooldown <= 0)
            enemy_pending_dmg += e->s_unit.properties.damage_object;
    }
}
if (t->hp <= enemy_pending_dmg)
{
    // Enemy can destroy obstacle this tick; yield to avoid AF and scoop gem pile next tick
    return;
}
```

#### B. Corridor Transit Yield
In `__wrap_core_action_move()`, check if an armed enemy unit is adjacent to your target tile:
```c
for (int i = 0; i < game.objects_count; i++)
{
    t_core_object *e = game.objects[i];
    if (e && e->team_id != game.my_team_id && e->type == OBJ_UNIT)
    {
        if (e->s_unit.properties.damage_unit > 0 &&
            ft_distance(e->pos, target_pos) <= 1 &&
            e->s_unit.action_cooldown == 0)
        {
            // Yield transit for 1 tick; let enemy step into melee range on cooldown
            return;
        }
    }
}
```

#### C. Universal Overkill & Pile Deconfliction
Maintain tracking arrays for cumulative damage and cargo harvests:
- Abort any unit attack if `t->hp <= get_pending_damage(t->id)`.
- Abort any gem pile harvest if `get_pending_pile_gems(pile->id) > 0`.
