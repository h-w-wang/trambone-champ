#include "Note.hpp"

Note::Note(float startYPos, float endYPos, float targetTime, float duration)
    : m_TargetTime(targetTime), m_startYPos(startYPos), m_endYPos(endYPos), m_Duration(duration) {

    // 1. 音符頭 (Start Dot)
    auto startDot = std::make_shared<Util::GameObject>();
    startDot->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
    startDot->SetZIndex(5.1f);
    m_GameObjects.push_back(startDot);

    // 2. 音符尾 (End Dot)
    auto endDot = std::make_shared<Util::GameObject>();
    endDot->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
    endDot->SetZIndex(5.1f);
    m_GameObjects.push_back(endDot);

    // 3. 延長線 (Middle Line)
    auto line = std::make_shared<Util::GameObject>();
    line->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light-rotated.png"));
    line->SetZIndex(5.0f);
    m_GameObjects.push_back(line);
}

void Note::Update(float currentBeat) {
    float speed = 150.0f; // 橫向滾動速度 (配合節拍)

    // 🚀 關鍵修復：目標是精準撞擊 -300.0f 的打擊線！
    float headX = -300.0f + (m_TargetTime - currentBeat) * speed;
    float endX = headX + (m_Duration * speed);

    m_GameObjects[0]->m_Transform.translation = {headX, m_startYPos};
    m_GameObjects[0]->m_Transform.scale = {0.3f, 0.3f};

    m_GameObjects[1]->m_Transform.translation = {endX, m_endYPos};
    m_GameObjects[1]->m_Transform.scale = {0.3f, 0.3f};

    float deltaX = endX - headX;
    float centerX = headX + (deltaX / 2.0f);
    m_GameObjects[2]->m_Transform.translation = {centerX, m_startYPos};

    // 這裡 256.0f 請配合你 warmup-light-rotated.png 圖片的實際寬度微調
    float scaleX = deltaX / 256.0f;
    if (scaleX < 0.01f) scaleX = 0.01f;
    m_GameObjects[2]->m_Transform.scale = {scaleX, 0.3f};
}

bool Note::IsOut() const {
    return m_GameObjects[1]->m_Transform.translation.x < -800.0f;
}