#!/usr/bin/env python3
import argparse
import json
import os
import re
import subprocess
import sys
import time

WORKSPACE_DIR = "/goinfre/malshare/malshare/Core_game_world/CORE-Game-World-Championship-No_Plan_Just_Prayers"
MATRIX_DIR = os.path.join(WORKSPACE_DIR, "bots", "track2_matrix")
HEADER_PATH = os.path.join(MATRIX_DIR, "inc", "policy_matrix.h")
STATE_JSON_PATH = os.path.join(MATRIX_DIR, "matrix_state.json")

sys.path.append(WORKSPACE_DIR)
from scripts.isolated_scrimmage import run_match

NUM_CORE_STATES = 64
NUM_CORE_ACTIONS = 5
NUM_HUNTER_STATES = 32
NUM_HUNTER_ACTIONS = 5
MIN_MATRIX_CONFIDENCE = 3

CORE_ACTION_NAMES = ["RUNNER", "DIGGER", "BRAWLER", "SKIRMISHER", "FINISHER"]
HUNTER_ACTION_NAMES = ["SHARED", "LOWEST_HP", "WORKER", "HOLD_D2", "GUARD_LANE"]

DEFAULT_OPPONENTS = [
    "my-core-bot/bot",
    "monir_bot/bot",
    "bots/opponent_speed_brawler/bot"
]

def load_or_init_state(reset=False):
    if not reset and os.path.exists(STATE_JSON_PATH):
        try:
            with open(STATE_JSON_PATH, "r") as f:
                state = json.load(f)
            print(f"[TRAIN] Loaded existing matrix state from {STATE_JSON_PATH}")
            return state
        except Exception as e:
            print(f"[TRAIN] Failed to load {STATE_JSON_PATH} ({e}), initializing fresh state.")

    state = {
        "core_q": [[0.0 for _ in range(NUM_CORE_ACTIONS)] for _ in range(NUM_CORE_STATES)],
        "core_visits": [0 for _ in range(NUM_CORE_STATES)],
        "hunter_q": [[0.0 for _ in range(NUM_HUNTER_ACTIONS)] for _ in range(NUM_HUNTER_STATES)],
        "hunter_visits": [0 for _ in range(NUM_HUNTER_STATES)],
        "matches_played": 0,
        "wins": 0,
        "losses": 0,
        "draws": 0
    }
    save_state(state)
    return state

def save_state(state):
    with open(STATE_JSON_PATH, "w") as f:
        json.dump(state, f, indent=2)

