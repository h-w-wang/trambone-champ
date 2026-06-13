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
    // 定義節點結構
    struct Waypoint {
        float beat;
        float y;
    };

    Note(const std::vector<Waypoint>& waypoints);

    void Update(float currentBeat, float deltaBeat, bool isBlowing, float currentY);
    const std::vector<std::shared_ptr<Util::GameObject>>& GetGameObjects() const { return m_GameObjects; }

    bool IsOut(float currentBeat) const;
    std::string GetScoreResult() const;

    bool IsActivated() const { return m_IsActivated; }
    void Activate() { m_IsActivated = true; }
    float GetTargetTime() const;
    float GetDuration() const;

private:
    std::vector<Waypoint> m_Waypoints;       // 原始設計的轉折邏輯點
    std::vector<Waypoint> m_RenderPoints;     // 經過平滑曲線插值後的細緻渲染點
    std::vector<std::shared_ptr<Util::GameObject>> m_GameObjects; // [0]為頭Dot, [1]為尾Dot, 其餘為細分線段

    float m_HitBeatAmount = 0.0f;
    bool m_IsActivated = false;
    float m_MaxAccuracy = 0.0f;
};

#endif