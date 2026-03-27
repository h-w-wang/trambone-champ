#include "App.hpp"
#include "Util/Transform.hpp"
#include <iostream>

App::State App::GetCurrentState() const {
    return m_CurrentState;
}

void App::Start() {
    m_Keyboard = std::make_shared<Keyboard>();

    m_BGM = std::make_shared<Util::BGM>(RESOURCE_DIR "/test_music.mp3");
    m_BGM->SetVolume(64);
    m_BGM->Play(-1);

    m_Background = std::make_shared<Util::GameObject>();
    m_Background->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/titlescreen_bg.jpg"));
    m_Background->SetZIndex(-10.0f);

    m_Indicator = std::make_shared<Util::GameObject>();
    m_Indicator->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));
    m_Indicator->m_Transform.translation = {-300.0f, 0.0f};
    m_Indicator->m_Transform.scale = {1.0f, 2.0f};
    m_Indicator->SetZIndex(1.0f);

    m_PatternIdleImage = std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png");
    m_PatternPlayImage = std::make_shared<Util::Image>(RESOURCE_DIR "/player-note-dot-ON.png");
    m_Pattern = std::make_shared<Util::GameObject>();
    m_Pattern->SetDrawable(m_PatternIdleImage);
    m_Pattern->SetZIndex(100.0f);

    // 🚀 修正譜面：參數變成 4 個 (開始Y, 結束Y, 目標時間, 持續時間)
    // 目前我們先讓 開始Y = 結束Y，做出水平的長音符
    m_Notes.push_back(std::make_shared<Note>(0.0f, 0.0f, 2000.0f, 500.0f));     // 短音
    m_Notes.push_back(std::make_shared<Note>(150.0f, 150.0f, 3000.0f, 1500.0f));  // 長音
    m_Notes.push_back(std::make_shared<Note>(-150.0f, -150.0f, 5000.0f, 2000.0f)); // 超長音

    SDL_ShowCursor(SDL_DISABLE);
    m_StartTime = SDL_GetTicks();
    m_CurrentState = State::UPDATE;
}

void App::RestartLevel() {
    m_BGM->Play(-1);
    m_StartTime = SDL_GetTicks();
}

void App::Update() {
    m_CurrentMusicTime = SDL_GetTicks() - m_StartTime;
    auto cursorPos = Util::Input::GetCursorPosition();

    for (auto& note : m_Notes) {
        note->Update(m_CurrentMusicTime);
    }

    m_Pattern->m_Transform.translation = {-300.0f, cursorPos.y};
    m_Pattern->m_Transform.scale = {0.3f, 0.3f};

    bool currentBlowing = Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB) ||
                          Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB) ||
                          Util::Input::IsKeyDown(Util::Keycode::SPACE) ||
                          Util::Input::IsKeyPressed(Util::Keycode::SPACE);

    if (currentBlowing) {
        m_Pattern->SetDrawable(m_PatternPlayImage);
    } else {
        m_Pattern->SetDrawable(m_PatternIdleImage);
    }

    m_Background->Draw();
    m_Indicator->Draw();

    // 🚀 修正畫圖邏輯：現在 Note 裡面有很多個物件 (陣列)，要跑迴圈把它們全畫出來
    for (auto& note : m_Notes) {
        for (auto& obj : note->GetGameObjects()) {
            obj->Draw();
        }
    }

    m_Pattern->Draw();

    m_WasBlowing = currentBlowing;

    if (Util::Input::IsKeyPressed(Util::Keycode::R)) {
        m_RestartHoldStartTime = SDL_GetTicks();
        m_IsR_Pressed = true;
    } else if (Util::Input::IsKeyDown(Util::Keycode::R)) {
        if (m_IsR_Pressed && (SDL_GetTicks() - m_RestartHoldStartTime >= 800)) {
            RestartLevel();
            m_IsR_Pressed = false;
        }
    } else {
        m_IsR_Pressed = false;
    }
}

void App::End() {}