#include "App.hpp"
#include <iostream>

App::State App::GetCurrentState() const {
    return m_CurrentState;
}

void App::Start() {
    m_Keyboard = std::make_shared<Keyboard>();

    m_BGM = std::make_shared<Util::BGM>(RESOURCE_DIR "/test_music.mp3");
    m_BGM->SetVolume(64);
    m_BGM->Play(-1);

    m_SoundEffect = std::make_shared<Util::SFX>(RESOURCE_DIR "/c8.wav");
    m_SoundEffect->SetVolume(100);

    // ==========================================
    // ++ 修改：建立 GameObject 並裝備上圖片
    // ==========================================
    m_Cursor = std::make_shared<Util::GameObject>();
    m_Cursor->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note.png"));
    m_Cursor->SetZIndex(10.0f); // 設定層級在最上層

    m_StartTime = SDL_GetTicks();
    m_CurrentState = State::UPDATE;
}

void App::RestartLevel() {
    std::cout << "重新開始關卡！\n";
    m_BGM->Play(-1);
    m_StartTime = SDL_GetTicks();
}

void App::Update() {
    m_CurrentMusicTime = SDL_GetTicks() - m_StartTime;
    m_Keyboard->Update();

    auto cursorPos = Util::Input::GetCursorPosition();

    // ==========================================
    // 視覺：讓 GameObject 跟著滑鼠移動並畫出來
    // ==========================================
    m_Cursor->m_Transform.translation = cursorPos;
    m_Cursor->m_Transform.scale = {0.5f, 0.5f};
    m_Cursor->Draw();


    // 聽覺：偵測吹奏並發出聲音
    bool currentBlowing = m_Keyboard->IsBlowing();

    if (currentBlowing && !m_WasBlowing) {
        m_SoundEffect->Play();
    }
    m_WasBlowing = currentBlowing;

    // 遊戲控制邏輯
    if (Util::Input::IsKeyPressed(Util::Keycode::R)) {
        m_RestartHoldStartTime = SDL_GetTicks();
        m_IsR_Pressed = true;
    }
    else if (Util::Input::IsKeyDown(Util::Keycode::R)) {
        if (m_IsR_Pressed && (SDL_GetTicks() - m_RestartHoldStartTime >= 800)) {
            RestartLevel();
            m_IsR_Pressed = false;
        }
    }
    else {
        m_IsR_Pressed = false;
    }

    if (Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB)) {
        if (cursorPos.x < -400 && cursorPos.y > 250) {
            std::cout << "點擊了退出按鈕，結束遊戲。\n";
            m_CurrentState = State::END;
        }
    }
}

void App::End() {
}