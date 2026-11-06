#!/usr/bin/env python3
"""
42 CORE Autonomous Evolutionary Trainer (Self-Play / Hall-of-Fame Gauntlet)

Usage:
    # Initialize the training environment & save Reigning Champion:
    python3 scripts/evolve.py --init-only

    # Run evolutionary self-play training loop (e.g., 20 generations):
    python3 scripts/evolve.py --generations 20 --matches-per-eval 1
"""

import argparse
import copy
import json
import os
import random
import shutil
import subprocess
import sys
import time

CONTAINER_NAME = "default-my-706fc-my-core-bot-1"
HEADER_PATH = "my-core-bot/inc/tuned_params.h"
HOF_DIR = "hall_of_fame"
BEST_GENOME_PATH = os.path.join(HOF_DIR, "best_genome.json")

PARAM_SPACE = {
    "MAX_CARRIERS": (1, 2),
    "TARGET_MINERS": (1, 3),
    "MAX_MINERS": (2, 4),
    "CARRIER_EARLY_TICKS": (120, 300),
    "CARRIER_EARLY_THRESHOLD": (15, 40),
    "CARRIER_MID_THRESHOLD": (30, 80),
    "CARRIER_MINER_PICKUP_MIN": (8, 30),
    "MINER_SOLO_RETURN_GEMS": (30, 65),
    "RES_DIST_UNIT_WEIGHT": (1, 4),
    "RES_DIST_CORE_WEIGHT": (1, 3),
    "RES_CLAIMED_PENALTY": (60, 200),
    "RES_PILE_NEAR_RADIUS": (3, 8),
    "CORE_THREAT_RADIUS": (6, 11),
    "DEFENDER_INTERCEPT_RADIUS": (5, 9),
    "HUNTER_UNARMED_BOUNTY": (2000, 7000),
    "HUNTER_GEM_CARRIER_BOUNTY": (400, 1500),
    "HUNTER_LOW_HP_WEIGHT": (20, 80),
    "HUNTER_CORE_THREAT_BOUNTY": (600, 2500),
    "SQUAD_RALLY_SIZE": (1, 3),
    "SQUAD_RALLY_DIST": (4, 8),
    "HUNTER_LOADOUT": (0, 1),
    "STRIKER_LOADOUT": (0, 1),
    "STRIKER_TO_HUNTER_RATIO": (1, 4),
}

BASELINE_GENOME = {
    "MAX_CARRIERS": 1,
    "TARGET_MINERS": 2,
    "MAX_MINERS": 3,
    "CARRIER_EARLY_TICKS": 200,
    "CARRIER_EARLY_THRESHOLD": 29,
    "CARRIER_MID_THRESHOLD": 39,
    "CARRIER_MINER_PICKUP_MIN": 12,
    "MINER_SOLO_RETURN_GEMS": 45,
    "RES_DIST_UNIT_WEIGHT": 2,
    "RES_DIST_CORE_WEIGHT": 2,
    "RES_CLAIMED_PENALTY": 120,
    "RES_PILE_NEAR_RADIUS": 6,
    "CORE_THREAT_RADIUS": 8,
    "DEFENDER_INTERCEPT_RADIUS": 7,
    "HUNTER_UNARMED_BOUNTY": 5000,
    "HUNTER_GEM_CARRIER_BOUNTY": 919,
    "HUNTER_LOW_HP_WEIGHT": 40,
    "HUNTER_CORE_THREAT_BOUNTY": 1200,
    "SQUAD_RALLY_SIZE": 2,
    "SQUAD_RALLY_DIST": 6,
    "HUNTER_LOADOUT": 1,
    "STRIKER_LOADOUT": 1,
    "STRIKER_TO_HUNTER_RATIO": 2,
}

def run_in_container(cmd, timeout=120):
    full_cmd = ["docker", "exec", CONTAINER_NAME, "bash", "-c", cmd]
    return subprocess.run(full_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout)

