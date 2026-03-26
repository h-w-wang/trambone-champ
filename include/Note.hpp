#ifndef NOTE_HPP
#define NOTE_HPP

#include "pch.hpp"
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>

class Note {
public:
    Note(float yPos, float targetTime);
    void Update(Uint32 currentMusicTime);

    // ++ 補上這行最關鍵的程式碼：讓 App 可以拿到 GameObject 來畫在畫面上
    std::shared_ptr<Util::GameObject> GetGameObject() const { return m_GameObject; }

    bool IsOut() const;

private:
    float m_TargetTime;
    float m_YPos;
    std::shared_ptr<Util::GameObject> m_GameObject;
};

#endif