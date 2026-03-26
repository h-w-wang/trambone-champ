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

    // 1. 背景圖片
    m_Background = std::make_shared<Util::GameObject>();
    m_Background->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/titlescreen_bg.jpg"));

    // ==========================================
    // 2. [視覺修正] 左側那條白線：換成 warmup-light.png，並壓細長
    // ==========================================
    m_Indicator = std::make_shared<Util::GameObject>();
    m_Indicator->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));
    m_Indicator->m_Transform.translation = {-300.0f, 0.0f}; // 固定在左邊 X = -300
    m_Indicator->m_Transform.scale = {1.0f, 2.0f}; // 寬度縮到 0.02 變成一條細長白線！

    // 3. 圓點游標 (Idle)
    m_PatternIdleImage = std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png");
    // 4. 圓點游標 (Play - 按下去發光)
    m_PatternPlayImage = std::make_shared<Util::Image>(RESOURCE_DIR "/player-note-dot-ON.png");

    // 建立圓點游標物件
    m_Pattern = std::make_shared<Util::GameObject>();
    m_Pattern->SetDrawable(m_PatternIdleImage);

    // [測試用] 這裡原本是飛過來的東西，我們不用動它，因為下一步我們要直接改寫音符的邏輯！
    m_Notes.push_back(std::make_shared<Note>(0.0f, 2000));
    m_Notes.push_back(std::make_shared<Note>(150.0f, 3000));
    m_Notes.push_back(std::make_shared<Note>(-150.0f, 4000));
    m_Notes.push_back(std::make_shared<Note>(0.0f, 5000));

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
    m_Keyboard->Update();
    auto cursorPos = Util::Input::GetCursorPosition();

    // 更新 Note 座標
    for (auto& note : m_Notes) {
        note->Update(m_CurrentMusicTime);
    }

    // ==========================================
    // 2. [邏輯修正] 更新圓點游標 (小白點)
    // 核心解法：死死鎖在白線上 (X = -300)，並且疊在白線上方，只有 Y 軸跟著滑鼠滑動！
    // ==========================================
    m_Pattern->m_Transform.translation = {-300.0f, cursorPos.y};
    m_Pattern->m_Transform.scale = {0.3f, 0.3f}; // 圓點的大小

    // 點擊變換圖片邏輯
    bool currentBlowing = m_Keyboard->IsBlowing();
    if (currentBlowing) {
        m_Pattern->SetDrawable(m_PatternPlayImage);
    } else {
        m_Pattern->SetDrawable(m_PatternIdleImage);
    }

    // 繪圖順序：背景 -> 白線 -> 音符 -> 圓點 (點點要在最上面)
    m_Background->Draw();
    m_Indicator->Draw();
    for (auto& note : m_Notes) {
        note->GetGameObject()->Draw();
    }
    m_Pattern->Draw();

    m_WasBlowing = currentBlowing;

    // 控制邏輯
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

    if (Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB)) {
        if (cursorPos.x < -400 && cursorPos.y > 250) {
            m_CurrentState = State::END;
        }
    }
}

void App::End() {}