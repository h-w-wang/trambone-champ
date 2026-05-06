#include "Note.hpp"
#include <cmath>
#include <algorithm>

Note::Note(float startYPos, float endYPos, float targetTime, float duration)
    : m_TargetTime(targetTime), m_startYPos(startYPos), m_endYPos(endYPos), m_Duration(duration) {

    auto startDot = std::make_shared<Util::GameObject>();
    startDot->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
    startDot->SetZIndex(5.1f);
    m_GameObjects.push_back(startDot);

    auto endDot = std::make_shared<Util::GameObject>();
    endDot->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
    endDot->SetZIndex(5.1f);
    m_GameObjects.push_back(endDot);

    auto line = std::make_shared<Util::GameObject>();
    line->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light-rotated.png"));
    line->SetZIndex(5.0f);
    m_GameObjects.push_back(line);
}

void Note::Update(float currentBeat, float deltaBeat, bool isBlowing, int currentPitch) {
    float speed = 150.0f;

    // 1. 位置更新與數學旋轉 (保持不變)
    float headX = -300.0f + (m_TargetTime - currentBeat) * speed;
    float endX = headX + (m_Duration * speed);

    m_GameObjects[0]->m_Transform.translation = {headX, m_startYPos};
    m_GameObjects[1]->m_Transform.translation = {endX, m_endYPos};
    m_GameObjects[0]->m_Transform.scale = {0.3f, 0.3f};
    m_GameObjects[1]->m_Transform.scale = {0.3f, 0.3f};

    float deltaX = endX - headX;
    float deltaY = m_endYPos - m_startYPos;
    float hypotenuse = std::sqrt(deltaX * deltaX + deltaY * deltaY);
    float angle = std::atan2(deltaY, deltaX);

    m_GameObjects[2]->m_Transform.translation = {(headX + endX) / 2.0f, (m_startYPos + m_endYPos) / 2.0f};
    m_GameObjects[2]->m_Transform.rotation = angle;

    float baseWidth = 256.0f;
    float scaleX = std::max(0.01f, hypotenuse / baseWidth);
    m_GameObjects[2]->m_Transform.scale = {scaleX, 0.2f};

    // 🚀 2. 動態點擊判定邏輯
    // 如果當前音樂時間正在這個音符的生存區間內
    if (currentBeat >= m_TargetTime && currentBeat <= m_TargetTime + m_Duration) {
        // 計算這個瞬間，滑條應該落在哪個 Y 軸 (處理斜線滑音的判定)
        float progress = (currentBeat - m_TargetTime) / m_Duration;
        progress = std::clamp(progress, 0.0f, 1.0f);

        float currentExpectedY = m_startYPos + (m_endYPos - m_startYPos) * progress;
        int expectedPitch = std::clamp(static_cast<int>(std::round((currentExpectedY + 480.0f) / 40.0f)), 0, 24);

        // 如果玩家有吹，且當前游標的音高 == 音符現在應該在的音高，就累積成功按壓的時間
        if (isBlowing && currentPitch == expectedPitch) {
            m_HitBeatAmount += deltaBeat;
        }
    }
}

bool Note::IsOut(float currentBeat) const {
    // 🚀 只要當前音樂節拍 > (音符目標時間 + 音符長度)，就代表整條線完全過了判定線 (-300)
    return currentBeat > (m_TargetTime + m_Duration);
}

std::string Note::GetScoreResult() const {
    if (m_Duration <= 0) return "Miss"; // 防呆

    // 計算成功覆蓋的比例
    float hitPercent = m_HitBeatAmount / m_Duration;

    // 百分比判定：你可以根據需求自己調整這個標準
    if (hitPercent >= 0.8f) return "Perfect";
    if (hitPercent >= 0.5f) return "Good";
    return "Miss";
}