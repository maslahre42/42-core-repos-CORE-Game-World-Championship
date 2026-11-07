#!/usr/bin/env python3
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from isolated_scrimmage import run_match

def main():
    bot_v6 = "bots/candidate_v6/bot"
    bot_v5 = "my-core-bot/bot"
    
    total_matches = 20
    results = []
    
    print(f"=== STARTING 20-MATCH BENCHMARK: Candidate v6 vs Candidate/Champion v5 ===")
    print(f"Total Matches: {total_matches} (10 v6 as P1, 10 v6 as P2)\n")
    
    for i in range(1, total_matches + 1):
        if i % 2 == 1:
            p1_path = bot_v6
            p2_path = bot_v5
            p1_name = "candidate_v6"
            p2_name = "my_core_bot"
        else:
            p1_path = bot_v5
            p2_path = bot_v6
            p1_name = "my_core_bot"
            p2_name = "candidate_v6"
            
        tag = f"v6_v5_{i:02d}"
        print(f"[{i}/{total_matches}] Match {tag}: P1={p1_name} vs P2={p2_name} ...", flush=True)
        t_start = time.time()
        res = run_match(p1_path, p2_path, 42, 43, timeout=120, tag=tag)
        dur = time.time() - t_start
        
        w_place = res.get("winner_place", -1)
        if w_place == 0:
            actual_winner = p1_name
            winning_side = "P1"
        elif w_place == 1:
            actual_winner = p2_name
            winning_side = "P2"
        else:
            actual_winner = "Draw"
            winning_side = "None"
            
        ticks = res.get("ticks", 0)
        p1_af = res.get("p1_af", 0)
        p2_af = res.get("p2_af", 0)
        
        v6_af = p1_af if p1_name == "candidate_v6" else p2_af
        v5_af = p2_af if p1_name == "candidate_v6" else p1_af
        
        # Read death reason and core destroyed from replay if available
        death_reason = None
        cores_destroyed = None
        try:
            with open(f"scratch_replays/replay_{tag}.json") as rf:
                rd = json.load(rf)
                tres = rd.get("misc", {}).get("team_results", [])
                stats = rd.get("misc", {}).get("stats", {})
                loser_res = next((r for r in tres if r.get("place") == 1), {})
                death_reason = loser_res.get("death_reason")
                cores_destroyed = stats.get("cores_destroyed")
        except Exception:
            pass
        
        match_record = {
            "match_num": i,
            "tag": tag,
            "p1": p1_name,
            "p2": p2_name,
            "winner": actual_winner,
            "winning_side": winning_side,
            "ticks": ticks,
            "duration_sec": round(dur, 2),
            "p1_af": p1_af,
            "p2_af": p2_af,
            "v6_af": v6_af,
            "v5_af": v5_af,
            "death_reason": death_reason,
            "cores_destroyed": cores_destroyed,
            "p1_core_dmg": res.get("p1_core_dmg", 0),
            "p2_core_dmg": res.get("p2_core_dmg", 0),
            "p1_unit_dmg": res.get("p1_unit_dmg", 0),
            "p2_unit_dmg": res.get("p2_unit_dmg", 0),
            "p1_units_killed": res.get("p1_units_killed", 0),
            "p2_units_killed": res.get("p2_units_killed", 0)
        }
        results.append(match_record)
        
        print(f"    -> Winner: {actual_winner} ({winning_side}) | Ticks: {ticks} | AF: v6={v6_af}, v5={v5_af} | DeathReason: {death_reason} ({dur:.1f}s)", flush=True)
        
        with open("scripts/benchmark_v6_vs_v5_results.json", "w") as f:
            json.dump(results, f, indent=2)
            
    v6_wins = sum(1 for r in results if r["winner"] == "candidate_v6")
    v5_wins = sum(1 for r in results if r["winner"] == "my_core_bot")
    draws = sum(1 for r in results if r["winner"] == "Draw")
    
    v6_p1_matches = [r for r in results if r["p1"] == "candidate_v6"]
    v6_p2_matches = [r for r in results if r["p2"] == "candidate_v6"]
    
    v6_p1_wins = sum(1 for r in v6_p1_matches if r["winner"] == "candidate_v6")
    v6_p2_wins = sum(1 for r in v6_p2_matches if r["winner"] == "candidate_v6")
    
    v5_p1_wins = sum(1 for r in v6_p2_matches if r["winner"] == "my_core_bot")
    v5_p2_wins = sum(1 for r in v6_p1_matches if r["winner"] == "my_core_bot")
    
    ticks_list = [r["ticks"] for r in results]
    sorted_ticks = sorted(ticks_list)
    avg_ticks = sum(ticks_list) / len(ticks_list) if ticks_list else 0
    med_ticks = sorted_ticks[len(sorted_ticks)//2] if sorted_ticks else 0
    
    # Decisive ticks (excluding timeouts)
    decisive_ticks = [r["ticks"] for r in results if r["ticks"] < 10000]
    avg_decisive = sum(decisive_ticks) / len(decisive_ticks) if decisive_ticks else 0
    med_decisive = sorted(decisive_ticks)[len(decisive_ticks)//2] if decisive_ticks else 0
    
    total_v6_af = sum(r["v6_af"] for r in results)
    total_v5_af = sum(r["v5_af"] for r in results)
    
    summary = {
        "total_matches": total_matches,
        "candidate_v6_wins": v6_wins,
        "my_core_bot_wins": v5_wins,
        "draws": draws,
        "candidate_v6_win_rate": f"{(v6_wins/total_matches)*100:.1f}%",
        "my_core_bot_win_rate": f"{(v5_wins/total_matches)*100:.1f}%",
        "v6_p1_record": f"{v6_p1_wins}/{len(v6_p1_matches)} ({(v6_p1_wins/len(v6_p1_matches))*100:.1f}%)",
        "v6_p2_record": f"{v6_p2_wins}/{len(v6_p2_matches)} ({(v6_p2_wins/len(v6_p2_matches))*100:.1f}%)",
        "v5_p1_record": f"{v5_p1_wins}/{len(v6_p2_matches)} ({(v5_p1_wins/len(v6_p2_matches))*100:.1f}%)",
        "v5_p2_record": f"{v5_p2_wins}/{len(v6_p1_matches)} ({(v5_p2_wins/len(v6_p1_matches))*100:.1f}%)",
        "avg_ticks_all": round(avg_ticks, 1),
        "median_ticks_all": med_ticks,
        "avg_ticks_decisive": round(avg_decisive, 1),
        "median_ticks_decisive": med_decisive,
        "min_ticks": min(ticks_list) if ticks_list else 0,
        "max_ticks": max(ticks_list) if ticks_list else 0,
        "total_v6_af": total_v6_af,
        "total_v5_af": total_v5_af,
        "core_kills": sum(1 for r in results if r["death_reason"] == 1)
    }
    
    final_output = {
        "summary": summary,
        "matches": results
    }
    
    with open("scripts/benchmark_v6_vs_v5_results.json", "w") as f:
        json.dump(final_output, f, indent=2)
        
    print("\n" + "="*60)
    print("=== 20-MATCH BENCHMARK SUMMARY: Candidate v6 vs Candidate/Champion v5 ===")
    print("="*60)
    print(f"Candidate v6 Wins:   {v6_wins}/{total_matches} ({summary['candidate_v6_win_rate']})")
    print(f"Champion v5 Wins:    {v5_wins}/{total_matches} ({summary['my_core_bot_win_rate']})")
    print(f"Draws:               {draws}/{total_matches}")
    print(f"Candidate v6 as P1:  {summary['v6_p1_record']}")
    print(f"Candidate v6 as P2:  {summary['v6_p2_record']}")
    print(f"Champion v5 as P1:   {summary['v5_p1_record']}")
    print(f"Champion v5 as P2:   {summary['v5_p2_record']}")
    print(f"Ticks (All) - Avg: {avg_ticks:.1f} | Median: {med_ticks} | Min: {summary['min_ticks']} | Max: {summary['max_ticks']}")
    print(f"Ticks (Decisive) - Avg: {avg_decisive:.1f} | Median: {med_decisive} (Core kills: {summary['core_kills']}/{total_matches})")
    print(f"Action Failures - Candidate v6: {total_v6_af} | Champion v5: {total_v5_af}")
    print("="*60 + "\n")

if __name__ == "__main__":
    main()
