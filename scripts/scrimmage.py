#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import sys
import time

CONTAINER_NAME = "default-my-706fc-my-core-bot-1"

def run_in_container(cmd, timeout=120):
    full_cmd = ["docker", "exec", CONTAINER_NAME, "bash", "-c", cmd]
    return subprocess.run(full_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout)

def main():
    parser = argparse.ArgumentParser(description="Headless CORE Game Scrimmage Runner")
    parser.add_argument("--bot1", default="my-core-bot/bot", help="Path to bot 1 executable relative to /workspace")
    parser.add_argument("--bot2", default="gridmaster/gridmaster", help="Path to bot 2 executable relative to /workspace")
    parser.add_argument("--p1-id", type=int, default=42, help="Player 1 team ID")
    parser.add_argument("--p2-id", type=int, default=43, help="Player 2 team ID")
    parser.add_argument("--build", action="store_true", help="Rebuild bots before match")
    args = parser.parse_args()

    # Stop any lingering processes
    run_in_container("pkill -9 server; pkill -9 bot; pkill -9 gridmaster; true")
    time.sleep(0.5)

    if args.build:
        dir1 = os.path.dirname(args.bot1)
        dir2 = os.path.dirname(args.bot2)
        if dir1:
            print(f"Building {dir1}...")
            res = run_in_container(f"make -C /workspace/{dir1}")
            if res.returncode != 0:
                print(f"Build failed for {dir1}:\n{res.stderr}\n{res.stdout}")
                sys.exit(1)
        if dir2 and dir2 != dir1:
            print(f"Building {dir2}...")
            res = run_in_container(f"make -C /workspace/{dir2}")
            if res.returncode != 0:
                print(f"Build failed for {dir2}:\n{res.stderr}\n{res.stdout}")
                sys.exit(1)

    print(f"\n🎮 Starting Match: {args.bot1} (P1:{args.p1_id}) vs {args.bot2} (P2:{args.p2_id})")
    
    # Launch server and bot2 in background, then run bot1 in foreground
    match_cmd = (
        f"/core/server /workspace/configs/server.config.json /workspace/configs/game.config.json /core/data {args.p1_id} {args.p2_id} > /tmp/server.log 2>&1 & "
        f"sleep 0.2; "
        f"/workspace/{args.bot2} {args.p2_id} > /tmp/bot2.log 2>&1 & "
        f"sleep 0.1; "
        f"/workspace/{args.bot1} {args.p1_id} > /tmp/bot1.log 2>&1"
    )

    t0 = time.time()
    try:
        res = run_in_container(match_cmd, timeout=60)
    except subprocess.TimeoutExpired:
        print("❌ Match timed out!")
        run_in_container("pkill -9 server; pkill -9 bot; pkill -9 gridmaster; true")
        sys.exit(1)
    duration = time.time() - t0

    # Read bot1 logs for any action failures
    bot1_log_res = run_in_container("cat /tmp/bot1.log")
    bot1_log = bot1_log_res.stdout
    action_failures_p1 = bot1_log.count("Action Failure")

    # Read replay
    replay_res = run_in_container("cat /workspace/replays/replay_latest.json")
    if replay_res.returncode != 0 or not replay_res.stdout.strip():
        print("❌ Could not read replay_latest.json")
        print("Server log:\n", run_in_container("cat /tmp/server.log").stdout)
        print("Bot1 log:\n", bot1_log)
        sys.exit(1)

    try:
        replay = json.loads(replay_res.stdout)
        results = replay.get("misc", {}).get("team_results", [])
        stats = replay.get("misc", {}).get("stats", {})
        full_ticks = replay.get("full_tick_amount", 0)

        p1_result = next((r for r in results if r.get("id") == args.p1_id), {})
        p2_result = next((r for r in results if r.get("id") == args.p2_id), {})

        winner_name = "Unknown"
        if p1_result.get("place") == 0:
            winner_name = p1_result.get("name", "Player 1")
            status_icon = "🏆 WIN"
        elif p2_result.get("place") == 0:
            winner_name = p2_result.get("name", "Player 2")
            status_icon = "❌ LOSS"
        else:
            status_icon = "🤝 DRAW"

        print(f"\n================ MATCH SUMMARY ================")
        print(f"Result:             {status_icon} ({winner_name})")
        print(f"Duration:           {full_ticks} ticks ({duration:.2f}s)")
        print(f"P1 Action Failures: {action_failures_p1}")
        print(f"Total Gems Gained:  {stats.get('gems_gained', 0)}")
        print(f"Gems Transferred:   {stats.get('gems_transferred', 0)}")
        print(f"Deposits Destroyed: {stats.get('deposits_destroyed', 0)}")
        print(f"Units Spawned:      {stats.get('units_spawned', 0)}")
        print(f"Units Destroyed:    {stats.get('units_destroyed', 0)}")
        print(f"Damage Opponent:    {stats.get('damage_opponent', 0)}")
        print(f"Damage Cores:       {stats.get('damage_cores', 0)}")
        print(f"================================================\n")
    except Exception as e:
        print(f"Failed to parse replay json: {e}")
        print("Bot1 log:\n", bot1_log)

if __name__ == "__main__":
    main()
