#ifndef NOTE_HPP
#define NOTE_HPP

#include "pch.hpp"
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>
#include <vector>

class Note {
public:
    Note(float startYPos, float endYPos, float targetTime, float duration);
    void Update(float currentBeat); // 改吃節拍參數

    const std::vector<std::shared_ptr<Util::GameObject>>& GetGameObjects() const { return m_GameObjects; }

    bool IsOut() const;

private:
    float m_TargetTime;
    float m_startYPos;
    float m_endYPos;
    float m_Duration;

    std::vector<std::shared_ptr<Util::GameObject>> m_GameObjects;
};

#endif