#include "App.hpp"
#include "Util/Transform.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <set>
#include <algorithm>
#include <SDL_mixer.h>

App::State App::GetCurrentState() const {
    return m_CurrentState;
}

void App::Start() {
    m_Keyboard = std::make_shared<Keyboard>();

    // 1. 初始化共用音效 (你的 10 秒長號音檔)
    for (int i = 0; i <= 24; ++i) {
        std::string filePath = RESOURCE_DIR "/notes/note_" + std::to_string(i) + ".wav";
        m_TromboneNotes.push_back(std::make_shared<Util::SFX>(filePath));
    }

    // 2. 初始化共通視覺物件
    m_Background = std::make_shared<Util::GameObject>();
    m_Background->SetZIndex(-10.0f);
    m_Background->m_Transform.scale = {1.5f, 1.5f};

    // 佈局：只畫白鍵參考線
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

        if (i == 0 || i == 12 || i == 24) {
            line->m_Transform.scale = {20.0f, 0.04f};
        } else {
            line->m_Transform.scale = {20.0f, 0.015f};
        }
        m_GuideLines.push_back(line);
    }

    // 垂直判定線
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

    // 3. 註冊歌曲清單
    m_SongList = {
        {"song1", "第一首歌 - 預設曲目"},
        {"song2", "第二首歌"},
        {"song3", "第三首歌"}
    };

    // 4. 預設載入第一首歌
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
    Mix_FadeOutChannel(-1, 100);

    m_Background->SetDrawable(std::make_shared<Util::Image>(baseDir + "bg.png"));

    m_BGM = std::make_shared<Util::BGM>(baseDir + "song.ogg");
    m_BGM->SetVolume(64);
    m_BGM->Play(-1);

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

    float currentBeat = (static_cast<float>(m_CurrentMusicTime) / 60000.0f) * 164.0f;

    for (auto& note : m_Notes) {
        note->Update(currentBeat);
    }

    float currentY = cursorPos.y;
    if (currentY > 480.0f) currentY = 480.0f;
    if (currentY < -480.0f) currentY = -480.0f;

    m_Pattern->m_Transform.translation = {-300.0f, currentY};
    m_Pattern->m_Transform.scale = {0.3f, 0.3f};

    bool currentBlowing = Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB) ||
                          Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB) ||
                          Util::Input::IsKeyDown(Util::Keycode::SPACE) ||
                          Util::Input::IsKeyPressed(Util::Keycode::SPACE);

    int targetNoteIndex = std::round((currentY + 480.0f) / 40.0f);
    targetNoteIndex = std::max(0, std::min(24, targetNoteIndex));

    Uint32 currentTime = SDL_GetTicks();
    bool shouldPlay = false;

    // ==========================================
    // 🚀 清爽版：專為 10 秒長音檔打造的平滑滑音
    // ==========================================
    if (currentBlowing) {
        if (!m_WasBlowing) {
            // 情況 A：剛點下去的瞬間
            shouldPlay = true;
            m_CurrentNoteIndex = targetNoteIndex;
        } else if (targetNoteIndex != m_CurrentNoteIndex) {
            // 情況 B：上下滑動產生新音階！
            // 給 30ms 的防震時間，防止頻道被瞬間產生的幾十個聲音塞爆
            if (currentTime - m_LastPlayTime > 30) {
                shouldPlay = true;
                m_CurrentNoteIndex = targetNoteIndex;
            }
        }
        // 注意：移除了「手動延音」，因為你的檔案有 10 秒，只要按著不放它就會自己一直播！

        if (shouldPlay) {
            // 把舊聲音花 20 毫秒溫和淡出，完美釋放頻道，杜絕冷卻期！
            Mix_FadeOutChannel(-1, 20);
            if (m_CurrentNoteIndex >= 0 && m_CurrentNoteIndex <= 24) {
                // 傳入 -1 讓它無限循環，就算你按超過 10 秒也不會斷
                m_TromboneNotes[m_CurrentNoteIndex]->Play(-1);
            }
            m_LastPlayTime = currentTime;
        }
        m_Pattern->SetDrawable(m_PatternPlayImage);
    } else {
        // 放開按鍵時，給一個 50 毫秒的聲音尾巴淡出，聽起來更自然
        if (m_WasBlowing) {
            Mix_FadeOutChannel(-1, 50);
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
    Mix_FadeOutChannel(-1, 200);
}