def export_policy_header(state, enable_trace=1):
    lines = []
    lines.append("#ifndef POLICY_MATRIX_H")
    lines.append("#define POLICY_MATRIX_H")
    lines.append("")
    lines.append("#include <stdbool.h>")
    lines.append("")
    lines.append(f"#define MIN_MATRIX_CONFIDENCE {MIN_MATRIX_CONFIDENCE}")
    lines.append(f"#define ENABLE_MATRIX_TRACE {enable_trace}")
    lines.append("")
    lines.append("// Roles")
    lines.append("#define ROLE_CORE 0")
    lines.append("#define ROLE_HUNTER 1")
    lines.append("#define ROLE_CARRIER 2")
    lines.append("#define ROLE_MINER 3")
    lines.append("")
    lines.append("// Core Spawner Actions (5): RUNNER=0, DIGGER=1, BRAWLER=2, SKIRMISHER=3, FINISHER=4. States: 64.")
    lines.append("#define CORE_ACTION_RUNNER 0")
    lines.append("#define CORE_ACTION_DIGGER 1")
    lines.append("#define CORE_ACTION_BRAWLER 2")
    lines.append("#define CORE_ACTION_SKIRMISHER 3")
    lines.append("#define CORE_ACTION_FINISHER 4")
    lines.append(f"#define NUM_CORE_ACTIONS {NUM_CORE_ACTIONS}")
    lines.append(f"#define NUM_CORE_STATES {NUM_CORE_STATES}")
    lines.append("")
    lines.append("// Hunter Combat Actions (5): SHARED=0, LOWEST_HP=1, WORKER=2, HOLD_D2=3, GUARD_LANE=4. States: 32.")
    lines.append("#define HUNTER_ACTION_SHARED 0")
    lines.append("#define HUNTER_ACTION_LOWEST_HP 1")
    lines.append("#define HUNTER_ACTION_WORKER 2")
    lines.append("#define HUNTER_ACTION_HOLD_D2 3")
    lines.append("#define HUNTER_ACTION_GUARD_LANE 4")
    lines.append(f"#define NUM_HUNTER_ACTIONS {NUM_HUNTER_ACTIONS}")
    lines.append(f"#define NUM_HUNTER_STATES {NUM_HUNTER_STATES}")
    lines.append("")
    lines.append("#ifdef MATRIX_DATA_DEFINITION")

    # Core Q
    lines.append(f"float g_core_q[NUM_CORE_STATES][NUM_CORE_ACTIONS] = {{")
    for s in range(NUM_CORE_STATES):
        row = ", ".join(f"{val:.4f}f" for val in state["core_q"][s])
        comma = "," if s < NUM_CORE_STATES - 1 else ""
        lines.append(f"\t{{{row}}}{comma} // State {s}")
    lines.append("};")
    lines.append("")

    # Core Visits
    lines.append(f"int g_core_visits[NUM_CORE_STATES] = {{")
    v_lines = []
    for chunk in range(0, NUM_CORE_STATES, 16):
        row = ", ".join(str(state["core_visits"][i]) for i in range(chunk, min(chunk + 16, NUM_CORE_STATES)))
        comma = "," if chunk + 16 < NUM_CORE_STATES else ""
        v_lines.append(f"\t{row}{comma}")
    lines.extend(v_lines)
    lines.append("};")
    lines.append("")

    # Hunter Q
    lines.append(f"float g_hunter_q[NUM_HUNTER_STATES][NUM_HUNTER_ACTIONS] = {{")
    for s in range(NUM_HUNTER_STATES):
        row = ", ".join(f"{val:.4f}f" for val in state["hunter_q"][s])
        comma = "," if s < NUM_HUNTER_STATES - 1 else ""
        lines.append(f"\t{{{row}}}{comma} // State {s}")
    lines.append("};")
    lines.append("")

    # Hunter Visits
    lines.append(f"int g_hunter_visits[NUM_HUNTER_STATES] = {{")
    hv_lines = []
    for chunk in range(0, NUM_HUNTER_STATES, 16):
        row = ", ".join(str(state["hunter_visits"][i]) for i in range(chunk, min(chunk + 16, NUM_HUNTER_STATES)))
        comma = "," if chunk + 16 < NUM_HUNTER_STATES else ""
        hv_lines.append(f"\t{row}{comma}")
    lines.extend(hv_lines)
    lines.append("};")

    lines.append("#else")
    lines.append("extern float g_core_q[NUM_CORE_STATES][NUM_CORE_ACTIONS];")
    lines.append("extern int g_core_visits[NUM_CORE_STATES];")
    lines.append("extern float g_hunter_q[NUM_HUNTER_STATES][NUM_HUNTER_ACTIONS];")
    lines.append("extern int g_hunter_visits[NUM_HUNTER_STATES];")
    lines.append("#endif")
    lines.append("")
    lines.append("#endif /* POLICY_MATRIX_H */")
    lines.append("")

    with open(HEADER_PATH, "w") as f:
        f.write("\n".join(lines))
    print(f"[TRAIN] Exported policy header to {HEADER_PATH}")

def recompile_bot():
    cmd = ["make", "-C", MATRIX_DIR]
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        print("[TRAIN ERROR] Compilation failed!")
        print(res.stderr)
        sys.exit(1)

def parse_mtrace(log_content):
    pattern = re.compile(r"\[MTRACE\] R=(\d+) S=(\d+) A=(\d+) T=(\d+)")
    core_events = []
    hunter_events = []

    for line in log_content.splitlines():
        m = pattern.search(line)
        if m:
            r = int(m.group(1))
            s = int(m.group(2))
            a = int(m.group(3))
            t = int(m.group(4))
            if r == 0 and s < NUM_CORE_STATES and a < NUM_CORE_ACTIONS:
                core_events.append((s, a, t))
            elif r == 1 and s < NUM_HUNTER_STATES and a < NUM_HUNTER_ACTIONS:
                hunter_events.append((s, a, t))

    return core_events, hunter_events

