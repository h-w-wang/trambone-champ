#include "Note.hpp"
#include <cmath>
#include <algorithm>

static std::shared_ptr<Util::Image> s_NoteDotImage = nullptr;
static std::shared_ptr<Util::Image> s_NoteLineImage = nullptr;

Note::Note(float startYPos, float endYPos, float targetTime, float duration)
    : m_TargetTime(targetTime), m_startYPos(startYPos), m_endYPos(endYPos), m_Duration(duration) {

    if (!s_NoteDotImage) {
        s_NoteDotImage = std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png");
        s_NoteLineImage = std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light-rotated.png");
    }

    // 1. 音符頭
    auto startDot = std::make_shared<Util::GameObject>();
    startDot->SetDrawable(s_NoteDotImage); // 大家都共用同一張圖！
    startDot->SetZIndex(5.1f);
    m_GameObjects.push_back(startDot);

    // 2. 音符尾
    auto endDot = std::make_shared<Util::GameObject>();
    endDot->SetDrawable(s_NoteDotImage);
    endDot->SetZIndex(5.1f);
    m_GameObjects.push_back(endDot);

    // 3. 連接線段
    auto line = std::make_shared<Util::GameObject>();
    line->SetDrawable(s_NoteLineImage);
    line->SetZIndex(5.0f);
    m_GameObjects.push_back(line);
}

void Note::Update(float currentBeat) {
    float speed = 150.0f;
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
}

bool Note::IsOut() const {
    return m_GameObjects[1]->m_Transform.translation.x < -1000.0f;
}