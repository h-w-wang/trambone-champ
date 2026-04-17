#include "App.hpp"
#include "Util/Transform.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <set>

App::State App::GetCurrentState() const {
    return m_CurrentState;
}

void App::Start() {
    m_Keyboard = std::make_shared<Keyboard>();

    // 1. 預載音效
    for (int i = 0; i <= 24; ++i) {
        std::string filePath = RESOURCE_DIR "/notes/note_" + std::to_string(i) + ".wav";
        m_TromboneNotes.push_back(std::make_shared<Util::SFX>(filePath));
    }

    m_Background = std::make_shared<Util::GameObject>();
    m_Background->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/daohsiang.png"));
    m_Background->SetZIndex(-10.0f);
    m_Background->m_Transform.scale = {1.5f, 1.5f};

    // ==========================================
    // 📐 佈局重做：白鍵橫線、加大間距、消除粗細差
    // ==========================================
    float spacing = 40.0f; // 🚀 間距加大到 40，填滿上下
    float bottomY = -(12 * spacing); // 最低音從 -480 開始

    // 定義白鍵在 25 個半音中的位置 (0=C, 2=D, 4=E, 5=F, 7=G, 9=A, 11=B ...)
    std::set<int> whiteKeyIndices = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24};

    for (int i = 0; i <= 24; ++i) {
        if (whiteKeyIndices.find(i) == whiteKeyIndices.end()) continue; // 🚀 關鍵：只畫白鍵

        float h = bottomY + (i * spacing);
        auto line = std::make_shared<Util::GameObject>();
        line->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));
        line->SetZIndex(-5.0f);

        // 🚀 核心修復：把 X 軸拉到極大(100倍)，並向右偏移，讓淡出的邊緣消失在螢幕外
        line->m_Transform.translation = {1000.0f, h};

        if (i == 0 || i == 12 || i == 24) {
            line->m_Transform.scale = {100.0f, 0.04f}; // Do 的線粗一點點
        } else {
            line->m_Transform.scale = {100.0f, 0.015f}; // 其他白鍵均勻細線
        }
        m_GuideLines.push_back(line);
    }

    // 🎯 垂直判定線 (貫穿全螢幕)
    m_Indicator = std::make_shared<Util::GameObject>();
    m_Indicator->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));
    m_Indicator->m_Transform.translation = {-300.0f, 0.0f};
    m_Indicator->m_Transform.scale = {0.05f, 100.0f};
    m_Indicator->SetZIndex(1.0f);

    m_PatternIdleImage = std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png");
    m_PatternPlayImage = std::make_shared<Util::Image>(RESOURCE_DIR "/player-note-dot-ON.png");
    m_Pattern = std::make_shared<Util::GameObject>();
    m_Pattern->SetDrawable(m_PatternIdleImage);
    m_Pattern->SetZIndex(100.0f);

    // .tmb 解析器 (同步更新高度比例)
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
                    if (!singleNote.empty() && singleNote[0] == '[') singleNote = singleNote.substr(1);
                    for (char& c : singleNote) { if (c == ',') c = ' '; }
                    std::stringstream ss(singleNote);
                    float bStart, bDur, sY, dY, eY;
                    if (ss >> bStart >> bDur >> sY >> dY >> eY) {
                        // 🚀 比例修復：原本是 20，現在間距 40，所以高度乘以 2
                        m_Notes.push_back(std::make_shared<Note>(sY * 2.0f, eY * 2.0f, bStart, bDur));
                    }
                    bracketStart = bracketEnd + 1;
                }
            }
        }
        file.close();
    }

    m_BGM = std::make_shared<Util::BGM>(RESOURCE_DIR "/song.ogg");
    m_BGM->SetVolume(64);
    m_BGM->Play(-1);

    SDL_ShowCursor(SDL_DISABLE);
    m_StartTime = SDL_GetTicks();
    m_CurrentState = State::UPDATE;
}

void App::Update() {
    m_CurrentMusicTime = SDL_GetTicks() - m_StartTime;
    auto cursorPos = Util::Input::GetCursorPosition();

    float currentBeat = (static_cast<float>(m_CurrentMusicTime) / 60000.0f) * 164.0f;

    for (auto& note : m_Notes) {
        note->Update(currentBeat);
    }

    // 🚀 自由移動：取消 std::round，讓升降音能停在兩線中央
    float currentY = cursorPos.y;
    if (currentY > 480.0f) currentY = 480.0f;
    if (currentY < -480.0f) currentY = -480.0f;

    m_Pattern->m_Transform.translation = {-300.0f, currentY};
    m_Pattern->m_Transform.scale = {0.3f, 0.3f};

    bool currentBlowing = Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB) ||
                          Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB) ||
                          Util::Input::IsKeyDown(Util::Keycode::SPACE) ||
                          Util::Input::IsKeyPressed(Util::Keycode::SPACE);

    if (currentBlowing) {
        // 音效判定依然使用 40 像素作為一個半音的級距
        int noteIndex = std::round((currentY + 480.0f) / 40.0f);
        if (noteIndex >= 0 && noteIndex <= 24 && (noteIndex != m_CurrentNoteIndex || !m_WasBlowing)) {
            m_TromboneNotes[noteIndex]->Play(0);
            m_CurrentNoteIndex = noteIndex;
        }
        m_Pattern->SetDrawable(m_PatternPlayImage);
    } else {
        m_Pattern->SetDrawable(m_PatternIdleImage);
    }

    m_Background->Draw();
    for (auto& line : m_GuideLines) line->Draw();
    m_Indicator->Draw();
    for (auto& note : m_Notes) {
        for (auto& obj : note->GetGameObjects()) obj->Draw();
    }
    m_Pattern->Draw();

    m_WasBlowing = currentBlowing;
}

void App::End() {}