def compute_reward(match_res, is_p1):
    winner_str = match_res.get("winner", "")
    ticks = match_res.get("ticks", 10000)

    we_won = ("Bot1" in winner_str and is_p1) or ("Bot2" in winner_str and not is_p1)
    opp_won = ("Bot2" in winner_str and is_p1) or ("Bot1" in winner_str and not is_p1)

    if we_won:
        # Speed-weighted terminal return: +100 + int(50 * max(0, 2500 - ticks) / 2500)
        speed_bonus = int(50.0 * max(0, 2500 - ticks) / 2500.0)
        return float(100 + speed_bonus)
    elif opp_won:
        # Loss cushion based on survival ticks: -100 + int(40 * min(2500, ticks) / 2500)
        cushion = int(40.0 * min(2500, ticks) / 2500.0)
        return float(-100 + cushion)
    else:
        return 0.0

def train(num_matches=90, alpha=0.2, opponents=None, reset=False):
    if not opponents:
        opponents = DEFAULT_OPPONENTS

    state = load_or_init_state(reset)
    export_policy_header(state, enable_trace=1)
    recompile_bot()

    print(f"\n==========================================")
    print(f"Starting Q-Matrix Training: {num_matches} Matches")
    print(f"Alpha: {alpha}, Reset: {reset}")
    print(f"Opponent Pool ({len(opponents)}):")
    for opp in opponents:
        print(f"  - {opp}")
    print(f"Matrix Bot: bots/track2_matrix/bot")
    print(f"==========================================\n")

    for match_idx in range(1, num_matches + 1):
        # Rotate opponent and sides: each opponent gets P1 then P2
        pair_idx = (match_idx - 1) // 2
        opp_idx = pair_idx % len(opponents)
        current_opp = opponents[opp_idx]

        is_p1 = (match_idx % 2 != 0)

        tag = f"train_m{match_idx}"
        p1_bot = "bots/track2_matrix/bot" if is_p1 else current_opp
        p2_bot = current_opp if is_p1 else "bots/track2_matrix/bot"

        side_str = "P1" if is_p1 else "P2"
        print(f"--- MATCH {match_idx}/{num_matches}: Track 2 ({side_str}) vs {current_opp} ---")
        t0 = time.time()
        match_res = run_match(p1_bot, p2_bot, p1_id=42, p2_id=43, tag=tag)
        duration = round(time.time() - t0, 2)

        if "error" in match_res and match_res["error"] != "Timeout":
            print(f"[ERROR] Match failed: {match_res['error']}")
            continue

        log_path = f"scratch_replays/bot1_{tag}.log" if is_p1 else f"scratch_replays/bot2_{tag}.log"
        if not os.path.exists(log_path):
            print(f"[WARN] Log file {log_path} not found!")
            continue

        with open(log_path, "r") as f:
            bot_log = f.read()

        core_events, hunter_events = parse_mtrace(bot_log)
        reward = compute_reward(match_res, is_p1)
        my_af = match_res.get("p1_af", 0) if is_p1 else match_res.get("p2_af", 0)
        if my_af > 0:
            print(f"  [CRITICAL ALERT] Action Failure detected: {my_af} AFs in match {match_idx}!")

        we_won = ("Bot1" in match_res.get("winner", "") and is_p1) or ("Bot2" in match_res.get("winner", "") and not is_p1)
        opp_won = ("Bot2" in match_res.get("winner", "") and is_p1) or ("Bot1" in match_res.get("winner", "") and not is_p1)
        outcome_str = "WIN" if we_won else ("LOSS" if opp_won else "DRAW")

        state["matches_played"] += 1
        if we_won:
            state["wins"] += 1
        elif opp_won:
            state["losses"] += 1
        else:
            state["draws"] += 1

        # Opening Credit Protection: If match is a Loss but lasted ticks >= 1000,
        # skip updating (do not penalize) trace entries recorded at T <= 250
        skip_opening_penalty = (opp_won and match_res.get("ticks", 0) >= 1000)

        visited_core_states = set()
        visited_core_sa = set()
        for s, a, t in core_events:
            if skip_opening_penalty and t <= 250:
                continue
            visited_core_states.add(s)
            visited_core_sa.add((s, a))

        visited_hunter_states = set()
        visited_hunter_sa = set()
        for s, a, t in hunter_events:
            if skip_opening_penalty and t <= 250:
                continue
            visited_hunter_states.add(s)
            visited_hunter_sa.add((s, a))

        # Update visit counts per state
        for s in visited_core_states:
            state["core_visits"][s] += 1
        for s in visited_hunter_states:
            state["hunter_visits"][s] += 1

        # Update Q(s, a) <- Q(s, a) + alpha * (R - Q(s, a))
        core_q_deltas = []
        for s, a in visited_core_sa:
            old_q = state["core_q"][s][a]
            new_q = old_q + alpha * (reward - old_q)
            state["core_q"][s][a] = round(new_q, 4)
            core_q_deltas.append(abs(new_q - old_q))

        hunter_q_deltas = []
        for s, a in visited_hunter_sa:
            old_q = state["hunter_q"][s][a]
            new_q = old_q + alpha * (reward - old_q)
            state["hunter_q"][s][a] = round(new_q, 4)
            hunter_q_deltas.append(abs(new_q - old_q))

        confident_core = sum(1 for v in state["core_visits"] if v >= MIN_MATRIX_CONFIDENCE)
        confident_hunter = sum(1 for v in state["hunter_visits"] if v >= MIN_MATRIX_CONFIDENCE)

        print(f"  Result: {outcome_str} (Reward: {reward:.1f}) | Ticks: {match_res.get('ticks', 0)} | AF: {my_af} | Opp: {current_opp}")
        print(f"  Core Events: {len(core_events)} ({len(visited_core_sa)} updated) | Confident: {confident_core}/{NUM_CORE_STATES}")
        print(f"  Hunter Events: {len(hunter_events)} ({len(visited_hunter_sa)} updated) | Confident: {confident_hunter}/{NUM_HUNTER_STATES}")
        if skip_opening_penalty:
            print(f"  [CREDIT PROTECTION] Protected T<=250 opening decisions from late-game loss penalty.")
        if core_q_deltas:
            print(f"  Max Core Q-delta: {max(core_q_deltas):.2f}")
        if hunter_q_deltas:
            print(f"  Max Hunter Q-delta: {max(hunter_q_deltas):.2f}")

        # Save and Re-export
        save_state(state)
        export_policy_header(state, enable_trace=1)
        recompile_bot()

    total = state["matches_played"]
    print(f"\n==========================================")
    print(f"Training Complete! Summary:")
    print(f"Total Matches: {total} (W: {state['wins']}, L: {state['losses']}, D: {state['draws']})")
    print(f"Confident Core States: {sum(1 for v in state['core_visits'] if v >= MIN_MATRIX_CONFIDENCE)}/{NUM_CORE_STATES}")
    print(f"Confident Hunter States: {sum(1 for v in state['hunter_visits'] if v >= MIN_MATRIX_CONFIDENCE)}/{NUM_HUNTER_STATES}")
    print(f"==========================================\n")

    # Freeze policy for zero-latency tournament execution
    export_policy_header(state, enable_trace=0)
    recompile_bot()
    print("[TRAIN] Frozen tournament bot recompiled with ENABLE_MATRIX_TRACE 0.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Train Track 2 Matrix Q-Brain")
    parser.add_argument("--matches", type=int, default=90, help="Number of training matches")
    parser.add_argument("--alpha", type=float, default=0.2, help="Learning rate alpha")
    parser.add_argument("--reset", action="store_true", help="Reset matrix state to zero")
    args = parser.parse_args()

    train(num_matches=args.matches, alpha=args.alpha, reset=args.reset)
