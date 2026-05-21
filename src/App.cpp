#include "App.hpp"
#include "Util/Transform.hpp"
#include "Util/Text.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <set>
#include <algorithm>
#include <filesystem>
#include <utility>
#include <SDL_mixer.h>
#include <SDL.h>

namespace fs = std::filesystem;

namespace {
    bool g_RequireInputRelease = false;

    // 🚀 視覺化變數：改為填滿效果
    std::shared_ptr<Util::GameObject> s_RestartText = nullptr;
    std::shared_ptr<Util::GameObject> s_RestartFill = nullptr;
    std::shared_ptr<Util::GameObject> s_RestartBG = nullptr;
}

App::State App::GetCurrentState() const {
    return m_CurrentState;
}

void App::Start() {
    m_Keyboard = std::make_shared<Keyboard>();

    for (int i = 0; i <= 24; ++i) {
        std::string filePath = RESOURCE_DIR "/notes/note_" + std::to_string(i) + ".wav";
        m_TromboneNotes.push_back(std::make_shared<Util::SFX>(filePath));
    }

    m_Background = std::make_shared<Util::GameObject>();
    m_Background->SetZIndex(-10.0f);
    m_Background->m_Transform.scale = {1.5f, 1.5f};
    m_Background->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));

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
    m_Pattern->SetZIndex(80.0f);

    std::vector<std::string> pauseTexts = {"繼續遊戲", "重新開始", "回到選單"};
    for (int i = 0; i < 3; ++i) {
        auto btn = std::make_shared<Util::GameObject>();
        try {
            btn->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 65, pauseTexts[i], SDL_Color{255, 255, 0, 255}));
        } catch (...) {
            btn->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
        }
        btn->SetZIndex(90.0f);
        m_PauseButtons.push_back(btn);
    }

    m_SongList.clear();
    m_SongButtons.clear();
    fs::path songDir = fs::path(RESOURCE_DIR) / "songs";

    auto parseJsonFloat = [](const std::string& json, const std::string& key, float defaultVal) -> float {
        size_t keyPos = json.find("\"" + key + "\"");
        if (keyPos == std::string::npos) return defaultVal;
        size_t colonPos = json.find(":", keyPos);
        if (colonPos == std::string::npos) return defaultVal;
        size_t numStart = json.find_first_of("-0123456789.", colonPos);
        if (numStart == std::string::npos) return defaultVal;
        size_t numEnd = json.find_first_not_of("-0123456789.", numStart);
        try {
            return std::stof(json.substr(numStart, numEnd - numStart));
        } catch (...) { return defaultVal; }
    };

    struct ParsedSong {
        std::string folder;
        std::string name;
        float bpm;
        float offset;
    };
    std::vector<ParsedSong> songData;

    if (fs::exists(songDir) && fs::is_directory(songDir)) {
        for (const auto& entry : fs::directory_iterator(songDir)) {
            if (entry.is_directory()) {
                std::string folder = entry.path().filename().string();
                fs::path tmbPath = entry.path() / "song.tmb";
                if (fs::exists(tmbPath)) {
                    std::string realName = folder;
                    float realBpm = 120.0f;
                    float realOffset = 0.0f;
                    std::ifstream tmbFile(tmbPath.string());
                    if (tmbFile.is_open()) {
                        std::string content((std::istreambuf_iterator<char>(tmbFile)), std::istreambuf_iterator<char>());
                        size_t namePos = content.find("\"name\"");
                        if (namePos != std::string::npos) {
                            size_t fQ = content.find("\"", content.find(":", namePos));
                            size_t sQ = content.find("\"", fQ + 1);
                            if (fQ != std::string::npos && sQ != std::string::npos)
                                realName = content.substr(fQ + 1, sQ - fQ - 1);
                        }
                        realBpm = parseJsonFloat(content, "tempo", 120.0f);
                        if (realBpm == 120.0f) realBpm = parseJsonFloat(content, "bpm", 120.0f);
                        realOffset = parseJsonFloat(content, "offset", 0.0f);
                    }
                    songData.push_back({folder, realName, realBpm, realOffset});
                }
            }
        }
    }

    std::sort(songData.begin(), songData.end(), [](const auto& a, const auto& b) {
        if (a.folder.length() != b.folder.length()) return a.folder.length() < b.folder.length();
        return a.folder < b.folder;
    });

    for (const auto& data : songData) {
        m_SongList.push_back({data.folder, data.name, data.bpm, data.offset});
        auto textObj = std::make_shared<Util::GameObject>();
        try {
            textObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 45, data.name, SDL_Color{255, 255, 255, 255}));
        } catch (...) {
            textObj->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
        }
        textObj->SetZIndex(50.0f);
        m_SongButtons.push_back(textObj);
    }

    m_TotalMenuHeight = m_SongList.size() * 100.0f;
    m_CurrentState = State::SELECT;
    m_LastFrameTime = SDL_GetTicks();
}

