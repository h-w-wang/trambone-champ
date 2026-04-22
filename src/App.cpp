#include "App.hpp"
#include "Util/Transform.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <set>
#include <algorithm>
#include <SDL_mixer.h>
#include <SDL.h>

App::State App::GetCurrentState() const {
    return m_CurrentState;
}

void App::Start() {
    m_Keyboard = std::make_shared<Keyboard>();

    // 1. 初始化 25 個長號音效
    for (int i = 0; i <= 24; ++i) {
        std::string filePath = RESOURCE_DIR "/notes/note_" + std::to_string(i) + ".wav";
        m_TromboneNotes.push_back(std::make_shared<Util::SFX>(filePath));
    }

    // 2. 初始化背景與參考線
    m_Background = std::make_shared<Util::GameObject>();
    m_Background->SetZIndex(-10.0f);
    m_Background->m_Transform.scale = {1.5f, 1.5f};

    float spacing = 40.0f;
    float bottomY = -(12 * spacing);
    std::set<int> whiteKeyIndices = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24};

    for (int i = 0; i <= 24; ++i) {
        if (whiteKeyIndices.find(i) == whiteKeyIndices.end()) continue;
        float h = bottomY + (i * spacing);
        auto line = std::make_shared<Util::GameObject>();
        line->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));
        line->SetZIndex(-5.0f);
        line->m_Transform.translation = {300.0f, h};
        line->m_Transform.scale = (i == 0 || i == 12 || i == 24) ? glm::vec2{20.0f, 0.04f} : glm::vec2{20.0f, 0.015f};
        m_GuideLines.push_back(line);
    }

    m_Indicator = std::make_shared<Util::GameObject>();
    m_Indicator->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));
    m_Indicator->m_Transform.translation = {-300.0f, 0.0f};
    m_Indicator->m_Transform.scale = {0.05f, 20.0f};
    m_Indicator->SetZIndex(1.0f);

    m_PatternIdleImage = std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png");
    m_PatternPlayImage = std::make_shared<Util::Image>(RESOURCE_DIR "/player-note-dot-ON.png");
    m_Pattern = std::make_shared<Util::GameObject>();
    m_Pattern->SetDrawable(m_PatternIdleImage);
    m_Pattern->SetZIndex(100.0f);

    // 3. 註冊歌曲清單 (稻香 BPM=164)
    m_SongList = {
        {"song1", "稻香", 164.0f, 0.0f},
        {"song2", "第二首歌", 164.0f, 0.0f},
        {"song3", "第三首歌", 120.0f, 0.0f}
    };

    LoadSong(0);
    SDL_ShowCursor(SDL_DISABLE);
    m_CurrentState = State::UPDATE;
}

void App::LoadSong(int index) {
    if (index < 0 || index >= static_cast<int>(m_SongList.size())) return;
    if (index == m_CurrentSongIndex) return;

    m_CurrentSongIndex = index;
    const auto& song = m_SongList[index];
    std::string baseDir = RESOURCE_DIR "/songs/" + song.folderName + "/";

    m_Notes.clear();
    Mix_HaltChannel(-1);

    m_Background->SetDrawable(std::make_shared<Util::Image>(baseDir + "bg.png"));

    m_BGM = std::make_shared<Util::BGM>(baseDir + "song.ogg");
    m_BGM->SetVolume(64);

    // 解析 .tmb 譜面
    std::ifstream file(baseDir + "song.tmb");
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
                        m_Notes.push_back(std::make_shared<Note>(sY * 2.0f, eY * 2.0f, bStart, bDur));
                    }
                    bracketStart = bracketEnd + 1;
                }
            }
        }
        file.close();
    }

    // 音樂與碼表同步啟動
    m_BGM->Play(-1);
    m_StartTime = SDL_GetTicks();

    m_CurrentMusicTime = 0;
    m_WasBlowing = false;
    m_CurrentNoteIndex = -1;
    m_LastPlayTime = 0;
    std::cout << "已成功載入歌曲: " << song.displayName << std::endl;
}

void App::Update() {
    if (Util::Input::IsKeyPressed(Util::Keycode::NUM_1)) LoadSong(0);
    if (Util::Input::IsKeyPressed(Util::Keycode::NUM_2)) LoadSong(1);
    if (Util::Input::IsKeyPressed(Util::Keycode::NUM_3)) LoadSong(2);

    m_CurrentMusicTime = SDL_GetTicks() - m_StartTime;
    auto cursorPos = Util::Input::GetCursorPosition();

    const auto& currentSong = m_SongList[m_CurrentSongIndex];
    float adjustedTime = static_cast<float>(m_CurrentMusicTime) + currentSong.offsetMs;
    float currentBeat = (adjustedTime / 60000.0f) * currentSong.bpm;

    for (auto& note : m_Notes) {
        note->Update(currentBeat);
    }

    float currentY = std::clamp(cursorPos.y, -480.0f, 480.0f);
    m_Pattern->m_Transform.translation = {-300.0f, currentY};
    m_Pattern->m_Transform.scale = {0.3f, 0.3f};


    Uint32 mouseState = SDL_GetMouseState(NULL, NULL);
    bool mouseBlowing = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    const Uint8* keyState = SDL_GetKeyboardState(NULL);
    bool spaceBlowing = keyState[SDL_SCANCODE_SPACE];

    bool currentBlowing = mouseBlowing || spaceBlowing;


    float noteFloat = (currentY + 480.0f) / 40.0f;
    int targetNoteIndex = m_CurrentNoteIndex;

    if (m_CurrentNoteIndex == -1 || !m_WasBlowing) {
        targetNoteIndex = std::clamp(static_cast<int>(std::round(noteFloat)), 0, 24);
    } else {
        if (std::abs(noteFloat - m_CurrentNoteIndex) > 0.6f) {
            targetNoteIndex = std::clamp(static_cast<int>(std::round(noteFloat)), 0, 24);
        }
    }

    Uint32 currentTime = SDL_GetTicks();
    bool shouldPlay = false;

    if (currentBlowing) {
        if (!m_WasBlowing) {
            shouldPlay = true;
            m_CurrentNoteIndex = targetNoteIndex;
        } else if (targetNoteIndex != m_CurrentNoteIndex) {
            if (currentTime - m_LastPlayTime > 30) {
                shouldPlay = true;
                m_CurrentNoteIndex = targetNoteIndex;
            }
        }

        if (shouldPlay) {
            Mix_HaltChannel(-1);
            if (m_CurrentNoteIndex >= 0 && m_CurrentNoteIndex <= 24) {
                m_TromboneNotes[m_CurrentNoteIndex]->Play(-1);
            }
            m_LastPlayTime = currentTime;
        }
        m_Pattern->SetDrawable(m_PatternPlayImage);
    } else {
        if (m_WasBlowing) {
            Mix_HaltChannel(-1);
        }
        m_Pattern->SetDrawable(m_PatternIdleImage);
    }

    m_WasBlowing = currentBlowing;

    // 繪製順序
    m_Background->Draw();
    for (auto& line : m_GuideLines) line->Draw();
    m_Indicator->Draw();
    for (auto& note : m_Notes) {
        for (auto& obj : note->GetGameObjects()) obj->Draw();
    }
    m_Pattern->Draw();
}

void App::End() {
    Mix_HaltChannel(-1);
}