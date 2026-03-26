#ifndef NOTE_HPP
#define NOTE_HPP

#include "pch.hpp"
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>

class Note {
public:
    // 建立音符時，告訴它要在哪個高度 (yPos)，以及在音樂的哪一毫秒 (targetTime) 抵達判定線
    Note(float yPos, float targetTime);

    // 每一幀根據目前的音樂時間來更新座標
    void Update(Uint32 currentMusicTime);
    void Draw();

    // 檢查音符是不是已經跑到畫面最左邊外側了 (用來清除不需要的音符)
    bool IsOut() const;

private:
    std::shared_ptr<Util::GameObject> m_GameObject;
    float m_TargetTime;
    float m_YPos;
};

#endif