void App::SelectUpdate() {
    Uint32 currentTicks = SDL_GetTicks();
    float dt = (currentTicks - m_LastFrameTime) / 1000.0f;
    if (dt > 0.1f) dt = 0.1f;
    m_LastFrameTime = currentTicks;

    m_Keyboard->Update();
    auto mousePos = Util::Input::GetCursorPosition();
    bool mouseClick = Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB);

    float scrollSpeed = 1500.0f * dt;
    if (Util::Input::IsKeyPressed(Util::Keycode::UP) || Util::Input::IsKeyPressed(Util::Keycode::W)) m_TargetScrollY -= scrollSpeed;
    if (Util::Input::IsKeyPressed(Util::Keycode::DOWN) || Util::Input::IsKeyPressed(Util::Keycode::S)) m_TargetScrollY += scrollSpeed;

    m_MenuScrollY += (m_TargetScrollY - m_MenuScrollY) * 12.0f * dt;

    if (m_TotalMenuHeight > 0) {
        m_MenuScrollY = fmod(m_MenuScrollY, m_TotalMenuHeight);
        if (m_MenuScrollY < 0) m_MenuScrollY += m_TotalMenuHeight;
    }

    // 🚀 已將 m_Background->Draw() 移除，選單背景現在會是全黑的！

    m_HoveredSongIndex = -1;
    for (size_t i = 0; i < m_SongButtons.size(); ++i) {
        auto& btn = m_SongButtons[i];
        float spacing = 100.0f;
        float baseSlotY = i * spacing - m_MenuScrollY;
        float wrappedY = fmod(baseSlotY + 400.0f, m_TotalMenuHeight);
        if (wrappedY < 0) wrappedY += m_TotalMenuHeight;
        float finalY = wrappedY - 400.0f;

        if (finalY < -500.0f || finalY > 500.0f) continue;

        btn->m_Transform.translation = {0.0f, -finalY};
        if (std::abs(mousePos.y - (-finalY)) < 40.0f) {
            m_HoveredSongIndex = static_cast<int>(i);
            btn->m_Transform.scale = {1.2f, 1.2f};
        } else {
            btn->m_Transform.scale = {1.0f, 1.0f};
        }
        btn->Draw();
    }

    if (m_HoveredSongIndex != -1 && mouseClick && !m_PrevMouseClick) {
        LoadSong(m_HoveredSongIndex);
        m_CurrentState = State::UPDATE;
        g_RequireInputRelease = true;
    }
    m_PrevMouseClick = mouseClick;
}

void App::LoadSong(int index) {
    if (index < 0 || index >= (int)m_SongList.size()) return;
    m_CurrentSongIndex = index;
    const auto& song = m_SongList[index];
    fs::path baseDir = fs::path(RESOURCE_DIR) / "songs" / song.folderName;
    m_Notes.clear();
    Mix_HaltChannel(-1);
    Mix_HaltMusic();

    std::string finalBgPath = "";
    if (fs::exists(baseDir / "bg.png")) finalBgPath = (baseDir / "bg.png").string();
    else if (fs::exists(baseDir / "bg.jpg")) finalBgPath = (baseDir / "bg.jpg").string();
    else if (fs::exists(baseDir / "bg.jpeg")) finalBgPath = (baseDir / "bg.jpeg").string();

    if (!finalBgPath.empty()) m_Background->SetDrawable(std::make_shared<Util::Image>(finalBgPath));
    else m_Background->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));

    m_BGM = std::make_shared<Util::BGM>((baseDir / "song.ogg").string());
    m_BGM->SetVolume(64);

    std::ifstream file((baseDir / "song.tmb").string());
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    size_t startPos = content.find("\"notes\":[[");
    if (startPos != std::string::npos) {
        startPos += 9;
        size_t endPos = content.find("]]", startPos);
        std::string notesStr = content.substr(startPos, endPos - startPos + 1);
        size_t bS = 0;
        while ((bS = notesStr.find('[', bS)) != std::string::npos) {
            size_t bE = notesStr.find(']', bS);
            std::string sN = notesStr.substr(bS + 1, bE - bS - 1);
            for (char& c : sN) if (c == ',') c = ' ';
            std::stringstream ss(sN);
            float bStart, bDur, sY, dY, eY;
            if (ss >> bStart >> bDur >> sY >> dY >> eY) m_Notes.push_back(std::make_shared<Note>(sY * 2.0f, eY * 2.0f, bStart, bDur));
            bS = bE + 1;
        }
    }
    file.close();

    m_BGM->Play(-1);
    m_StartTime = SDL_GetTicks();
    m_CurrentMusicTime = 0;
    m_TotalPauseDuration = 0;
    m_IsRHolding = false;
    m_RequireRRelease = false;
}

