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
    m_Pattern->SetZIndex(80.0f); // 🚀 確保游標也在安全範圍內

    // 🚀 暫停選單按鈕：Z-Index 設為 90 (低於極限 100，絕對能顯示！)
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

    std::cout << "========== [開始解析歌單] ==========" << std::endl;
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

                        std::cout << "[成功讀取] 資料夾: " << folder << " | 歌名: " << realName << " | BPM: " << realBpm << std::endl;
                    }
                    songData.push_back({folder, realName, realBpm, realOffset});
                }
            }
        }
    }
    std::cout << "====================================" << std::endl;

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
}

void App::SelectUpdate() {
    m_Keyboard->Update();
    auto mousePos = Util::Input::GetCursorPosition();
    bool mouseClick = Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB);

    if (Util::Input::IsKeyPressed(Util::Keycode::UP) || Util::Input::IsKeyPressed(Util::Keycode::W)) m_MenuScrollY -= 10.0f;
    if (Util::Input::IsKeyPressed(Util::Keycode::DOWN) || Util::Input::IsKeyPressed(Util::Keycode::S)) m_MenuScrollY += 10.0f;

    if (m_TotalMenuHeight > 0) {
        m_MenuScrollY = fmod(m_MenuScrollY, m_TotalMenuHeight);
        if (m_MenuScrollY < 0) m_MenuScrollY += m_TotalMenuHeight;
    }

    // 🚀 已移除 m_Background->Draw()，讓選單變成乾淨的黑色背景！
    m_HoveredSongIndex = -1;
    for (size_t i = 0; i < m_SongButtons.size(); ++i) {
        auto& btn = m_SongButtons[i];
        float spacing = 100.0f;
        float baseSlotY = i * spacing - m_MenuScrollY;
        float wrappedY = fmod(baseSlotY + 400.0f, m_TotalMenuHeight);
        if (wrappedY < 0) wrappedY += m_TotalMenuHeight;
        float finalY = wrappedY - 400.0f;
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

    // 🚀 自動偵測背景圖片格式 (支援 png, jpg, jpeg)
    std::string finalBgPath = "";
    if (fs::exists(baseDir / "bg.png")) {
        finalBgPath = (baseDir / "bg.png").string();
    } else if (fs::exists(baseDir / "bg.jpg")) {
        finalBgPath = (baseDir / "bg.jpg").string();
    } else if (fs::exists(baseDir / "bg.jpeg")) {
        finalBgPath = (baseDir / "bg.jpeg").string();
    }

    // 🚀 如果有找到背景圖就載入，沒找到就給個預設的點點圖防崩潰
    if (!finalBgPath.empty()) {
        m_Background->SetDrawable(std::make_shared<Util::Image>(finalBgPath));
    } else {
        m_Background->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
    }

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
}

void App::Update() {
    m_Keyboard->Update();

    if (m_Keyboard->IsEscDown()) {
        m_CurrentState = State::PAUSE;
        m_PauseStartTime = SDL_GetTicks();
        Mix_PauseMusic(); Mix_Pause(-1);
        m_PrevMouseClick = true;
        return;
    }
    if (m_Keyboard->IsRKeyPressed()) {
        if (!m_IsRHolding) { m_IsRHolding = true; m_RHoldStartTime = SDL_GetTicks(); }
        else if (SDL_GetTicks() - m_RHoldStartTime >= 1000) { LoadSong(m_CurrentSongIndex); return; }
    } else { m_IsRHolding = false; }

    if (m_Keyboard->IsOffsetUp()) {
        m_GlobalOffsetMs += 10.0f;
        std::cout << "[系統微調] 判定延遲: " << m_GlobalOffsetMs << " ms" << std::endl;
    }
    if (m_Keyboard->IsOffsetDown()) {
        m_GlobalOffsetMs -= 10.0f;
        std::cout << "[系統微調] 判定延遲: " << m_GlobalOffsetMs << " ms" << std::endl;
    }

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

    m_Background->Draw();
    for (auto& line : m_GuideLines) line->Draw();
    m_Indicator->Draw();
    for (auto& note : m_Notes) for (auto& obj : note->GetGameObjects()) obj->Draw();
    m_Pattern->Draw();
}

void App::PauseUpdate() {
    m_Keyboard->Update();
    auto mousePos = Util::Input::GetCursorPosition();
    bool click = Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB);

    // ESC 恢復遊戲
    if (m_Keyboard->IsEscDown()) {
        m_TotalPauseDuration += (SDL_GetTicks() - m_PauseStartTime);
        Mix_ResumeMusic(); Mix_Resume(-1);
        m_CurrentState = State::UPDATE;
        return;
    }

    // 畫出凍結的遊戲畫面
    m_Background->Draw();
    for (auto& line : m_GuideLines) line->Draw();
    m_Indicator->Draw();
    for (auto& note : m_Notes) for (auto& obj : note->GetGameObjects()) obj->Draw();
    m_Pattern->Draw();

    // 🚀 畫出 UI 按鈕 (因為放在最後面且 Z-Index=90，它絕對會顯示在最上層)
    int hovered = -1;
    for (size_t i = 0; i < m_PauseButtons.size(); ++i) {
        float tx = 0.0f, ty = 120.0f - (i * 120.0f); // 置中，拉開間距
        m_PauseButtons[i]->m_Transform.translation = {tx, ty};

        if (std::abs(mousePos.x - tx) < 200.0f && std::abs(mousePos.y - ty) < 50.0f) {
            hovered = (int)i;
            m_PauseButtons[i]->m_Transform.scale = {1.2f, 1.2f};
        } else {
            m_PauseButtons[i]->m_Transform.scale = {1.0f, 1.0f};
        }
        m_PauseButtons[i]->Draw();
    }

    // 處理點擊邏輯
    if (hovered != -1 && click && !m_PrevMouseClick) {
        if (hovered == 0) { // 繼續遊戲
            m_TotalPauseDuration += (SDL_GetTicks() - m_PauseStartTime);
            Mix_ResumeMusic(); Mix_Resume(-1);
            m_CurrentState = State::UPDATE;
        } else if (hovered == 1) { // 重新開始
            Mix_ResumeMusic(); Mix_Resume(-1);
            LoadSong(m_CurrentSongIndex);
            m_CurrentState = State::UPDATE;
        } else if (hovered == 2) { // 回到選單
            Mix_ResumeMusic(); Mix_Resume(-1); Mix_HaltChannel(-1); Mix_HaltMusic();
            m_CurrentState = State::SELECT;
        }
    }
    m_PrevMouseClick = click;
}

void App::End() { Mix_HaltChannel(-1); Mix_HaltMusic(); }