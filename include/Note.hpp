#ifndef NOTE_HPP
#define NOTE_HPP

#include "pch.hpp"
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>
#include <vector>

class Note {
public:
    // 新增參數：endYPos (音符結束時的 Y 座標，用來做滑音)
    Note(float startYPos, float endYPos, float targetTime, float duration);
    void Update(Uint32 currentMusicTime);

    //Container Class 需要暴露它管理的所有物件以進行繪製
    const std::vector<std::shared_ptr<Util::GameObject>>& GetGameObjects() const { return m_GameObjects; }

    bool IsOut() const;

private:
    float m_TargetTime;
    float m_startYPos;
    float m_endYPos; // 音符結束時的音高
    float m_Duration; // 記錄這個音符有多長

    // 管理這顆長音符的所有視覺元件 (頭, 尾, 線)
    std::vector<std::shared_ptr<Util::GameObject>> m_GameObjects;
};

#endif