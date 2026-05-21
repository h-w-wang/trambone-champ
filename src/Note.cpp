#include "Note.hpp"
#include <cmath>
#include <algorithm>

// 🚀 關鍵修復：改用 weak_ptr，防止遊戲關閉時 OpenGL 引擎崩潰
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

    // 1. 音符頭
    auto startDot = std::make_shared<Util::GameObject>();
    startDot->SetDrawable(dotImg);
    startDot->SetZIndex(5.1f);
    m_GameObjects.push_back(startDot);

    // 2. 音符尾
    auto endDot = std::make_shared<Util::GameObject>();
    endDot->SetDrawable(dotImg);
    endDot->SetZIndex(5.1f);
    m_GameObjects.push_back(endDot);

    // 3. 連接線段
    auto line = std::make_shared<Util::GameObject>();
    line->SetDrawable(lineImg);
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