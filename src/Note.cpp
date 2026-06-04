#include "Note.hpp"
#include <cmath>
#include <algorithm>

static std::weak_ptr<Util::Image> s_NoteDotImage;
static std::weak_ptr<Util::Image> s_NoteLineImage;

Note::Note(float startYPos, float endYPos, float targetTime, float duration)
    : m_TargetTime(targetTime), m_startYPos(startYPos), m_endYPos(endYPos), m_Duration(duration) {

    auto dotImg = s_NoteDotImage.lock();
    if (!dotImg) {
        dotImg = std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png");
        s_NoteDotImage = dotImg;
    }
    auto lineImg = s_NoteLineImage.lock();
    if (!lineImg) {
        lineImg = std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light-rotated.png");
        s_NoteLineImage = lineImg;
    }

    auto startDot = std::make_shared<Util::GameObject>();
    startDot->SetDrawable(dotImg); startDot->SetZIndex(5.1f);
    m_GameObjects.push_back(startDot);

    auto endDot = std::make_shared<Util::GameObject>();
    endDot->SetDrawable(dotImg); endDot->SetZIndex(5.1f);
    m_GameObjects.push_back(endDot);

    auto line = std::make_shared<Util::GameObject>();
    line->SetDrawable(lineImg); line->SetZIndex(5.0f);
    m_GameObjects.push_back(line);
}

void Note::Update(float currentBeat, float deltaBeat, bool isBlowing, int currentPitch) {
    float speed = 150.0f;
    float headX = -350.0f + (m_TargetTime - currentBeat) * speed;
    float endX = headX + (m_Duration * speed);

    m_GameObjects[0]->m_Transform.translation = {headX, m_startYPos};
    m_GameObjects[1]->m_Transform.translation = {endX, m_endYPos};
    m_GameObjects[0]->m_Transform.scale = {0.5f, 0.5f};
    m_GameObjects[1]->m_Transform.scale = {0.5f, 0.5f};

    float deltaX = endX - headX;
    float deltaY = m_endYPos - m_startYPos;
    float hypotenuse = std::sqrt(deltaX * deltaX + deltaY * deltaY);
    float angle = std::atan2(deltaY, deltaX);

    m_GameObjects[2]->m_Transform.translation = {(headX + endX) / 2.0f, (m_startYPos + m_endYPos) / 2.0f};
    m_GameObjects[2]->m_Transform.rotation = angle;

    float baseWidth = 256.0f;
    float scaleX = std::max(0.01f, hypotenuse / baseWidth);
    m_GameObjects[2]->m_Transform.scale = {scaleX, 0.5f};

    if (currentBeat >= m_TargetTime && currentBeat <= m_TargetTime + m_Duration) {
        float progress = std::clamp((currentBeat - m_TargetTime) / m_Duration, 0.0f, 1.0f);
        float currentExpectedY = m_startYPos + (m_endYPos - m_startYPos) * progress;
        int expectedPitch = std::clamp(static_cast<int>(std::round((currentExpectedY + 300.0f) / 25.0f)), 0, 24);

        // 🚀 核心修改：必須是「已啟動(m_IsActivated)」的狀態才會累積加分！
        if (isBlowing && m_IsActivated && currentPitch == expectedPitch) {
            m_HitBeatAmount += deltaBeat;
        }
    }
}

bool Note::IsOut(float currentBeat) const {
    return currentBeat > (m_TargetTime + m_Duration);
}

std::string Note::GetScoreResult() const {
    if (m_Duration <= 0) return "Miss";
    float hitPercent = m_HitBeatAmount / m_Duration;
    if (hitPercent >= 0.8f) return "Perfect";
    if (hitPercent >= 0.5f) return "Good";
    return "Miss"; // 🚀 如果死按著不放，沒被啟動，hitPercent 會是 0，直接判定 Miss！
}