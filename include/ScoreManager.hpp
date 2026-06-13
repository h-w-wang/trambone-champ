#ifndef SCORE_MANAGER_HPP
#define SCORE_MANAGER_HPP

#include <string>
#include <algorithm>

// 判定完全對照遊戲結算：只保留這五種
enum class Judgment {
    PERFECTO, // 完美
    NICE,     // 挺好
    OK,       // 不錯
    MEH,      // 一般
    NASTY     // 差勁 (沒吹或音高偏離太多都算這個)
};

class ScoreManager {
public:
    ScoreManager() = default;

    // 傳入誤差值與吹奏狀態，沒吹或誤差太大都直接計入 NASTY
    void EvaluateNote(float deltaY, bool isBlowing);

    // 實時 HUD 數據
    int GetScore() const { return m_Score; }
    int GetCombo() const { return m_Combo; }
    int GetMultiplier() const { return std::min(10, 1 + (m_Combo / 10)); }
    std::string GetLatestJudgmentText() const { return m_LatestJudgment; }

    // 結算畫面五大指標
    int GetPerfectCount() const { return m_PerfectCount; }
    int GetNiceCount() const { return m_NiceCount; }
    int GetOkCount() const { return m_OkCount; }
    int GetMehCount() const { return m_MehCount; }
    int GetNastyCount() const { return m_NastyCount; }

    // 結算計算
    std::string CalculateRank(int maxPossibleScore) const;
    int CalculateTootCoins() const;
    float GetProgressBarPercentage(int maxPossibleScore) const;

    void Reset();

private:
    int m_Score = 0;
    int m_Combo = 0;
    std::string m_LatestJudgment = "";

    // 五種判定計數器
    int m_PerfectCount = 0;
    int m_NiceCount = 0;
    int m_OkCount = 0;
    int m_MehCount = 0;
    int m_NastyCount = 0;
};

#endif