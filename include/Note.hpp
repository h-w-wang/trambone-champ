#ifndef NOTE_HPP
#define NOTE_HPP

#include "pch.hpp"
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>
#include <vector>
#include <string>

class Note {
public:
    Note(float startYPos, float endYPos, float targetTime, float duration);

    // 🚀 新增 deltaBeat, isBlowing, currentPitch 等參數來處理判定
    void Update(float currentBeat, float deltaBeat, bool isBlowing, int currentPitch);

    const std::vector<std::shared_ptr<Util::GameObject>>& GetGameObjects() const { return m_GameObjects; }

    bool IsOut(float currentBeat) const;

    // 🚀 取得結算成績 (Perfect, Good, Miss)
    std::string GetScoreResult() const;

private:
    float m_TargetTime;
    float m_startYPos;
    float m_endYPos;
    float m_Duration;

    std::vector<std::shared_ptr<Util::GameObject>> m_GameObjects;

    // 🚀 紀錄成功按壓的時間長度
    float m_HitBeatAmount = 0.0f;
};

#endif