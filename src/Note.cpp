#include "../include/Note.hpp"

Note::Note(float yPos, float targetTime)
    : m_YPos(yPos), m_TargetTime(targetTime) {

    m_GameObject = std::make_shared<Util::GameObject>();
    m_GameObject->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note.png"));
    m_GameObject->SetZIndex(5.0f); // 層級設為 5，確保它會被層級 10 的游標蓋在下面
}

void Note::Update(Uint32 currentMusicTime) {
    // 假設我們的打擊判定線在 X = -300 的位置
    // 音符移動速度：每毫秒移動 0.4 個像素 (數值越大跑越快)
    float speed = 0.4f;

    // 計算目前的 X 座標：判定線位置 + (目標時間 - 當前時間) * 速度
    float xPos = -300.0f + (m_TargetTime - static_cast<float>(currentMusicTime)) * speed;

    m_GameObject->m_Transform.translation = {xPos, m_YPos};
    m_GameObject->m_Transform.scale = {0.3f, 0.3f}; // 讓音符稍微小一點
}

void Note::Draw() {
    m_GameObject->Draw();
}

bool Note::IsOut() const {
    // 如果 X 座標小於 -600，代表已經飛出畫面左側了
    return m_GameObject->m_Transform.translation.x < -600.0f;
}