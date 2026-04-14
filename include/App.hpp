#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp"
#include "Util/BGM.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/SFX.hpp"
#include "Util/Image.hpp"
#include "Util/GameObject.hpp"
#include "Keyboard.hpp"
#include "Note.hpp"
#include <SDL.h>
#include <memory>
#include <vector>

class App {
public:
    enum class State { START, UPDATE, END };
    State GetCurrentState() const;
    void Start();
    void Update();
    void End();

private:
    State m_CurrentState = State::START;
    std::shared_ptr<Util::BGM> m_BGM;
    std::shared_ptr<Keyboard> m_Keyboard;

    // --- 視覺與音效物件 ---
    std::shared_ptr<Util::GameObject> m_Background;
    std::vector<std::shared_ptr<Util::SFX>> m_TromboneNotes; // 25 個半音效
    std::shared_ptr<Util::GameObject> m_Indicator;           // 左側的判定線
    std::vector<std::shared_ptr<Note>> m_Notes;              // 飛過來的音符

    // --- 玩家控制的 Pattern 相關 --
    std::shared_ptr<Util::GameObject> m_Pattern;
    std::shared_ptr<Util::Image> m_PatternIdleImage;
    std::shared_ptr<Util::Image> m_PatternPlayImage;

    bool m_WasBlowing = false;
    int m_CurrentNoteIndex = -1;

    // --- 時間管理 ---
    Uint32 m_StartTime = 0;
    Uint32 m_CurrentMusicTime = 0;
    Uint32 m_RestartHoldStartTime = 0;
    bool m_IsR_Pressed = false;

    void RestartLevel();

    // --- 錄製譜面與視覺輔助 ---
    Uint32 m_RecordStartTime = 0;
    float m_RecordStartY = 0.0f;
    std::vector<std::shared_ptr<Util::GameObject>> m_GuideLines; // 25條參考線
};

#endif