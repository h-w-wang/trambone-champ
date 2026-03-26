#include "Note.hpp"

Note::Note(float yPos, float targetTime)
    : m_TargetTime(targetTime), m_YPos(yPos) {

    m_GameObject = std::make_shared<Util::GameObject>();
    // 關鍵：這裡改成讀取你上傳的紫藍色長條音符！
    m_GameObject->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/image_416763.png"));
}

void Note::Update(Uint32 currentMusicTime) {
    float speed = 0.4f;
    float xPos = -300.0f + (m_TargetTime - static_cast<float>(currentMusicTime)) * speed;
    m_GameObject->m_Transform.translation = {xPos, m_YPos};

    // 如果覺得這張紫藍色音符在畫面上太大或太小，可以改這兩個數字 (目前預設 0.5 倍)
    m_GameObject->m_Transform.scale = {0.5f, 0.5f};
}

bool Note::IsOut() const {
    return m_GameObject->m_Transform.translation.x < -600.0f;
}