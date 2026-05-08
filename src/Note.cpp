#include "Note.hpp"
#include <cmath>
#include <algorithm> // 給 std::max 用的

Note::Note(float startYPos, float endYPos, float targetTime, float duration)
    : m_TargetTime(targetTime), m_startYPos(startYPos), m_endYPos(endYPos), m_Duration(duration) {

    // 1. 音符頭
    auto startDot = std::make_shared<Util::GameObject>();
    startDot->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
    startDot->SetZIndex(5.1f);
    m_GameObjects.push_back(startDot);

    // 2. 音符尾
    auto endDot = std::make_shared<Util::GameObject>();
    endDot->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
    endDot->SetZIndex(5.1f);
    m_GameObjects.push_back(endDot);

    // 3. 連接線段
    auto line = std::make_shared<Util::GameObject>();
    line->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light-rotated.png"));
    line->SetZIndex(5.0f);
    m_GameObjects.push_back(line);
}

void Note::Update(float currentBeat) {
    float speed = 150.0f; // 橫向移動速度

    float headX = -300.0f + (m_TargetTime - currentBeat) * speed;
    float endX = headX + (m_Duration * speed);

    //更新頭尾點的位置
    m_GameObjects[0]->m_Transform.translation = {headX, m_startYPos};
    m_GameObjects[1]->m_Transform.translation = {endX, m_endYPos};
    m_GameObjects[0]->m_Transform.scale = {0.3f, 0.3f};
    m_GameObjects[1]->m_Transform.scale = {0.3f, 0.3f};

    //計算滑音線段
    float deltaX = endX - headX;
    float deltaY = m_endYPos - m_startYPos;

    // 計算斜邊長度
    float hypotenuse = std::sqrt(deltaX * deltaX + deltaY * deltaY);

    // 計算旋轉角度
    float angle = std::atan2(deltaY, deltaX);

    // 將線段放在兩個點的正中間
    m_GameObjects[2]->m_Transform.translation = {(headX + endX) / 2.0f, (m_startYPos + m_endYPos) / 2.0f};

    // 設置旋轉弧度，讓它斜接兩點
    m_GameObjects[2]->m_Transform.rotation = angle;

    float baseWidth = 256.0f;
    float scaleX = std::max(0.01f, hypotenuse / baseWidth);
    m_GameObjects[2]->m_Transform.scale = {scaleX, 0.2f};
}

bool Note::IsOut() const {
    return m_GameObjects[1]->m_Transform.translation.x < -1000.0f;
}