void App::Update() {
    m_Keyboard->Update();

    if (m_Keyboard->IsEscDown()) {
        m_CurrentState = State::PAUSE;
        m_PauseStartTime = SDL_GetTicks();
        Mix_PauseMusic();
        Mix_HaltChannel(-1);
        m_WasBlowing = false;
        m_PrevMouseClick = true;
        return;
    }

    // 🚀 防連觸機制：必須放開 R 鍵才能觸發下一次
    if (m_Keyboard->IsRKeyPressed()) {
        if (!m_RequireRRelease) {
            if (!m_IsRHolding) {
                m_IsRHolding = true;
                m_RHoldStartTime = SDL_GetTicks();
            }
        }
    } else {
        m_IsRHolding = false;
        m_RequireRRelease = false; // 放開按鍵，解鎖
    }

    if (m_Keyboard->IsOffsetUp()) m_GlobalOffsetMs += 10.0f;
    if (m_Keyboard->IsOffsetDown()) m_GlobalOffsetMs -= 10.0f;

    m_CurrentMusicTime = SDL_GetTicks() - m_StartTime - m_TotalPauseDuration;
    const auto& song = m_SongList[m_CurrentSongIndex];
    float adjustedTime = static_cast<float>(m_CurrentMusicTime) + song.offsetMs + m_GlobalOffsetMs;
    float currentBeat = (adjustedTime / 60000.0f) * song.bpm;

    for (auto& note : m_Notes) note->Update(currentBeat);

    auto cursorPos = Util::Input::GetCursorPosition();
    float currentY = std::clamp(cursorPos.y, -480.0f, 480.0f);
    m_Pattern->m_Transform.translation = {-300.0f, currentY};

    bool blowing = m_Keyboard->IsBlowing();
    if (g_RequireInputRelease) { if (!blowing) g_RequireInputRelease = false; blowing = false; }

    int targetIdx = std::clamp((int)std::round((currentY + 480.0f) / 40.0f), 0, 24);
    if (blowing) {
        if (!m_WasBlowing || targetIdx != m_CurrentNoteIndex) {
            Mix_HaltChannel(-1);
            m_TromboneNotes[targetIdx]->Play(-1);
            m_CurrentNoteIndex = targetIdx;
        }
        m_Pattern->SetDrawable(m_PatternPlayImage);
    } else {
        if (m_WasBlowing) Mix_HaltChannel(-1);
        m_Pattern->SetDrawable(m_PatternIdleImage);
    }
    m_WasBlowing = blowing;

    // 畫出背景與軌道
    m_Background->Draw();
    for (auto& line : m_GuideLines) line->Draw();
    m_Indicator->Draw();

    // 畫出範圍內的音符
    for (auto& note : m_Notes) {
        auto& objs = note->GetGameObjects();
        if (objs.empty()) continue;
        float headX = objs[0]->m_Transform.translation.x;
        float endX = objs[1]->m_Transform.translation.x;
        if (endX < -1000.0f || headX > 1000.0f) continue;
        for (auto& obj : objs) obj->Draw();
    }
    m_Pattern->Draw();

    // 🚀 畫出由內向外覆蓋的同心圓 (取代扇形)
    if (m_IsRHolding) {
        float progress = std::clamp((SDL_GetTicks() - m_RHoldStartTime) / 1000.0f, 0.0f, 1.0f);

        if (!s_RestartText) {
            s_RestartText = std::make_shared<Util::GameObject>();
            s_RestartText->SetZIndex(99.0f);

            s_RestartFill = std::make_shared<Util::GameObject>();
            s_RestartFill->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/player-note-dot-ON.png")); // 亮色圓圈
            s_RestartFill->SetZIndex(98.0f);

            s_RestartBG = std::make_shared<Util::GameObject>();
            s_RestartBG->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png")); // 暗色圓圈
            s_RestartBG->SetZIndex(97.0f);
        }

        int percent = static_cast<int>(progress * 100);
        try {
            s_RestartText->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 40, "RESTARTING... " + std::to_string(percent) + "%", SDL_Color{255, 255, 0, 255}));
        } catch (...) {}

        s_RestartText->m_Transform.translation = {0.0f, -120.0f};

        // 底圖圓圈大小固定
        s_RestartBG->m_Transform.translation = {0.0f, 0.0f};
        s_RestartBG->m_Transform.scale = {2.0f, 2.0f};

        // 🚀 填滿圓圈隨時間變大，完美覆蓋底圖
        s_RestartFill->m_Transform.translation = {0.0f, 0.0f};
        s_RestartFill->m_Transform.scale = {2.0f * progress, 2.0f * progress};

        s_RestartBG->Draw();
        s_RestartFill->Draw();
        s_RestartText->Draw();

        // 🚀 滿 100% 執行重來，並立刻鎖定，強迫玩家放開按鍵
        if (progress >= 1.0f) {
            LoadSong(m_CurrentSongIndex);
            g_RequireInputRelease = true;
            m_RequireRRelease = true; // 上鎖
            m_IsRHolding = false;
            return;
        }
    }
}

