#ifndef NOTE_HPP
#define NOTE_HPP

#include "pch.hpp"
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>
#include <vector>
#include <string>

class Note {
public:
    Note(float startYPos, float endYPos, float targetTime, float duration);

    // 🚀 修改：傳入 deltaBeat, isBlowing, currentPitch 來做判定
    void Update(float currentBeat, float deltaBeat, bool isBlowing, int currentPitch);

    const std::vector<std::shared_ptr<Util::GameObject>>& GetGameObjects() const { return m_GameObjects; }

    // 🚀 修改：改用節拍來判斷是否完全越過判定線
    bool IsOut(float currentBeat) const;

    // 🚀 新增：取得結算成績 (Perfect, Good, Miss)
    std::string GetScoreResult() const;

private:
    float m_TargetTime;
    float m_startYPos;
    float m_endYPos;
    float m_Duration;

    std::vector<std::shared_ptr<Util::GameObject>> m_GameObjects;

    // 🚀 判定相關變數
    float m_HitBeatAmount = 0.0f; // 成功按壓的節拍長度
};

#endif