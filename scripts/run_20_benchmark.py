#!/usr/bin/env python3
import json
import os
import sys
import time

# Ensure scripts dir is in python path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from isolated_scrimmage import run_match

def main():
    bot_matrix = "bots/track2_matrix/bot"
    bot_champion = "my-core-bot/bot"
    
    total_matches = 20
    results = []
    
    print(f"=== STARTING 20-MATCH BENCHMARK: Track 2 Matrix vs Champion v5 ===")
    print(f"Total Matches: {total_matches} (10 Matrix as P1, 10 Matrix as P2)\n")
    
    for i in range(1, total_matches + 1):
        # Odd: Matrix P1 vs Champion P2
        # Even: Champion P1 vs Matrix P2
        if i % 2 == 1:
            p1_path = bot_matrix
            p2_path = bot_champion
            p1_name = "track2_matrix"
            p2_name = "my_core_bot"
        else:
            p1_path = bot_champion
            p2_path = bot_matrix
            p1_name = "my_core_bot"
            p2_name = "track2_matrix"
            
        tag = f"sim_{i:02d}"
        print(f"[{i}/{total_matches}] Match {tag}: P1={p1_name} vs P2={p2_name} ...", flush=True)
        t_start = time.time()
        res = run_match(p1_path, p2_path, 42, 43, timeout=120, tag=tag)
        dur = time.time() - t_start
        
        # Determine actual winning bot name
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
        
        matrix_af = p1_af if p1_name == "track2_matrix" else p2_af
        champ_af = p2_af if p1_name == "track2_matrix" else p1_af
        
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
            "matrix_af": matrix_af,
            "champ_af": champ_af,
            "p1_core_dmg": res.get("p1_core_dmg", 0),
            "p2_core_dmg": res.get("p2_core_dmg", 0),
            "p1_unit_dmg": res.get("p1_unit_dmg", 0),
            "p2_unit_dmg": res.get("p2_unit_dmg", 0),
            "p1_units_killed": res.get("p1_units_killed", 0),
            "p2_units_killed": res.get("p2_units_killed", 0)
        }
        results.append(match_record)
        
        print(f"    -> Winner: {actual_winner} ({winning_side}) | Ticks: {ticks} | P1_AF: {p1_af} | P2_AF: {p2_af} ({dur:.1f}s)", flush=True)
        
        # Incremental save
        with open("scripts/benchmark_20_results.json", "w") as f:
            json.dump(results, f, indent=2)
            
    # Compute summary
    matrix_wins = sum(1 for r in results if r["winner"] == "track2_matrix")
    champ_wins = sum(1 for r in results if r["winner"] == "my_core_bot")
    draws = sum(1 for r in results if r["winner"] == "Draw")
    
    matrix_p1_matches = [r for r in results if r["p1"] == "track2_matrix"]
    matrix_p2_matches = [r for r in results if r["p2"] == "track2_matrix"]
    
    matrix_p1_wins = sum(1 for r in matrix_p1_matches if r["winner"] == "track2_matrix")
    matrix_p2_wins = sum(1 for r in matrix_p2_matches if r["winner"] == "track2_matrix")
    
    champ_p1_wins = sum(1 for r in matrix_p2_matches if r["winner"] == "my_core_bot")
    champ_p2_wins = sum(1 for r in matrix_p1_matches if r["winner"] == "my_core_bot")
    
    ticks_list = [r["ticks"] for r in results]
    sorted_ticks = sorted(ticks_list)
    avg_ticks = sum(ticks_list) / len(ticks_list) if ticks_list else 0
    med_ticks = sorted_ticks[len(sorted_ticks)//2] if sorted_ticks else 0
    
    total_matrix_af = sum(r["matrix_af"] for r in results)
    total_champ_af = sum(r["champ_af"] for r in results)
    
    summary = {
        "total_matches": total_matches,
        "track2_matrix_wins": matrix_wins,
        "my_core_bot_wins": champ_wins,
        "draws": draws,
        "track2_matrix_win_rate": f"{(matrix_wins/total_matches)*100:.1f}%",
        "my_core_bot_win_rate": f"{(champ_wins/total_matches)*100:.1f}%",
        "matrix_p1_record": f"{matrix_p1_wins}/{len(matrix_p1_matches)} ({(matrix_p1_wins/len(matrix_p1_matches))*100:.1f}%)",
        "matrix_p2_record": f"{matrix_p2_wins}/{len(matrix_p2_matches)} ({(matrix_p2_wins/len(matrix_p2_matches))*100:.1f}%)",
        "champ_p1_record": f"{champ_p1_wins}/{len(matrix_p2_matches)} ({(champ_p1_wins/len(matrix_p2_matches))*100:.1f}%)",
        "champ_p2_record": f"{champ_p2_wins}/{len(matrix_p1_matches)} ({(champ_p2_wins/len(matrix_p1_matches))*100:.1f}%)",
        "avg_ticks": round(avg_ticks, 1),
        "median_ticks": med_ticks,
        "min_ticks": min(ticks_list) if ticks_list else 0,
        "max_ticks": max(ticks_list) if ticks_list else 0,
        "total_matrix_af": total_matrix_af,
        "total_champ_af": total_champ_af
    }
    
    final_output = {
        "summary": summary,
        "matches": results
    }
    
    with open("scripts/benchmark_20_results.json", "w") as f:
        json.dump(final_output, f, indent=2)
        
    print("\n" + "="*60)
    print("=== 20-MATCH BENCHMARK SUMMARY ===")
    print("="*60)
    print(f"Track 2 Matrix Wins: {matrix_wins}/{total_matches} ({summary['track2_matrix_win_rate']})")
    print(f"Champion v5 Wins:    {champ_wins}/{total_matches} ({summary['my_core_bot_win_rate']})")
    print(f"Draws:               {draws}/{total_matches}")
    print(f"Track 2 Matrix as P1: {summary['matrix_p1_record']}")
    print(f"Track 2 Matrix as P2: {summary['matrix_p2_record']}")
    print(f"Ticks - Avg: {avg_ticks:.1f} | Median: {med_ticks} | Min: {summary['min_ticks']} | Max: {summary['max_ticks']}")
    print(f"Action Failures - Track 2 Matrix: {total_matrix_af} | Champion v5: {total_champ_af}")
    print("="*60 + "\n")

if __name__ == "__main__":
    main()
