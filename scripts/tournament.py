#!/usr/bin/env python3
import argparse
import itertools
import json
import os
import subprocess
import sys
import time

CONTAINER_NAME = "default-my-706fc-my-core-bot-1"

def run_in_container(cmd, timeout=120):
    full_cmd = ["docker", "exec", CONTAINER_NAME, "bash", "-c", cmd]
    return subprocess.run(full_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout)

def build_bot(path):
    directory = os.path.dirname(path)
    if not directory:
        return True
    res = run_in_container(f"make -C /workspace/{directory}")
    if res.returncode != 0:
        print(f"❌ Build failed for {directory}:\n{res.stderr}\n{res.stdout}")
        return False
    return True

def run_single_match(bot1, bot2, p1_id=42, p2_id=43):
    run_in_container("pkill -9 server; pkill -9 bot; pkill -9 gridmaster; true")
    time.sleep(0.3)

    match_cmd = (
        f"/core/server /workspace/configs/server.config.json /workspace/configs/game.config.json /core/data {p1_id} {p2_id} > /tmp/server.log 2>&1 & "
        f"sleep 0.2; "
        f"/workspace/{bot2} {p2_id} > /tmp/bot2.log 2>&1 & "
        f"sleep 0.1; "
        f"/workspace/{bot1} {p1_id} > /tmp/bot1.log 2>&1"
    )

    t0 = time.time()
    try:
        run_in_container(match_cmd, timeout=60)
    except subprocess.TimeoutExpired:
        run_in_container("pkill -9 server; pkill -9 bot; pkill -9 gridmaster; true")
        return {"error": "Timeout", "winner": "None", "ticks": 10000, "p1_af": 999, "p2_af": 999}
    duration = time.time() - t0

    # Count action failures in bot logs
    bot1_log = run_in_container("cat /tmp/bot1.log").stdout
    bot2_log = run_in_container("cat /tmp/bot2.log").stdout
    p1_af = bot1_log.count("Action Failure")
    p2_af = bot2_log.count("Action Failure")

    # Parse replay
    replay_res = run_in_container("cat /workspace/replays/replay_latest.json")
    if replay_res.returncode != 0 or not replay_res.stdout.strip():
        return {"error": "No replay", "winner": "None", "ticks": 0, "p1_af": p1_af, "p2_af": p2_af}

    try:
        replay = json.loads(replay_res.stdout)
        results = replay.get("misc", {}).get("team_results", [])
        stats = replay.get("misc", {}).get("stats", {})
        full_ticks = replay.get("full_tick_amount", 0)

        p1_res = next((r for r in results if r.get("id") == p1_id), {})
        p2_res = next((r for r in results if r.get("id") == p2_id), {})

        if p1_res.get("place") == 0:
            winner = bot1
        elif p2_res.get("place") == 0:
            winner = bot2
        else:
            winner = "Draw"

        return {
            "winner": winner,
            "ticks": full_ticks,
            "duration": duration,
            "p1_af": p1_af,
            "p2_af": p2_af,
            "gems_gained": stats.get("gems_gained", 0),
            "gems_transferred": stats.get("gems_transferred", 0),
            "units_spawned": stats.get("units_spawned", 0),
            "cores_destroyed": stats.get("cores_destroyed", 0)
        }
    except Exception as e:
        return {"error": str(e), "winner": "None", "ticks": 0, "p1_af": p1_af, "p2_af": p2_af}

def main():
    parser = argparse.ArgumentParser(description="Multi-Agent Tournament League")
    parser.add_argument("--bots", nargs="+", default=["my-core-bot/bot", "gridmaster/gridmaster"], help="List of bots to compete")
    parser.add_argument("--rounds", type=int, default=1, help="Rounds per pairing")
    args = parser.parse_args()

    print("\n🏆 Building Tournament Contestants...")
    for b in args.bots:
        if not build_bot(b):
            sys.exit(1)

    pairs = list(itertools.combinations(args.bots, 2))
    scores = {b: {"wins": 0, "losses": 0, "draws": 0, "ticks_won": 0, "total_af": 0} for b in args.bots}

    print(f"\n⚡ Starting Tournament: {len(args.bots)} bots, {len(pairs) * args.rounds * 2} matches total\n")

    for round_idx in range(args.rounds):
        for b1, b2 in pairs:
            # Game 1: b1 as P1, b2 as P2
            res1 = run_single_match(b1, b2, 42, 43)
            w1 = res1.get("winner")
            scores[b1]["total_af"] += res1.get("p1_af", 0)
            scores[b2]["total_af"] += res1.get("p2_af", 0)

            if w1 == b1:
                scores[b1]["wins"] += 1
                scores[b1]["ticks_won"] += res1.get("ticks", 0)
                scores[b2]["losses"] += 1
                print(f"[{b1} vs {b2}] -> 🏆 {b1} WON in {res1.get('ticks')} ticks (AF: {res1.get('p1_af')} vs {res1.get('p2_af')})")
            elif w1 == b2:
                scores[b2]["wins"] += 1
                scores[b2]["ticks_won"] += res1.get("ticks", 0)
                scores[b1]["losses"] += 1
                print(f"[{b1} vs {b2}] -> 🏆 {b2} WON in {res1.get('ticks')} ticks (AF: {res1.get('p1_af')} vs {res1.get('p2_af')})")
            else:
                scores[b1]["draws"] += 1
                scores[b2]["draws"] += 1
                print(f"[{b1} vs {b2}] -> 🤝 DRAW in {res1.get('ticks')} ticks")

            # Game 2: swap sides: b2 as P1, b1 as P2
            res2 = run_single_match(b2, b1, 42, 43)
            w2 = res2.get("winner")
            scores[b2]["total_af"] += res2.get("p1_af", 0)
            scores[b1]["total_af"] += res2.get("p2_af", 0)

            if w2 == b2:
                scores[b2]["wins"] += 1
                scores[b2]["ticks_won"] += res2.get("ticks", 0)
                scores[b1]["losses"] += 1
                print(f"[{b2} vs {b1}] -> 🏆 {b2} WON in {res2.get('ticks')} ticks (AF: {res2.get('p1_af')} vs {res2.get('p2_af')})")
            elif w2 == b1:
                scores[b1]["wins"] += 1
                scores[b1]["ticks_won"] += res2.get("ticks", 0)
                scores[b2]["losses"] += 1
                print(f"[{b2} vs {b1}] -> 🏆 {b1} WON in {res2.get('ticks')} ticks (AF: {res2.get('p1_af')} vs {res2.get('p2_af')})")
            else:
                scores[b1]["draws"] += 1
                scores[b2]["draws"] += 1
                print(f"[{b2} vs {b1}] -> 🤝 DRAW in {res2.get('ticks')} ticks")

    # Rank contestants
    ranked = sorted(scores.items(), key=lambda kv: (kv[1]["wins"], -kv[1]["total_af"], -kv[1]["losses"]), reverse=True)

    print("\n" + "=" * 65)
    print(f"{'RANK':<5}{'BOT NAME':<30}{'W-L-D':<12}{'TOTAL AF':<10}")
    print("=" * 65)
    for idx, (bname, s) in enumerate(ranked, 1):
        record = f"{s['wins']}-{s['losses']}-{s['draws']}"
        print(f"{idx:<5}{bname:<30}{record:<12}{s['total_af']:<10}")
    print("=" * 65 + "\n")

if __name__ == "__main__":
    main()
