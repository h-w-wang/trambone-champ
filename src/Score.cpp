#include "ScoreManager.hpp"

void ScoreManager::EvaluateNote(float deltaY, bool isBlowing) {
    // 狀況一：如果根本沒吹，直接斷 Combo 並算入 NASTY
    if (!isBlowing) {
        m_Combo = 0;
        m_NastyCount++;
        m_LatestJudgment = "NASTY!";
        return;
    }

    Judgment result;
    int baseScore = 0;

    // 狀況二：有吹，依據垂直距離判定
    if (deltaY <= 6.0f) {
        result = Judgment::PERFECTO;
        baseScore = 1000;
        m_PerfectCount++;
        m_LatestJudgment = "PERFECTO!";
    } else if (deltaY <= 18.0f) {
        result = Judgment::NICE;
        baseScore = 800;
        m_NiceCount++;
        m_LatestJudgment = "NICE!";
    } else if (deltaY <= 35.0f) {
        result = Judgment::OK;
        baseScore = 500;
        m_OkCount++;
        m_LatestJudgment = "OKAY";
    } else if (deltaY <= 60.0f) {
        result = Judgment::MEH;
        baseScore = 200;
        m_MehCount++;
        m_LatestJudgment = "MEH";
    } else {
        result = Judgment::NASTY;
        baseScore = 0;
        m_NastyCount++;
        m_LatestJudgment = "NASTY!";
    }

    // 只有非 NASTY 的判定才能增加 Combo 與分數
    if (result != Judgment::NASTY) {
        m_Combo++;
        m_Score += baseScore * GetMultiplier();
    } else {
        m_Combo = 0;
    }
}

std::string ScoreManager::CalculateRank(int maxPossibleScore) const {
    if (maxPossibleScore <= 0) return "F";
    float ratio = (float)m_Score / maxPossibleScore;

    if (ratio >= 0.95f) return "S";
    if (ratio >= 0.85f) return "A";
    if (ratio >= 0.70f) return "B";
    if (ratio >= 0.55f) return "C";
    if (ratio >= 0.40f) return "D";
    return "F";
}

int ScoreManager::CalculateTootCoins() const {
    return (m_Score / 20000) + m_PerfectCount;
}

float ScoreManager::GetProgressBarPercentage(int maxPossibleScore) const {
    if (maxPossibleScore <= 0) return 0.0f;
    return std::min(1.0f, (float)m_Score / maxPossibleScore);
}

void ScoreManager::Reset() {
    m_Score = 0; m_Combo = 0; m_LatestJudgment = "";
    m_PerfectCount = 0; m_NiceCount = 0; m_OkCount = 0;
    m_MehCount = 0; m_NastyCount = 0;
}