void App::PauseUpdate() {
    m_Keyboard->Update();
    auto mousePos = Util::Input::GetCursorPosition();
    bool click = Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB);

    if (m_Keyboard->IsEscDown()) {
        m_TotalPauseDuration += (SDL_GetTicks() - m_PauseStartTime);
        Mix_ResumeMusic();
        m_CurrentState = State::UPDATE;
        g_RequireInputRelease = true;
        m_LastFrameTime = SDL_GetTicks();
        return;
    }

    if (m_Keyboard->IsRKeyPressed()) {
        if (!m_RequireRRelease) {
            if (!m_IsRHolding) {
                m_IsRHolding = true;
                m_RHoldStartTime = SDL_GetTicks();
            }
        }
    } else {
        m_IsRHolding = false;
        m_RequireRRelease = false;
    }

    m_Background->Draw();
    for (auto& line : m_GuideLines) line->Draw();
    m_Indicator->Draw();
    for (auto& note : m_Notes) {
        auto& objs = note->GetGameObjects();
        if (objs.empty()) continue;
        float headX = objs[0]->m_Transform.translation.x;
        float endX = objs[1]->m_Transform.translation.x;
        if (endX < -1000.0f || headX > 1000.0f) continue;
        for (auto& obj : objs) obj->Draw();
    }
    m_Pattern->Draw();

    int hovered = -1;
    for (size_t i = 0; i < m_PauseButtons.size(); ++i) {
        float tx = 0.0f, ty = 150.0f - (i * 150.0f);
        m_PauseButtons[i]->m_Transform.translation = {tx, ty};
        if (std::abs(mousePos.x - tx) < 250.0f && std::abs(mousePos.y - ty) < 60.0f) {
            hovered = (int)i; m_PauseButtons[i]->m_Transform.scale = {1.3f, 1.3f};
        } else { m_PauseButtons[i]->m_Transform.scale = {1.0f, 1.0f}; }
        m_PauseButtons[i]->Draw();
    }

    if (m_IsRHolding) {
        float progress = std::clamp((SDL_GetTicks() - m_RHoldStartTime) / 1000.0f, 0.0f, 1.0f);
        if (!s_RestartText) {
            s_RestartText = std::make_shared<Util::GameObject>();
            s_RestartText->SetZIndex(99.0f);
            s_RestartFill = std::make_shared<Util::GameObject>();
            s_RestartFill->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/player-note-dot-ON.png"));
            s_RestartFill->SetZIndex(98.0f);
            s_RestartBG = std::make_shared<Util::GameObject>();
            s_RestartBG->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
            s_RestartBG->SetZIndex(97.0f);
        }
        int percent = static_cast<int>(progress * 100);
        try {
            s_RestartText->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 40, "RESTARTING... " + std::to_string(percent) + "%", SDL_Color{255, 255, 0, 255}));
        } catch(...) {}

        s_RestartText->m_Transform.translation = {0.0f, -120.0f};

        s_RestartBG->m_Transform.translation = {0.0f, 0.0f};
        s_RestartBG->m_Transform.scale = {2.0f, 2.0f};

        s_RestartFill->m_Transform.translation = {0.0f, 0.0f};
        s_RestartFill->m_Transform.scale = {2.0f * progress, 2.0f * progress};

        s_RestartBG->Draw();
        s_RestartFill->Draw();
        s_RestartText->Draw();

        if (progress >= 1.0f) {
            Mix_ResumeMusic();
            LoadSong(m_CurrentSongIndex);
            m_CurrentState = State::UPDATE;
            g_RequireInputRelease = true;
            m_RequireRRelease = true;
            m_IsRHolding = false;
            return;
        }
    }

    if (hovered != -1 && click && !m_PrevMouseClick) {
        if (hovered == 0) {
            m_TotalPauseDuration += (SDL_GetTicks() - m_PauseStartTime);
            Mix_ResumeMusic();
            m_CurrentState = State::UPDATE;
            g_RequireInputRelease = true;
            m_LastFrameTime = SDL_GetTicks();
        } else if (hovered == 1) {
            Mix_ResumeMusic();
            LoadSong(m_CurrentSongIndex);
            m_CurrentState = State::UPDATE;
            g_RequireInputRelease = true;
        } else if (hovered == 2) {
            Mix_ResumeMusic();
            Mix_HaltChannel(-1); Mix_HaltMusic();
            m_CurrentState = State::SELECT;
            g_RequireInputRelease = true;
        }
    }
    m_PrevMouseClick = click;
}

void App::End() { Mix_HaltChannel(-1); Mix_HaltMusic(); }