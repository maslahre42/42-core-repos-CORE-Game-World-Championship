# 42 CORE Championship: Official Tournament Run & Results

## 1. Executive Summary & Achievements

In the official **42 CORE Game World Championship**, team **`No_Plan_Just_Prayers`** (42 Heilbronn) achieved an outstanding performance:

- 🏅 **4th Place International Ranking** in the Swiss Group Phase out of all global 42 campuses.
- ⚔️ **5 Wins, 1 Loss (5.0 Points)** in the Swiss Group Phase.
- 🎯 **Advanced to the Championship Playoff Quarterfinals**.
- 🛡️ **Strictly 0 Action Failures** across official matches.

---

## 2. Swiss Group Phase Official Standings

In the group phase, all global teams competed using the official Swiss tournament system with Buchholz tiebreak scoring.

![Group Phase Rankings](assets/Screenshot%20from%202026-11-07%2021-33-27.png)

### Group Phase Leaderboard Top 10

| Rank | Team Name | Campus | Score | Buchholz | Byes | Match History |
| :---: | :--- | :---: | :---: | :---: | :---: | :---: |
| 1 | **CoreMasters** | 42 Beirut | **6.0** | 15.0 | 0 | `W W W W W W` |
| 2 | **Seegler.ai** | — | **5.0** | 25.0 | 0 | `W W W W W L` |
| 3 | **WorkedOnMyMachine** | — | **5.0** | 22.0 | 0 | `W W W W L W` |
| **4** | **No_Plan_Just_Prayers** | **42 Heilbronn** | **5.0** | **17.0** | **0** | **`W W L W W W`** 🏆 |
| 5 | **128126** | 42 Heilbronn | **5.0** | 17.0 | 0 | `W W L W W W` |
| 6 | **GerhardReinlunzen** | 42 Heilbronn | **4.0** | 23.0 | 0 | `L W W W W L` |
| 7 | **thel** | 42 Heilbronn | **4.0** | 21.0 | 0 | `L W W W W L` |
| 8 | **L_oves_Coffee** | 42 Heilbronn | **4.0** | 19.0 | 0 | `W W L L W W` |
| 9 | **bot1** | — | **4.0** | 17.0 | 0 | `W L W W L W` |
| 10 | **Banana** | 42 Porto | **4.0** | 16.0 | 0 | `L L W W W W` |

---

## 3. Championship Playoff Bracket

![Playoff Bracket Graph](assets/Screenshot%20from%202026-11-07%2021-30-43.png)

### Playoff Match Results:
1. **Round of 16 (Match `e0e6`):**
   - **`No_Plan_Just_Prayers`** vs **`Banana`** (42 Porto)
   - **Outcome:** **`No_Plan_Just_Prayers` WON!** 🏆
   - **Key Factor:** Early warrior rush interception neutralized Banana's aggressive opener, transitioning into an unstoppable 307g Brawler deathball.
2. **Quarterfinals (Match `ae4a`):**
   - **`No_Plan_Just_Prayers`** vs **`L_oves_Coffee`** (42 Heilbronn)
   - **Outcome:** Highly contested attrition battle; `L_oves_Coffee` advanced to the Semifinals.

---

## 4. Pre-Tournament Benchmark Archive

Before the final code freeze, both developmental tracks underwent rigorous 20-match side-swapped evaluation in isolated Docker containers:

### 4.1 Candidate v6 vs. Champion v5 (20 Matches)
- **Data File:** [`scripts/benchmark_v6_vs_v5_results.json`](file:///goinfre/malshare/malshare/Core_game_world/CORE-Game-World-Championship-No_Plan_Just_Prayers/scripts/benchmark_v6_vs_v5_results.json)
- **Candidate v6:** **15 Wins (75.0%)** (P1: 7/10, P2: 8/10)
- **Champion v5:** **5 Wins (25.0%)** (P1: 2/10, P2: 3/10)
- **Action Failures:** **0 AF across all 20 matches on both sides.**
- **Decisive Kills:** Candidate v6 destroyed Champion v5's Core in **13 out of 18 decisive matches** (median time: 1,112 ticks).
- **Result:** **Promoted to Champion v6 in `my-core-bot/`**.

### 4.2 Track 2 Retrained Q-Matrix vs. Champion v5 (20 Matches)
- **Data File:** [`scripts/benchmark_20_results.json`](file:///goinfre/malshare/malshare/Core_game_world/CORE-Game-World-Championship-No_Plan_Just_Prayers/scripts/benchmark_20_results.json)
- **Champion v5:** **19 Wins (95.0%)**
- **Track 2 Matrix:** **1 Win (5.0%)**
- **Action Failures:** **0 AF across all 20 matches on both sides.**
- **Finding:** The Q-matrix converged on squishy 167g Skirmishers from its multi-opponent training pool, falling victim to Lanchester's Square Law against pure 307g Heavy Brawlers.
- **Result:** Preserved as a research prototype in [`bots/track2_matrix/`](file:///goinfre/malshare/malshare/Core_game_world/CORE-Game-World-Championship-No_Plan_Just_Prayers/bots/track2_matrix/).