def write_genome(genome, path=HEADER_PATH):
    lines = [
        "// Auto-generated evolutionary genome",
        "#ifndef TUNED_PARAMS_H",
        "#define TUNED_PARAMS_H",
        ""
    ]
    for k, v in genome.items():
        lines.append(f"#define {k} {v}")
    lines.append("\n#endif /* TUNED_PARAMS_H */\n")
    with open(path, "w") as f:
        f.write("\n".join(lines))

def mutate_genome(parent, mutation_rate=0.35):
    child = copy.deepcopy(parent)
    mutated_keys = []
    for k, (low, high) in PARAM_SPACE.items():
        if random.random() < mutation_rate:
            span = max(1, int((high - low) * 0.25))
            delta = random.randint(-span, span)
            child[k] = max(low, min(high, child[k] + delta))
            mutated_keys.append(f"{k}={child[k]}")
    return child, mutated_keys

def build_bot(container=True):
    if container:
        res = run_in_container("make -C /workspace/my-core-bot re")
        return res.returncode == 0
    else:
        res = subprocess.run(["make", "-C", "my-core-bot", "re"], capture_output=True, text=True)
        return res.returncode == 0

def run_single_scrimmage(bot1_rel, bot2_rel, candidate_role="bot1", p1_id=42, p2_id=43):
    """Executes a single match between bot1 and bot2 inside the container."""
    run_in_container("pkill -9 server; pkill -9 bot; pkill -9 gridmaster; true")
    time.sleep(0.2)

    match_cmd = (
        f"/core/server /workspace/configs/server.config.json /workspace/configs/game.config.json /core/data {p1_id} {p2_id} > /tmp/server.log 2>&1 & "
        f"sleep 0.2; "
        f"/workspace/{bot2_rel} {p2_id} > /tmp/bot2.log 2>&1 & "
        f"sleep 0.1; "
        f"/workspace/{bot1_rel} {p1_id} > /tmp/bot1.log 2>&1"
    )

    try:
        run_in_container(match_cmd, timeout=45)
    except subprocess.TimeoutExpired:
        run_in_container("pkill -9 server; pkill -9 bot; pkill -9 gridmaster; true")
        return {"winner": "timeout", "ticks": 10000, "af": 999, "core_dmg": 0}

    # Accurately inspect candidate's log
    candidate_log_file = "/tmp/bot1.log" if candidate_role == "bot1" else "/tmp/bot2.log"
    cand_log = run_in_container(f"cat {candidate_log_file}").stdout
    action_failures = cand_log.count("Action Failure")

    replay_res = run_in_container("cat /workspace/replays/replay_latest.json")
    if replay_res.returncode != 0 or not replay_res.stdout.strip():
        return {"winner": "crash", "ticks": 0, "af": action_failures, "core_dmg": 0}

    try:
        replay = json.loads(replay_res.stdout)
        results = replay.get("misc", {}).get("team_results", [])
        stats = replay.get("misc", {}).get("stats", {})
        full_ticks = replay.get("full_tick_amount", 0)

        p1_res = next((r for r in results if r.get("id") == p1_id), {})
        is_p1_win = (p1_res.get("place") == 0)

        return {
            "winner": "bot1" if is_p1_win else "bot2",
            "ticks": full_ticks,
            "af": action_failures,
            "core_dmg": stats.get("damage_cores", 0),
            "gems_transferred": stats.get("gems_transferred", 0)
        }
    except Exception:
        return {"winner": "error", "ticks": 0, "af": action_failures, "core_dmg": 0}

