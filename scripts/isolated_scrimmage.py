#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import sys
import time

IMAGE_NAME = "ghcr.io/42core-team/my-core-bot-c:v0.1.3.0"
WORKSPACE_DIR = "/goinfre/malshare/malshare/Core_game_world/CORE-Game-World-Championship-No_Plan_Just_Prayers"

def run_match(bot1_rel, bot2_rel, p1_id=42, p2_id=43, timeout=90, tag="m1"):
    container_script = f"""
rm -rf /tmp/replays /tmp/*.log
mkdir -p /tmp/replays
cat << 'EOF' > /tmp/server_iso.json
{{
    "replayFolderPaths": [
        "/tmp/replays"
    ],
    "timeoutTicks": 10000,
    "timeoutMs": 6000000,
    "clientWaitTimeoutMs": 3000,
    "clientConnectTimeoutMs": 60000,
    "clientPacketsMaxSizeKb": 16
}}
EOF

/core/server /tmp/server_iso.json /workspace/configs/game.config.json /core/data {p1_id} {p2_id} > /tmp/server.log 2>&1 &
sleep 0.2
/workspace/{bot2_rel} {p2_id} > /tmp/bot2.log 2>&1 &
sleep 0.1
/workspace/{bot1_rel} {p1_id} > /tmp/bot1.log 2>&1
for i in $(seq 1 30); do
    if [ -f /tmp/replays/replay_latest.json ]; then
        break
    fi
    sleep 0.2
done
sleep 0.5

cat /tmp/replays/replay_latest.json 2>/dev/null || true
echo "---SPLIT_BOT1---"
cat /tmp/bot1.log 2>/dev/null || true
echo "---SPLIT_BOT2---"
cat /tmp/bot2.log 2>/dev/null || true
echo "---SPLIT_SERVER---"
cat /tmp/server.log 2>/dev/null || true
"""
    cmd = [
        "docker", "run", "--rm",
        "-v", f"{WORKSPACE_DIR}:/workspace",
        "-w", "/workspace",
        IMAGE_NAME,
        "bash", "-c", container_script
    ]
    
    t0 = time.time()
    try:
        proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout)
        duration = time.time() - t0
    except subprocess.TimeoutExpired:
        return {"error": "Timeout", "winner": "Timeout", "ticks": 10000}

    out = proc.stdout
    parts = out.split("---SPLIT_BOT1---")
    replay_raw = parts[0].strip() if len(parts) > 0 else ""
    rest = parts[1] if len(parts) > 1 else ""
    
    parts2 = rest.split("---SPLIT_BOT2---")
    bot1_log = parts2[0] if len(parts2) > 0 else ""
    rest2 = parts2[1] if len(parts2) > 1 else ""
    
    parts3 = rest2.split("---SPLIT_SERVER---")
    bot2_log = parts3[0] if len(parts3) > 0 else ""
    server_log = parts3[1] if len(parts3) > 1 else ""

    if not replay_raw or not replay_raw.startswith("{"):
        return {
            "error": "No replay found",
            "server_log": server_log[-500:],
            "bot1_log": bot1_log[-500:],
            "bot2_log": bot2_log[-500:]
        }

    try:
        replay = json.loads(replay_raw)
    except Exception as e:
        return {"error": f"JSON decode error: {e}"}

    results = replay.get("misc", {}).get("team_results", [])
    full_ticks = replay.get("full_tick_amount", 0)
    p1_res = next((r for r in results if r.get("id") == p1_id), {})
    p2_res = next((r for r in results if r.get("id") == p2_id), {})

    af1 = bot1_log.count("Action Failure")
    af2 = bot2_log.count("Action Failure")

    os.makedirs("scratch_replays", exist_ok=True)
    with open(f"scratch_replays/replay_{tag}.json", "w") as f:
        f.write(replay_raw)
    with open("scratch_replays/replay_latest.json", "w") as f:
        f.write(replay_raw)
    with open(f"scratch_replays/bot1_{tag}.log", "w") as f:
        f.write(bot1_log)
    with open(f"scratch_replays/bot2_{tag}.log", "w") as f:
        f.write(bot2_log)
    with open(f"scratch_replays/server_{tag}.log", "w") as f:
        f.write(server_log)

    winner = "Draw"
    if p1_res.get("place") == 0:
        winner = f"Bot1 ({p1_res.get('name', 'P1')})"
    elif p2_res.get("place") == 0:
        winner = f"Bot2 ({p2_res.get('name', 'P2')})"

    return {
        "bot1": bot1_rel,
        "bot2": bot2_rel,
        "p1_name": p1_res.get("name", ""),
        "p2_name": p2_res.get("name", ""),
        "winner": winner,
        "winner_place": 0 if p1_res.get("place") == 0 else (1 if p2_res.get("place") == 0 else -1),
        "ticks": full_ticks,
        "duration_sec": round(duration, 2),
        "p1_core_dmg": p1_res.get("damage_dealt_core", 0),
        "p2_core_dmg": p2_res.get("damage_dealt_core", 0),
        "p1_unit_dmg": p1_res.get("damage_dealt_units", 0),
        "p2_unit_dmg": p2_res.get("damage_dealt_units", 0),
        "p1_units_killed": p1_res.get("units_killed", 0),
        "p2_units_killed": p2_res.get("units_killed", 0),
        "p1_af": af1,
        "p2_af": af2
    }

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--bot1", required=True)
    parser.add_argument("--bot2", required=True)
    parser.add_argument("--swap", action="store_true", help="Also run side-swapped match")
    args = parser.parse_args()

    print(f"--- MATCH 1: {args.bot1} (P1) vs {args.bot2} (P2) ---")
    r1 = run_match(args.bot1, args.bot2, 42, 43, tag="m1")
    print(json.dumps(r1, indent=2))

    if args.swap:
        print(f"\n--- MATCH 2 (SWAPPED): {args.bot2} (P1) vs {args.bot1} (P2) ---")
        r2 = run_match(args.bot2, args.bot1, 42, 43, tag="m2")
        print(json.dumps(r2, indent=2))
