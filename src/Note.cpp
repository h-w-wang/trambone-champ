#include "Note.hpp"

Note::Note(float startYPos, float endYPos, float targetTime, float duration)
    : m_TargetTime(targetTime), m_startYPos(startYPos), m_endYPos(endYPos), m_Duration(duration) {

    // 1. 音符頭 (Start Dot)
    auto startDot = std::make_shared<Util::GameObject>();
    startDot->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
    startDot->SetZIndex(5.1f); // 高度 5.1，確保疊在延長線上方
    m_GameObjects.push_back(startDot);

    // 2. 音符尾 (End Dot)
    auto endDot = std::make_shared<Util::GameObject>();
    endDot->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
    endDot->SetZIndex(5.1f);
    m_GameObjects.push_back(endDot);

    // 3. 延長線 (Middle Line)
    auto line = std::make_shared<Util::GameObject>();
    line->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));
    line->SetZIndex(5.0f); // 高度 5.0，被頭尾點點壓在下面
    m_GameObjects.push_back(line);
}

void Note::Update(Uint32 currentMusicTime) {
    float speed = 0.4f; // 橫向滾動速度

    // 計算音符「頭部」的 X 座標
    float headX = -300.0f + (m_TargetTime - static_cast<float>(currentMusicTime)) * speed;

    // 計算音符「尾部」的 X 座標
    float endX = headX + (m_Duration * speed);

    // 更新頭部點點位置
    m_GameObjects[0]->m_Transform.translation = {headX, m_startYPos};
    m_GameObjects[0]->m_Transform.scale = {0.3f, 0.3f};

    // 更新尾部點點位置
    m_GameObjects[1]->m_Transform.translation = {endX, m_endYPos};
    m_GameObjects[1]->m_Transform.scale = {0.3f, 0.3f};

    // 更新中間延長線位置與拉伸比例
    float deltaX = endX - headX;
    float centerX = headX + (deltaX / 2.0f);
    m_GameObjects[2]->m_Transform.translation = {centerX, m_startYPos};

    // 假設 note-line.png 的原始寬度是 100 像素，我們依此來計算拉伸比例
    float scaleX = deltaX / 100.0f;
    if (scaleX < 0.01f) scaleX = 0.01f; // 防呆，避免縮太小
    m_GameObjects[2]->m_Transform.scale = {scaleX, 0.3f};
}

// 修正：加上 Note:: 讓它屬於 Note 類別
bool Note::IsOut() const {
    // 用「尾巴」(m_GameObjects[1]) 的位置來判斷是否整條音符都超過左邊界了
    return m_GameObjects[1]->m_Transform.translation.x < -800.0f;
}