def evaluate_candidate(opponents, rounds=1):
    """
    Evaluates candidate my-core-bot/bot against a list of opponent binaries.
    Returns normalized composite fitness score, wins, matches, and action failures.
    """
    raw_score = 0
    total_wins = 0
    total_matches = 0
    total_af = 0

    for opp in opponents:
        for _ in range(rounds):
            # Test as P1
            res1 = run_single_scrimmage("my-core-bot/bot", opp, candidate_role="bot1", p1_id=42, p2_id=43)
            total_matches += 1
            total_af += res1["af"]

            if res1["winner"] == "bot1":
                total_wins += 1
                win_bonus = 1500
                speed_bonus = max(0, 5000 - res1["ticks"]) // 2
                raw_score += win_bonus + speed_bonus
            else:
                raw_score += res1["core_dmg"] * 10

            # Test side-swapped as P2
            res2 = run_single_scrimmage(opp, "my-core-bot/bot", candidate_role="bot2", p1_id=42, p2_id=43)
            total_matches += 1
            total_af += res2["af"]

            if res2["winner"] == "bot2":
                total_wins += 1
                win_bonus = 1500
                speed_bonus = max(0, 5000 - res2["ticks"]) // 2
                raw_score += win_bonus + speed_bonus
            else:
                raw_score += (100 - res2.get("core_dmg", 0)) * 5

    # Normalized average score per match
    avg_score = int(raw_score / total_matches) if total_matches > 0 else 0
    # Penalty for action failures
    avg_score -= (total_af * 100)
    return avg_score, total_wins, total_matches, total_af

def setup_environment():
    """Sets up hall_of_fame directory and ensures baseline / champion models exist."""
    os.makedirs(HOF_DIR, exist_ok=True)
    os.makedirs("replays", exist_ok=True)

    # 1. Load existing best genome if present, otherwise initialize with baseline
    if os.path.exists(BEST_GENOME_PATH):
        print(f"📦 Loading existing best genome from {BEST_GENOME_PATH}...")
        with open(BEST_GENOME_PATH) as f:
            active_genome = json.load(f)
        for k, v in BASELINE_GENOME.items():
            if k not in active_genome:
                active_genome[k] = v
    else:
        print("📦 Initializing with baseline genome...")
        active_genome = copy.deepcopy(BASELINE_GENOME)

    write_genome(active_genome)

    # 2. Build my-core-bot with active genome
    if not build_bot(container=True):
        print("❌ Initial build failed for my-core-bot!")
        sys.exit(1)

    # 3. Always maintain champion_best as the reigning champion binary for true self-play
    champ_best_path = os.path.join(HOF_DIR, "champion_best")
    shutil.copy("my-core-bot/bot", champ_best_path)
    print(f"✅ Reigning champion binary preserved at {champ_best_path}")

    # Ensure champion_gen0 exists
    gen0_path = os.path.join(HOF_DIR, "champion_gen0")
    if not os.path.exists(gen0_path):
        shutil.copy("my-core-bot/bot", gen0_path)
        print(f"✅ Gen 0 Champion saved to {gen0_path}")

    # Copy baseline gridmaster if available
    if os.path.exists("gridmaster/gridmaster"):
        gm_target = os.path.join(HOF_DIR, "gridmaster")
        if not os.path.exists(gm_target):
            shutil.copy("gridmaster/gridmaster", gm_target)
            print(f"✅ Gridmaster benchmark saved to {gm_target}")

    # Copy Bot Beta if available
    if os.path.exists("bots/bot_beta/bot"):
        beta_target = os.path.join(HOF_DIR, "bot_beta")
        if not os.path.exists(beta_target):
            shutil.copy("bots/bot_beta/bot", beta_target)
            print(f"✅ Bot Beta (Citadel) saved to {beta_target}")

    # Copy opponent_speed_brawler if available
    if os.path.exists("bots/opponent_speed_brawler/bot"):
        brawler_target = os.path.join(HOF_DIR, "opponent_speed_brawler")
        if not os.path.exists(brawler_target):
            shutil.copy("bots/opponent_speed_brawler/bot", brawler_target)
            print(f"✅ Opponent Speed Brawler clone saved to {brawler_target}")

    # Persist best genome
    with open(BEST_GENOME_PATH, "w") as f:
        json.dump(active_genome, f, indent=4)
    print(f"✅ Active best genome persisted to {BEST_GENOME_PATH}")
    return active_genome

