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
#include "Note.hpp" // 引入音符類別
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

    // --- 音效與視覺 ---
    std::vector<std::shared_ptr<Util::SFX>> m_TromboneNotes;
    std::shared_ptr<Util::GameObject> m_Cursor;

    // --- UI 判定線與音符陣列 ---
    std::shared_ptr<Util::GameObject> m_JudgmentLine;
    std::vector<std::shared_ptr<Note>> m_Notes;

    bool m_WasBlowing = false;
    int m_CurrentNoteIndex = -1;

    // --- 時間 ---
    Uint32 m_StartTime = 0;
    Uint32 m_CurrentMusicTime = 0;
    Uint32 m_RestartHoldStartTime = 0;
    bool m_IsR_Pressed = false;

    void RestartLevel();
};

#endif