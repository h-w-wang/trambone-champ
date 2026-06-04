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

    void Update(float currentBeat, float deltaBeat, bool isBlowing, int currentPitch);
    const std::vector<std::shared_ptr<Util::GameObject>>& GetGameObjects() const { return m_GameObjects; }

    bool IsOut(float currentBeat) const;
    std::string GetScoreResult() const;

    // 🚀 新增：音符的獨立啟動機制
    bool IsActivated() const { return m_IsActivated; }
    void Activate() { m_IsActivated = true; }
    float GetTargetTime() const { return m_TargetTime; }
    float GetDuration() const { return m_Duration; }

private:
    float m_TargetTime;
    float m_startYPos;
    float m_endYPos;
    float m_Duration;

    std::vector<std::shared_ptr<Util::GameObject>> m_GameObjects;

    float m_HitBeatAmount = 0.0f;
    bool m_IsActivated = false; // 🚀 紀錄這顆音符是否已經被玩家「重新吹氣」啟動
};

#endif