def main():
    parser = argparse.ArgumentParser(description="42 CORE Evolutionary Parameter Optimizer")
    parser.add_argument("--init-only", action="store_true", help="Only setup the environment without running training")
    parser.add_argument("--generations", type=int, default=30, help="Number of evolutionary generations")
    parser.add_argument("--matches-per-eval", type=int, default=1, help="Rounds per opponent matchup")
    parser.add_argument("--mutation-rate", type=float, default=0.35, help="Probability of mutating each weight")
    args = parser.parse_args()

    current_best_genome = setup_environment()

    if args.init_only:
        print("\n🎉 Training environment initialized successfully!")
        print("To start training, run:")
        print(f"    python3 scripts/evolve.py --generations {args.generations} --matches-per-eval {args.matches_per_eval}\n")
        return

    # Opponent Gauntlet: Reigning Champion (Self-Play) + Opponent Speed Brawler + Gridmaster
    gauntlet = [f"{HOF_DIR}/champion_best"]
    if os.path.exists(f"{HOF_DIR}/opponent_speed_brawler"):
        gauntlet.append(f"{HOF_DIR}/opponent_speed_brawler")
    if os.path.exists(f"{HOF_DIR}/gridmaster"):
        gauntlet.append(f"{HOF_DIR}/gridmaster")
    elif os.path.exists("gridmaster/gridmaster"):
        gauntlet.append("gridmaster/gridmaster")

    print(f"\n⚔️  Gauntlet Opponents ({len(gauntlet)}): {', '.join(gauntlet)}")
    print("⚔️  Evaluating Reigning Champion against Gauntlet...")
    current_best_score, wins, matches, af = evaluate_candidate(gauntlet, rounds=args.matches_per_eval)
    print(f"🏆 Reigning Champion Average Fitness: {current_best_score} | Wins: {wins}/{matches} | AF: {af}\n")

    print(f"🚀 Starting Evolution Loop ({args.generations} Generations)...")
    print("=" * 70)

    for gen in range(1, args.generations + 1):
        candidate_genome, mutations = mutate_genome(current_best_genome, mutation_rate=args.mutation_rate)
        write_genome(candidate_genome)

        if not build_bot(container=True):
            print(f"[Gen {gen:03d}] ❌ Build failed after mutation. Reverting...")
            write_genome(current_best_genome)
            continue

        score, wins, matches, af = evaluate_candidate(gauntlet, rounds=args.matches_per_eval)
        is_improvement = (score > current_best_score)

        status_tag = "🌟 NEW CHAMPION!" if is_improvement else "❌ Rejected"
        print(f"[Gen {gen:03d}] Score: {score:5d} (Best: {current_best_score:5d}) | Wins: {wins}/{matches} | AF: {af:2d} | {status_tag}")
        if is_improvement:
            print(f"          Mutations: {', '.join(mutations)}")
            current_best_score = score
            current_best_genome = candidate_genome

            # Update reigning champion binary and save historical checkpoint
            shutil.copy("my-core-bot/bot", f"{HOF_DIR}/champion_best")
            champ_file = os.path.join(HOF_DIR, f"champion_gen{gen}")
            shutil.copy("my-core-bot/bot", champ_file)

            with open(BEST_GENOME_PATH, "w") as f:
                json.dump(current_best_genome, f, indent=4)

            # Re-baseline current_best_score against the newly strengthened gauntlet
            # so a single lucky map spike against an older, weaker champion never freezes evolution!
            current_best_score, _, _, _ = evaluate_candidate(gauntlet, rounds=args.matches_per_eval)
            print(f"          Re-baselined Champion Score vs Updated Gauntlet: {current_best_score}")
        else:
            # Revert to best known genome
            write_genome(current_best_genome)

    # Final build with the best genome
    print("\n" + "=" * 70)
    print("🏁 Evolution Completed!")
    write_genome(current_best_genome)
    build_bot(container=True)
    print(f"🏆 Best Genome persisted to {HEADER_PATH} and {BEST_GENOME_PATH}")
    print(f"Best Average Fitness Score: {current_best_score}\n")

if __name__ == "__main__":
    main()
