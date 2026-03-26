#include "App.hpp"
#include "Util/Transform.hpp"
#include <iostream>

App::State App::GetCurrentState() const {
    return m_CurrentState;
}

void App::Start() {
    m_Keyboard = std::make_shared<Keyboard>();

    // 載入背景音樂
    m_BGM = std::make_shared<Util::BGM>(RESOURCE_DIR "/test_music.mp3");
    m_BGM->SetVolume(64);
    m_BGM->Play(-1);

    // 載入 5 個音階的音效
    /*m_TromboneNotes.push_back(std::make_shared<Util::SFX>(RESOURCE_DIR "/note_1.wav"));
    m_TromboneNotes.push_back(std::make_shared<Util::SFX>(RESOURCE_DIR "/note_2.wav"));
    m_TromboneNotes.push_back(std::make_shared<Util::SFX>(RESOURCE_DIR "/note_3.wav"));
    m_TromboneNotes.push_back(std::make_shared<Util::SFX>(RESOURCE_DIR "/note_4.wav"));
    m_TromboneNotes.push_back(std::make_shared<Util::SFX>(RESOURCE_DIR "/note_5.wav"));
    for(auto& note : m_TromboneNotes) note->SetVolume(100);*/

    // 建立游標
    m_Cursor = std::make_shared<Util::GameObject>();
    m_Cursor->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note.png"));
    m_Cursor->SetZIndex(10.0f);

    // ==========================================
    // UI：打擊判定線 (固定在畫面偏左側 X = -300)
    // ==========================================
    m_JudgmentLine = std::make_shared<Util::GameObject>();
    m_JudgmentLine->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note.png"));
    m_JudgmentLine->SetZIndex(1.0f); // 畫在最底層
    m_JudgmentLine->m_Transform.translation = {-300.0f, 0.0f};
    m_JudgmentLine->m_Transform.scale = {0.1f, 5.0f}; // 寬度變細，高度拉長變成一條線

    // ==========================================
    // 測試資料：生成幾個滾動的音符
    // 第一顆音符會在音樂播放到 2000 毫秒 (第 2 秒) 時抵達判定線
    // ==========================================
    m_Notes.push_back(std::make_shared<Note>(0.0f, 2000));     // 2秒時，在中間抵達
    m_Notes.push_back(std::make_shared<Note>(150.0f, 3000));   // 3秒時，在偏高處抵達
    m_Notes.push_back(std::make_shared<Note>(-150.0f, 4000));  // 4秒時，在偏低處抵達
    m_Notes.push_back(std::make_shared<Note>(0.0f, 5000));     // 5秒時，在中間抵達

    // 隱藏系統滑鼠箭頭
    SDL_ShowCursor(SDL_DISABLE);

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

    // 1. 更新與繪製 UI 判定線
    m_JudgmentLine->Draw();

    // 2. 更新與繪製所有滾動中的音符
    for (auto& note : m_Notes) {
        note->Update(m_CurrentMusicTime);
        note->Draw();
    }

    // 3. 更新游標位置
    auto cursorPos = Util::Input::GetCursorPosition();
    m_Cursor->m_Transform.translation = cursorPos;
    m_Cursor->m_Transform.scale = {0.5f, 0.5f};
    m_Cursor->Draw();

    // 4. 音階判定邏輯 (滑鼠 Y 座標轉陣列 Index)
    float windowTop = 300.0f;
    float windowBottom = -300.0f;
    float ratio = (cursorPos.y - windowBottom) / (windowTop - windowBottom);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio >= 1.0f) ratio = 0.999f;

    int targetNoteIndex = static_cast<int>(ratio * m_TromboneNotes.size());
    bool currentBlowing = m_Keyboard->IsBlowing();

    if (currentBlowing) {
        if (!m_WasBlowing || targetNoteIndex != m_CurrentNoteIndex) {
            m_TromboneNotes[targetNoteIndex]->Play();
            m_CurrentNoteIndex = targetNoteIndex;
        }
    } else {
        m_CurrentNoteIndex = -1;
    }
    m_WasBlowing = currentBlowing;

    // 5. 遊戲控制邏輯
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

    // 點擊左上角退出遊戲
    if (Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB)) {
        if (cursorPos.x < -400 && cursorPos.y > 250) {
            std::cout << "點擊了退出按鈕，結束遊戲。\n";
            m_CurrentState = State::END;
        }
    }
}

void App::End() {}