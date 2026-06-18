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
    std::shared_ptr<Util::GameObject> s_RestartText = nullptr;
    std::shared_ptr<Util::GameObject> s_RestartFill = nullptr;
    std::shared_ptr<Util::GameObject> s_RestartBG = nullptr;
    std::shared_ptr<Util::GameObject> s_CountdownText = nullptr;
    std::shared_ptr<Util::GameObject> s_CountdownBG = nullptr;
}

App::State App::GetCurrentState() const { return m_CurrentState; }

void App::Start() {
    m_Keyboard = std::make_shared<Keyboard>();
    m_LastFrameTime = SDL_GetTicks();
    SDL_ShowCursor(SDL_ENABLE);

    for (int i = 0; i <= 24; ++i) {
        std::string filePath = RESOURCE_DIR "/notes/note_" + std::to_string(i) + ".wav";
        m_TromboneNotes.push_back(std::make_shared<Util::SFX>(filePath));
    }

    m_Background = std::make_shared<Util::GameObject>();
    m_Background->SetZIndex(-10.0f);
    m_Background->m_Transform.scale = {1.5f, 1.5f};
    m_Background->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));

    m_PauseOverlay = std::make_shared<Util::GameObject>();
    m_PauseOverlay->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"));
    m_PauseOverlay->SetZIndex(140.0f);
    m_PauseOverlay->m_Transform.scale = {100.0f, 100.0f};

    float spacing = 25.0f;
    float bottomY = -(12 * spacing);
    std::set<int> whiteKeyIndices = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24};
    for (int i = 0; i <= 24; ++i) {
        if (whiteKeyIndices.find(i) == whiteKeyIndices.end()) continue;
        auto line = std::make_shared<Util::GameObject>();
        line->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));
        line->SetZIndex(-5.0f);
        line->m_Transform.translation = {0.0f, bottomY + (i * spacing)};
        line->m_Transform.scale = (i == 0 || i == 12 || i == 24) ? glm::vec2{30.0f, 0.09f} : glm::vec2{30.0f, 0.02f};
        m_GuideLines.push_back(line);
    }
    m_Indicator = std::make_shared<Util::GameObject>();
    m_Indicator->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light.png"));
    m_Indicator->m_Transform.translation = {-350.0f, 0.0f};
    m_Indicator->m_Transform.scale = {0.05f, 20.0f};
    m_Indicator->SetZIndex(1.0f);

    m_PatternIdleImage = std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png");
    m_PatternPlayImage = std::make_shared<Util::Image>(RESOURCE_DIR "/player-note-dot-ON.png");
    m_Pattern = std::make_shared<Util::GameObject>();
    m_Pattern->SetDrawable(m_PatternIdleImage);
    m_Pattern->SetZIndex(80.0f);

    for (int i = 0; i < 3; ++i) {
        auto btn = std::make_shared<Util::GameObject>();
        try { btn->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 65, std::vector<std::string>{"繼續遊戲", "重新開始", "回到選單"}[i], SDL_Color{255, 255, 0, 255})); }
        catch (...) { btn->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png")); }
        btn->SetZIndex(90.0f); m_PauseButtons.push_back(btn);
    }

    m_ScoreTextObj = std::make_shared<Util::GameObject>();
    m_ScoreTextObj->SetZIndex(85.0f);
    m_ScoreTextObj->m_Transform.translation = {0.0f, 150.0f};

    m_ComboTextObj = std::make_shared<Util::GameObject>();
    m_ComboTextObj->SetZIndex(85.0f);
    m_ComboTextObj->m_Transform.translation = {0.0f, 80.0f};

    // 初始化右上角即時分數 HUD 物件
    m_RealScoreTextObj = std::make_shared<Util::GameObject>();
    m_RealScoreTextObj->SetZIndex(85.0f);
    m_RealScoreTextObj->m_Transform.translation = {500.0f, 310.0f};

    // 初始化結算畫面 UI 物件
    m_EndRankTextObj = std::make_shared<Util::GameObject>(); m_EndRankTextObj->SetZIndex(150.0f);
    m_EndScoreTextObj = std::make_shared<Util::GameObject>(); m_EndScoreTextObj->SetZIndex(150.0f);
    m_EndPerfectObj = std::make_shared<Util::GameObject>(); m_EndPerfectObj->SetZIndex(150.0f);
    m_EndNiceObj = std::make_shared<Util::GameObject>(); m_EndNiceObj->SetZIndex(150.0f);
    m_EndOkObj = std::make_shared<Util::GameObject>(); m_EndOkObj->SetZIndex(150.0f);
    m_EndMehObj = std::make_shared<Util::GameObject>(); m_EndMehObj->SetZIndex(150.0f);
    m_EndNastyObj = std::make_shared<Util::GameObject>(); m_EndNastyObj->SetZIndex(150.0f);
    m_EndCoinTextObj = std::make_shared<Util::GameObject>(); m_EndCoinTextObj->SetZIndex(150.0f);
    m_EndHintTextObj = std::make_shared<Util::GameObject>(); m_EndHintTextObj->SetZIndex(150.0f);

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
        try { return std::stof(json.substr(numStart, numEnd - numStart)); }
        catch (...) { return defaultVal; }
    };

    struct ParsedSong { std::string folder; std::string name; float bpm; float offset; };
    std::vector<ParsedSong> songData;

    if (fs::exists(songDir) && fs::is_directory(songDir)) {
        for (const auto& entry : fs::directory_iterator(songDir)) {
            if (entry.is_directory()) {
                std::string folder = entry.path().filename().string();
                fs::path tmbPath = entry.path() / "song.tmb";
                if (fs::exists(tmbPath)) {
                    std::string realName = folder;
                    float realBpm = 120.0f, realOffset = 0.0f;
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
        try { textObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 45, data.name, SDL_Color{255, 255, 255, 255})); }
        catch (...) { textObj->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png")); }
        textObj->SetZIndex(50.0f);
        m_SongButtons.push_back(textObj);
    }
    m_TotalMenuHeight = m_SongList.size() * 100.0f;
    m_CurrentState = State::SELECT;
}

void App::SelectUpdate() {
    Uint32 currentTicks = SDL_GetTicks();
    float dt = (currentTicks - m_LastFrameTime) / 1000.0f;
    if (dt > 0.1f) dt = 0.1f;
    m_LastFrameTime = currentTicks;

    m_Keyboard->Update();
    auto mousePos = Util::Input::GetCursorPosition();
    bool mouseClick = Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB);

    float scrollVelocity = 0.0f;
    if (Util::Input::IsKeyPressed(Util::Keycode::UP) || Util::Input::IsKeyPressed(Util::Keycode::W)) scrollVelocity = -300.0f;
    if (Util::Input::IsKeyPressed(Util::Keycode::DOWN) || Util::Input::IsKeyPressed(Util::Keycode::S)) scrollVelocity = 300.0f;

    m_MenuScrollY += scrollVelocity * dt;
    if (m_TotalMenuHeight > 0) {
        m_MenuScrollY = fmod(m_MenuScrollY, m_TotalMenuHeight);
        if (m_MenuScrollY < 0) m_MenuScrollY += m_TotalMenuHeight;
    }

    m_HoveredSongIndex = -1;
    for (size_t i = 0; i < m_SongButtons.size(); ++i) {
        auto& btn = m_SongButtons[i];
        float finalY = fmod(i * 100.0f - m_MenuScrollY + 400.0f, m_TotalMenuHeight);
        if (finalY < 0) finalY += m_TotalMenuHeight;
        finalY -= 400.0f;
        if (finalY < -500.0f || finalY > 500.0f) continue;

        btn->m_Transform.translation = {0.0f, -finalY};
        if (std::abs(mousePos.y - (-finalY)) < 40.0f) { m_HoveredSongIndex = (int)i; btn->m_Transform.scale = {1.2f, 1.2f}; }
        else btn->m_Transform.scale = {1.0f, 1.0f};
        btn->Draw();
    }

    if (m_HoveredSongIndex != -1 && mouseClick && !m_PrevMouseClick) {
        LoadSong(m_HoveredSongIndex);
        m_CurrentState = State::UPDATE;
        SDL_ShowCursor(SDL_DISABLE);
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
    Mix_HaltChannel(-1); Mix_HaltMusic();

    std::string finalBgPath = "";
    if (fs::exists(baseDir / "bg.png")) finalBgPath = (baseDir / "bg.png").string();
    else if (fs::exists(baseDir / "bg.jpg")) finalBgPath = (baseDir / "bg.jpg").string();
    else if (fs::exists(baseDir / "bg.jpeg")) finalBgPath = (baseDir / "bg.jpeg").string();

    std::shared_ptr<Util::Image> bgImg;
    if (!finalBgPath.empty()) bgImg = std::make_shared<Util::Image>(finalBgPath);
    else bgImg = std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png");

    m_Background->SetDrawable(bgImg);
    glm::vec2 bgSize = bgImg->GetSize();
    if (bgSize.x > 0 && bgSize.y > 0) {
        float scaleX = 1280.0f / bgSize.x;
        float scaleY = 720.0f / bgSize.y;
        float finalScale = std::max(scaleX, scaleY);
        m_Background->m_Transform.scale = {finalScale, finalScale};
    } else {
        m_Background->m_Transform.scale = {1.0f, 1.0f};
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

        struct RawNote {
            float bStart;
            float bDur;
            float startY;
            float endY;
        };
        std::vector<RawNote> rawNotes;

        size_t bS = 0;
        while ((bS = notesStr.find('[', bS)) != std::string::npos) {
            size_t bE = notesStr.find(']', bS);
            std::string sN = notesStr.substr(bS + 1, bE - bS - 1);
            for (char& c : sN) if (c == ',') c = ' ';
            std::stringstream ss(sN);
            float bStart, bDur, sY, dY, eY;
            if (ss >> bStart >> bDur >> sY >> dY >> eY) {
                float mappedStartY = (sY / 165.0f) * 300.0f;
                float mappedEndY   = (eY / 165.0f) * 300.0f;
                rawNotes.push_back({bStart, bDur, mappedStartY, mappedEndY});
            }
            bS = bE + 1;
        }

        std::sort(rawNotes.begin(), rawNotes.end(), [](const RawNote& a, const RawNote& b){
            return a.bStart < b.bStart;
        });

        if (!rawNotes.empty()) {
            std::vector<Note::Waypoint> currentWaypoints;
            currentWaypoints.push_back({rawNotes[0].bStart, rawNotes[0].startY});
            currentWaypoints.push_back({rawNotes[0].bStart + rawNotes[0].bDur, rawNotes[0].endY});
            float currentEndTime = rawNotes[0].bStart + rawNotes[0].bDur;

            for (size_t i = 1; i < rawNotes.size(); ++i) {
                if (std::abs(rawNotes[i].bStart - currentEndTime) < 0.01f) {
                    currentWaypoints.push_back({rawNotes[i].bStart + rawNotes[i].bDur, rawNotes[i].endY});
                    currentEndTime = rawNotes[i].bStart + rawNotes[i].bDur;
                } else {
                    m_Notes.push_back(std::make_shared<Note>(currentWaypoints));
                    currentWaypoints.clear();
                    currentWaypoints.push_back({rawNotes[i].bStart, rawNotes[i].startY});
                    currentWaypoints.push_back({rawNotes[i].bStart + rawNotes[i].bDur, rawNotes[i].endY});
                    currentEndTime = rawNotes[i].bStart + rawNotes[i].bDur;
                }
            }
            if (!currentWaypoints.empty()) {
                m_Notes.push_back(std::make_shared<Note>(currentWaypoints));
            }
        }
    }
    file.close();

    m_BGM->Play(-1);
    m_StartTime = SDL_GetTicks();
    m_CurrentMusicTime = 0;
    m_TotalPauseDuration = 0;
    m_IsRHolding = false;
    m_RequireRRelease = false;
    m_IsCountingDown = false;

    m_LastBeat = 0.0f;
    m_PerfectoCount = 0;
    m_NiceCount = 0;
    m_OkCount = 0;
    m_MehCount = 0;
    m_NastyCount = 0;
    m_Combo = 0;
    m_MaxCombo = 0;
    m_LastScoreString = "";
    m_LastCombo = -1;

    // 重置內部結算開關與分數
    m_InSettlement = false;
    m_Score = 0;
    int totalNotes = m_Notes.size();
    int simCombo = 0;
    m_MaxPossibleScore = 0;
    for (int i = 0; i < totalNotes; ++i) {
        simCombo++;
        m_MaxPossibleScore += 1000 * std::min(10, 1 + (simCombo / 10));
    }
    if (m_MaxPossibleScore <= 0) m_MaxPossibleScore = 1;

    try { m_RealScoreTextObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 36, "0", SDL_Color{255, 255, 255, 255})); } catch(...) {}
}
void App::Update() {
    // =================================================================
    // 👑 1. 專屬遊戲結算畫面（獨立顯示）
    // =================================================================
    if (m_InSettlement) {
        m_Keyboard->Update();

        // 🎨 繪製結算 UI 文字
        if (m_EndSongNameTextObj) m_EndSongNameTextObj->Draw(); // 👈 繪製歌曲名稱
        if (m_EndRankTextObj) m_EndRankTextObj->Draw();
        if (m_EndScoreTextObj) m_EndScoreTextObj->Draw();
        if (m_EndPerfectObj) m_EndPerfectObj->Draw();
        if (m_EndNiceObj) m_EndNiceObj->Draw();
        if (m_EndOkObj) m_EndOkObj->Draw();
        if (m_EndMehObj) m_EndMehObj->Draw();
        if (m_EndNastyObj) m_EndNastyObj->Draw();
        if (m_EndCoinTextObj) m_EndCoinTextObj->Draw();
        if (m_EndHintTextObj) m_EndHintTextObj->Draw();     // "PRESS [R] TO RESTART"
        if (m_EndHintTextObj2) m_EndHintTextObj2->Draw();   // "PRESS [ENTER] TO RETURN TO MENU"

        // 🕹️ 結算選單控制邏輯
        if (Util::Input::IsKeyPressed(Util::Keycode::R)) {
            m_InSettlement = false;
            LoadSong(m_CurrentSongIndex);
            g_RequireInputRelease = true;
            return;
        }

        if (Util::Input::IsKeyPressed(Util::Keycode::RETURN)) {
            Mix_HaltChannel(-1);
            Mix_HaltMusic();
            m_CurrentState = State::SELECT;
            SDL_ShowCursor(SDL_ENABLE);
            g_RequireInputRelease = true;
            m_InSettlement = false;
        }
        return;
    }

    m_Keyboard->Update();

    // =================================================================
    // 👑 2. 初始化結算畫面
    // =================================================================
    if (m_CurrentState == State::UPDATE && m_Notes.empty()) {
        m_InSettlement = true;
        SDL_ShowCursor(SDL_ENABLE);

        // 初始化所有結算文字物件
        if (!m_EndSongNameTextObj) m_EndSongNameTextObj = std::make_shared<Util::GameObject>(); // 👈 初始化
        if (!m_EndRankTextObj) m_EndRankTextObj = std::make_shared<Util::GameObject>();
        if (!m_EndScoreTextObj) m_EndScoreTextObj = std::make_shared<Util::GameObject>();
        if (!m_EndPerfectObj) m_EndPerfectObj = std::make_shared<Util::GameObject>();
        if (!m_EndNiceObj) m_EndNiceObj = std::make_shared<Util::GameObject>();
        if (!m_EndOkObj) m_EndOkObj = std::make_shared<Util::GameObject>();
        if (!m_EndMehObj) m_EndMehObj = std::make_shared<Util::GameObject>();
        if (!m_EndNastyObj) m_EndNastyObj = std::make_shared<Util::GameObject>();
        if (!m_EndCoinTextObj) m_EndCoinTextObj = std::make_shared<Util::GameObject>();
        if (!m_EndHintTextObj) m_EndHintTextObj = std::make_shared<Util::GameObject>();
        if (!m_EndHintTextObj2) m_EndHintTextObj2 = std::make_shared<Util::GameObject>();

        // 🔥 強制把結算 UI 的 Z-Index 頂到最高（100）
        float topZ = 100.0f;
        m_EndSongNameTextObj->SetZIndex(topZ); // 👈 設定 Z-Index
        m_EndRankTextObj->SetZIndex(topZ);     m_EndScoreTextObj->SetZIndex(topZ);
        m_EndPerfectObj->SetZIndex(topZ);      m_EndNiceObj->SetZIndex(topZ);
        m_EndOkObj->SetZIndex(topZ);           m_EndMehObj->SetZIndex(topZ);
        m_EndNastyObj->SetZIndex(topZ);        m_EndCoinTextObj->SetZIndex(topZ);
        m_EndHintTextObj->SetZIndex(topZ);     m_EndHintTextObj2->SetZIndex(topZ);

        // 計算 Rank 評級
        std::string rank = "F";
        float ratio = (m_MaxPossibleScore > 0) ? (float)m_Score / m_MaxPossibleScore : 0.0f;
        if (ratio >= 0.95f) rank = "S";
        else if (ratio >= 0.85f) rank = "A";
        else if (ratio >= 0.70f) rank = "B";
        else if (ratio >= 0.55f) rank = "C";
        else if (ratio >= 0.40f) rank = "D";

        int tootCoins = (m_Score / 20000) + m_PerfectoCount;
        const auto& song = m_SongList[m_CurrentSongIndex]; // 取得當前歌曲資訊

        // 載入結算文字內容
        // 🎵 歌曲名稱：字體大小設定為 150（比 RANK 的 120 還大！）
        try { m_EndSongNameTextObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 150, song.displayName, SDL_Color{255, 255, 255, 255})); } catch (...) {}
        try { m_EndRankTextObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 80, "RANK: " + rank, SDL_Color{255, 215, 0, 255})); } catch (...) {}
        try { m_EndScoreTextObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 50, "TOTAL SCORE: " + std::to_string(m_Score), SDL_Color{255, 255, 255, 255})); } catch (...) {}
        try { m_EndPerfectObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 30, "PERFECTO: " + std::to_string(m_PerfectoCount), SDL_Color{255, 235, 100, 255})); } catch (...) {}
        try { m_EndNiceObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 30, "NICE:     " + std::to_string(m_NiceCount), SDL_Color{100, 255, 100, 255})); } catch (...) {}
        try { m_EndOkObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 30, "OK:       " + std::to_string(m_OkCount), SDL_Color{100, 255, 255, 255})); } catch (...) {}
        try { m_EndMehObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 30, "MEH:      " + std::to_string(m_MehCount), SDL_Color{255, 180, 100, 255})); } catch (...) {}
        try { m_EndNastyObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 30, "NASTY:    " + std::to_string(m_NastyCount), SDL_Color{240, 100, 100, 255})); } catch (...) {}
        try { m_EndCoinTextObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 35, "TOOT COINS: " + std::to_string(tootCoins), SDL_Color{0, 255, 255, 255})); } catch (...) {}
        try { m_EndHintTextObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 26, "PRESS [R] TO RESTART SONG", SDL_Color{255, 255, 0, 255})); } catch (...) {}
        try { m_EndHintTextObj2->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 26, "PRESS [ENTER] TO RETURN TO MENU", SDL_Color{0, 255, 0, 255})); } catch (...) {}

        // 📐 排版佈局配置（X軸保持原樣，Y軸往下移動調整）
        m_EndSongNameTextObj->m_Transform.translation = {0.0f, 230.0f};   // 👈 歌曲名稱靠上水平置中

        m_EndRankTextObj->m_Transform.translation = {250.0f, 0.0f};       // 👈 右側 RANK 下移 (原 80.0f)
        m_EndCoinTextObj->m_Transform.translation = {250.0f, -120.0f};    // 👈 右側 哺幣下移 (原 -40.0f)

        m_EndScoreTextObj->m_Transform.translation = {-250.0f, 90.0f};    // 👈 左側 分數下移 (原 180.0f)
        float startY = 0.0f, stepY = -42.0f;                              // 👈 左側 判定起始點下移 (原 90.0f)
        m_EndPerfectObj->m_Transform.translation = {-250.0f, startY};
        m_EndNiceObj->m_Transform.translation = {-250.0f, startY + stepY};
        m_EndOkObj->m_Transform.translation = {-250.0f, startY + stepY * 2};
        m_EndMehObj->m_Transform.translation = {-250.0f, startY + stepY * 3};
        m_EndNastyObj->m_Transform.translation = {-250.0f, startY + stepY * 4};

        // 最底部的操作提示也稍微往下一點點留出空間
        m_EndHintTextObj->m_Transform.translation = {0.0f, -260.0f};
        m_EndHintTextObj2->m_Transform.translation = {0.0f, -310.0f};
        return;
    }

    // =================================================================
    // 3. 原本的遊戲遊玩邏輯 (保持不變)
    // =================================================================
    if (m_Keyboard->IsEscDown()) {
        m_CurrentState = State::PAUSE;
        m_PauseStartTime = SDL_GetTicks();
        Mix_PauseMusic(); Mix_HaltChannel(-1);
        m_WasBlowing = false; m_PrevMouseClick = true;
        SDL_ShowCursor(SDL_ENABLE);
        return;
    }

    if (m_Keyboard->IsRKeyPressed()) {
        if (!m_RequireRRelease && !m_IsRHolding) { m_IsRHolding = true; m_RHoldStartTime = SDL_GetTicks(); }
    } else { m_IsRHolding = false; m_RequireRRelease = false; }

    if (m_Keyboard->IsOffsetUp()) m_GlobalOffsetMs += 10.0f;
    if (m_Keyboard->IsOffsetDown()) m_GlobalOffsetMs -= 10.0f;

    m_CurrentMusicTime = SDL_GetTicks() - m_StartTime - m_TotalPauseDuration;
    const auto& song = m_SongList[m_CurrentSongIndex];
    float adjustedTime = static_cast<float>(m_CurrentMusicTime) + song.offsetMs + m_GlobalOffsetMs;
    float currentBeat = (adjustedTime / 60000.0f) * song.bpm;

    float deltaBeat = currentBeat - m_LastBeat;
    if (deltaBeat < 0.0f) deltaBeat = 0.0f;
    m_LastBeat = currentBeat;

    auto cursorPos = Util::Input::GetCursorPosition();
    float currentY = std::clamp(cursorPos.y, -300.0f, 300.0f);
    m_Pattern->m_Transform.translation = {-350.0f, currentY};

    bool blowing = m_Keyboard->IsBlowing();
    if (g_RequireInputRelease) { if (!blowing) g_RequireInputRelease = false; blowing = false; }

    int targetIdx = std::clamp((int)std::round((currentY + 300.0f) / 25.0f), 0, 24);
    bool justBlew = blowing && !m_WasBlowing;

    if (blowing) {
        if (!m_WasBlowing || targetIdx != m_CurrentNoteIndex) {
            Mix_HaltChannel(-1); m_TromboneNotes[targetIdx]->Play(-1); m_CurrentNoteIndex = targetIdx;
        }
        m_Pattern->SetDrawable(m_PatternPlayImage);
    } else {
        if (m_WasBlowing) Mix_HaltChannel(-1);
        m_Pattern->SetDrawable(m_PatternIdleImage);
    }
    m_WasBlowing = blowing;

    if (justBlew) {
        for (auto& note : m_Notes) {
            if (!note->IsActivated() &&
                currentBeat >= note->GetTargetTime() - 0.4f &&
                currentBeat <= note->GetTargetTime() + note->GetDuration()) {
                note->Activate();
                break;
            }
        }
    }

    bool uiNeedsUpdate = false;
    for (auto& note : m_Notes) {
        note->Update(currentBeat, deltaBeat, blowing, currentY);
    }

    for (auto it = m_Notes.begin(); it != m_Notes.end(); ) {
        if ((*it)->IsOut(currentBeat)) {
            std::string score = (*it)->GetScoreResult();
            int baseScore = 0;

            if (score == "PERFECTO") { m_PerfectoCount++; m_Combo++; baseScore = 1000; }
            else if (score == "NICE") { m_NiceCount++; m_Combo++; baseScore = 800; }
            else if (score == "OK") { m_OkCount++; m_Combo++; baseScore = 500; }
            else if (score == "MEH") { m_MehCount++; m_Combo++; baseScore = 200; }
            else if (score == "NASTY") { m_NastyCount++; m_Combo = 0; baseScore = 0; }

            if (score != "NASTY") {
                int multiplier = std::min(10, 1 + (m_Combo / 10));
                m_Score += baseScore * multiplier;
            }

            if (m_Combo > m_MaxCombo) m_MaxCombo = m_Combo;
            m_LastScoreString = score;
            uiNeedsUpdate = true;
            it = m_Notes.erase(it);
        } else {
            ++it;
        }
    }

    if (uiNeedsUpdate || m_LastCombo != m_Combo) {
        SDL_Color color = {255, 255, 255, 255};
        if (m_LastScoreString == "PERFECTO") color = {255, 215, 0, 255};
        else if (m_LastScoreString == "NICE") color = {0, 255, 0, 255};
        else if (m_LastScoreString == "OK") color = {0, 255, 255, 255};
        else if (m_LastScoreString == "MEH") color = {255, 165, 0, 255};
        else if (m_LastScoreString == "NASTY") color = {160, 82, 45, 255};

        try {
            if (m_LastScoreString != "") {
                m_ScoreTextObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 50, m_LastScoreString, color));
                m_ScoreTextObj->m_Transform.translation = {-450.0f, 300.0f};
            }
            if (m_Combo > 0) {
                m_ComboTextObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 40, "Combo: " + std::to_string(m_Combo), SDL_Color{255, 255, 255, 255}));
                m_ComboTextObj->m_Transform.translation = {-450.0f, 250.0f};
            }
            m_RealScoreTextObj->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 38, std::to_string(m_Score), SDL_Color{255, 255, 255, 255}));
        } catch (...) {}
        m_LastCombo = m_Combo;
    }

    m_Background->Draw();
    for (auto& line : m_GuideLines) line->Draw();
    m_Indicator->Draw();

    for (auto& note : m_Notes) {
        auto& objs = note->GetGameObjects();
        if (objs.empty()) continue;
        float headX = objs[0]->m_Transform.translation.x;
        float tailX = objs[1]->m_Transform.translation.x;
        if (tailX < -1000.0f || headX > 1000.0f) continue;
        for (auto& obj : objs) obj->Draw();
    }
    m_Pattern->Draw();

    if (m_LastScoreString != "") m_ScoreTextObj->Draw();
    if (m_Combo > 0) m_ComboTextObj->Draw();
    if (m_RealScoreTextObj) m_RealScoreTextObj->Draw();

    if (m_IsRHolding) {
        float progress = std::clamp((SDL_GetTicks() - m_RHoldStartTime) / 1000.0f, 0.0f, 1.0f);
        if (!s_RestartText) {
            s_RestartText = std::make_shared<Util::GameObject>(); s_RestartText->SetZIndex(99.0f);
            s_RestartFill = std::make_shared<Util::GameObject>(); s_RestartFill->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/player-note-dot-ON.png")); s_RestartFill->SetZIndex(98.0f);
            s_RestartBG = std::make_shared<Util::GameObject>(); s_RestartBG->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png")); s_RestartBG->SetZIndex(97.0f);
        }
        int percent = static_cast<int>(progress * 100);
        try { s_RestartText->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 40, "RESTARTING... " + std::to_string(percent) + "%", SDL_Color{255, 255, 0, 255})); } catch (...) {}

        s_RestartText->m_Transform.translation = {0.0f, -120.0f};
        s_RestartBG->m_Transform.translation = {0.0f, 0.0f}; s_RestartBG->m_Transform.scale = {2.0f, 2.0f};
        s_RestartFill->m_Transform.translation = {0.0f, 0.0f}; s_RestartFill->m_Transform.scale = {2.0f * progress, 2.0f * progress};

        s_RestartBG->Draw(); s_RestartFill->Draw(); s_RestartText->Draw();
        if (progress >= 1.0f) { LoadSong(m_CurrentSongIndex); g_RequireInputRelease = true; m_RequireRRelease = true; m_IsRHolding = false; return; }
    }
}
void App::PauseUpdate() {
    m_Keyboard->Update();
    auto mousePos = Util::Input::GetCursorPosition();
    bool click = Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB);

    m_Background->Draw();
    for (auto& line : m_GuideLines) line->Draw();
    m_Indicator->Draw();
    for (auto& note : m_Notes) {
        auto& objs = note->GetGameObjects();
        if (objs.empty()) continue;
        float headX = objs[0]->m_Transform.translation.x;
        float tailX = objs[1]->m_Transform.translation.x;
        if (tailX < -1000.0f || headX > 1000.0f) continue;
        for (auto& obj : objs) obj->Draw();
    }
    m_Pattern->Draw();

    if (m_LastScoreString != "") m_ScoreTextObj->Draw();
    if (m_Combo > 0) m_ComboTextObj->Draw();
    if (m_RealScoreTextObj) m_RealScoreTextObj->Draw();

    if (m_IsCountingDown) {
        Uint32 elapsed = SDL_GetTicks() - m_CountdownStartTime;
        int remain = 3 - (elapsed / 1000);
        if (remain > 0) {
            if (!s_CountdownText) {
                s_CountdownText = std::make_shared<Util::GameObject>(); s_CountdownText->SetZIndex(99.0f);
                s_CountdownBG = std::make_shared<Util::GameObject>(); s_CountdownBG->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png")); s_CountdownBG->SetZIndex(98.0f);
            }
            try { s_CountdownText->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 150, std::to_string(remain), SDL_Color{255, 255, 0, 255})); } catch(...) {}
            s_CountdownBG->m_Transform.translation = {0.0f, 0.0f}; s_CountdownBG->m_Transform.scale = {3.0f, 3.0f};
            s_CountdownBG->Draw(); s_CountdownText->Draw();
            return;
        } else {
            m_IsCountingDown = false;
            m_TotalPauseDuration += (SDL_GetTicks() - m_PauseStartTime);
            Mix_ResumeMusic(); m_CurrentState = State::UPDATE; SDL_ShowCursor(SDL_DISABLE);
            g_RequireInputRelease = true; m_LastFrameTime = SDL_GetTicks();
            return;
        }
    }

    m_PauseOverlay->Draw();

    if (m_Keyboard->IsEscDown()) {
        m_IsCountingDown = true; m_CountdownStartTime = SDL_GetTicks(); return;
    }

    if (m_Keyboard->IsRKeyPressed()) {
        if (!m_RequireRRelease && !m_IsRHolding) { m_IsRHolding = true; m_RHoldStartTime = SDL_GetTicks(); }
    } else { m_IsRHolding = false; m_RequireRRelease = false; }

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
            s_RestartText = std::make_shared<Util::GameObject>(); s_RestartText->SetZIndex(99.0f);
            s_RestartFill = std::make_shared<Util::GameObject>(); s_RestartFill->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/player-note-dot-ON.png")); s_RestartFill->SetZIndex(98.0f);
            s_RestartBG = std::make_shared<Util::GameObject>(); s_RestartBG->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png")); s_RestartBG->SetZIndex(97.0f);
        }
        int percent = static_cast<int>(progress * 100);
        try { s_RestartText->SetDrawable(std::make_shared<Util::Text>(RESOURCE_DIR "/font.ttc", 40, "RESTARTING... " + std::to_string(percent) + "%", SDL_Color{255, 255, 0, 255})); } catch(...) {}

        s_RestartText->m_Transform.translation = {0.0f, -120.0f};
        s_RestartBG->m_Transform.translation = {0.0f, 0.0f}; s_RestartBG->m_Transform.scale = {2.0f, 2.0f};
        s_RestartFill->m_Transform.translation = {0.0f, 0.0f}; s_RestartFill->m_Transform.scale = {2.0f * progress, 2.0f * progress};

        s_RestartBG->Draw(); s_RestartFill->Draw(); s_RestartText->Draw();
        if (progress >= 1.0f) { Mix_ResumeMusic(); LoadSong(m_CurrentSongIndex); m_CurrentState = State::UPDATE; SDL_ShowCursor(SDL_DISABLE); g_RequireInputRelease = true; m_RequireRRelease = true; m_IsRHolding = false; return; }
    }

    if (hovered != -1 && click && !m_PrevMouseClick) {
        if (hovered == 0) { m_IsCountingDown = true; m_CountdownStartTime = SDL_GetTicks(); }
        else if (hovered == 1) { Mix_ResumeMusic(); LoadSong(m_CurrentSongIndex); m_CurrentState = State::UPDATE; SDL_ShowCursor(SDL_DISABLE); g_RequireInputRelease = true; }
        else if (hovered == 2) { Mix_ResumeMusic(); Mix_HaltChannel(-1); Mix_HaltMusic(); m_CurrentState = State::SELECT; SDL_ShowCursor(SDL_ENABLE); g_RequireInputRelease = true; }
    }
    m_PrevMouseClick = click;
}

// 👑 3. 還原為原本安全的清理函數，供 main.cpp 程式關閉時安全調用
void App::End() {
    Mix_HaltChannel(-1); Mix_HaltMusic();
    s_RestartText.reset(); s_RestartFill.reset(); s_RestartBG.reset();
    s_CountdownText.reset(); s_CountdownBG.reset();
    m_ScoreTextObj.reset(); m_ComboTextObj.reset();
}