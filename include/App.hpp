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
#include <SDL.h>
#include <memory>

class App {
public:
    enum class State {
        START,
        UPDATE,
        END,
    };

    State GetCurrentState() const;
    void Start();
    void Update();
    void End();

private:
    State m_CurrentState = State::START;
    std::shared_ptr<Util::BGM> m_BGM;
    std::shared_ptr<Keyboard> m_Keyboard;

    // --- 音效與視覺 ---
    std::shared_ptr<Util::SFX> m_SoundEffect;
    std::shared_ptr<Util::GameObject> m_Cursor;
    bool m_WasBlowing = false;

    // --- 音樂計時器 ---
    Uint32 m_StartTime = 0;
    Uint32 m_CurrentMusicTime = 0;

    // --- 長按 R 重新開始的相關變數 ---
    Uint32 m_RestartHoldStartTime = 0;
    bool m_IsR_Pressed = false;

    void RestartLevel();
};

#endif