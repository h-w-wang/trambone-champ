#include "App.hpp"
#include "Util/Transform.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>

App::State App::GetCurrentState() const {
    return m_CurrentState;
}

void App::Start() {
    m_Keyboard = std::make_shared<Keyboard>();

    m_BGM = std::make_shared<Util::BGM>(RESOURCE_DIR "/song.ogg");
    m_BGM->SetVolume(64);
    m_BGM->Play(-1);

    // 🎺 載入 25 個半音的長號音效
    for (int i = 0; i <= 24; ++i) {
        std::string filePath = RESOURCE_DIR "/notes/note_" + std::to_string(i) + ".wav";
        m_TromboneNotes.push_back(std::make_shared<Util::SFX>(filePath));
    }

    m_Background = std::make_shared<Util::GameObject>();
    m_Background->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/titlescreen_bg.jpg"));
    m_Background->SetZIndex(-10.0f);
    m_Background->m_Transform.scale = {2.0f, 2.0f};

    // 📏 畫出 25 條音高參考線
    for (int i = 0; i <= 24; ++i) {
        float h = -240.0f + (i * 20.0f);
        auto line = std::make_shared<Util::GameObject>();
        line->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));
        line->SetZIndex(-5.0f);
        line->m_Transform.translation = {0.0f, h};

        if (h == 0.0f || h == 240.0f || h == -240.0f) {
            line->m_Transform.scale = {20.0f, 0.04f}; // 主音粗線
        } else {
            line->m_Transform.scale = {20.0f, 0.01f}; // 一般半音細線
        }
        m_GuideLines.push_back(line);
    }

    // 🎯 左側打擊區判定線 (鎖定在 -300.0f)
    m_Indicator = std::make_shared<Util::GameObject>();
    m_Indicator->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));
    m_Indicator->m_Transform.translation = {-300.0f, 0.0f};
    m_Indicator->m_Transform.scale = {1.0f, 2.0f};
    m_Indicator->SetZIndex(1.0f);

    // 🖱️ 游標初始化
    m_PatternIdleImage = std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png");
    m_PatternPlayImage = std::make_shared<Util::Image>(RESOURCE_DIR "/player-note-dot-ON.png");
    m_Pattern = std::make_shared<Util::GameObject>();
    m_Pattern->SetDrawable(m_PatternIdleImage);
    m_Pattern->SetZIndex(100.0f);

    // ==========================================
    // 🤯 生吃 Trombone Champ .tmb 檔案解析器
    // ==========================================
    std::ifstream file(RESOURCE_DIR "/song.tmb");
    if (file.is_open()) {
        std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        size_t startPos = jsonContent.find("\"notes\":[[");
        if (startPos != std::string::npos) {
            startPos += 9;
            size_t endPos = jsonContent.find("]]", startPos);
            if (endPos != std::string::npos) {
                std::string notesStr = jsonContent.substr(startPos, endPos - startPos + 1);
                size_t bracketStart = 0;
                while ((bracketStart = notesStr.find('[', bracketStart)) != std::string::npos) {
                    size_t bracketEnd = notesStr.find(']', bracketStart);
                    if (bracketEnd == std::string::npos) break;
                    std::string singleNote = notesStr.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
                    for (char& c : singleNote) { if (c == ',') c = ' '; }

                    std::stringstream ss(singleNote);
                    float beatStart, beatDuration, startY, diffY, endY;
                    if (ss >> beatStart >> beatDuration >> startY >> diffY >> endY) {
                        m_Notes.push_back(std::make_shared<Note>(startY, endY, beatStart, beatDuration));
                    }
                    bracketStart = bracketEnd + 1;
                }
                std::cout << "[系統] 成功載入 " << m_Notes.size() << " 個音符！\n";
            }
        }
        file.close();
    } else {
        std::cout << "[警告] 找不到 song.tmb 檔案！\n";
    }
    // ==========================================

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

    // 🎵 專業音遊核心：將毫秒轉換為「當下節拍 (Beat)」
    float BPM = 164.0f; // 稻香的 BPM
    float currentBeat = (static_cast<float>(m_CurrentMusicTime) / 60000.0f) * BPM;

    for (auto& note : m_Notes) {
        note->Update(currentBeat);
    }

    // 🧲 磁吸游標效果 & 鎖定在 -300.0f 打擊線上！
    float snappedY = std::round(cursorPos.y / 20.0f) * 20.0f;
    if (snappedY > 240.0f) snappedY = 240.0f;
    if (snappedY < -240.0f) snappedY = -240.0f;
    m_Pattern->m_Transform.translation = {-300.0f, snappedY};
    m_Pattern->m_Transform.scale = {0.3f, 0.3f};

    bool currentBlowing = Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB) ||
                          Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB) ||
                          Util::Input::IsKeyDown(Util::Keycode::SPACE) ||
                          Util::Input::IsKeyPressed(Util::Keycode::SPACE);

    // 🎺 核心發聲引擎
    if (currentBlowing && !m_TromboneNotes.empty()) {
        int noteIndex = std::round((snappedY + 240.0f) / 20.0f);
        if (noteIndex < 0) noteIndex = 0;
        if (noteIndex > 24) noteIndex = 24;

        if (noteIndex != m_CurrentNoteIndex || !m_WasBlowing) {
            if (m_TromboneNotes[noteIndex]) {
                m_TromboneNotes[noteIndex]->Play(0);
            }
            m_CurrentNoteIndex = noteIndex;
        }
    }

    if (currentBlowing) {
        m_Pattern->SetDrawable(m_PatternPlayImage);
    } else {
        m_Pattern->SetDrawable(m_PatternIdleImage);
    }

    // 🎨 完美繪製順序
    m_Background->Draw();
    for (auto& line : m_GuideLines) {
        line->Draw();
    }
    m_Indicator->Draw(); // 畫中間的判定線
    for (auto& note : m_Notes) {
        for (auto& obj : note->GetGameObjects()) {
            obj->Draw();
        }
    }
    m_Pattern->Draw();

    // 🎙️ 內建譜面錄製器
    if (currentBlowing && !m_WasBlowing) {
        m_RecordStartTime = m_CurrentMusicTime;
        m_RecordStartY = cursorPos.y;
    }
    else if (!currentBlowing && m_WasBlowing) {
        float endY = cursorPos.y;
        Uint32 duration = m_CurrentMusicTime - m_RecordStartTime;
        std::ofstream outfile(RESOURCE_DIR "/chart.txt", std::ios_base::app);
        if (outfile.is_open() && duration > 50) {
            int alignedStartY = std::round(m_RecordStartY / 20.0f) * 20;
            int alignedEndY = std::round(endY / 20.0f) * 20;
            outfile << alignedStartY << " " << alignedEndY << " " << m_RecordStartTime << " " << duration << "\n";
        }
    }

    m_WasBlowing = currentBlowing;

    // R鍵重啟
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