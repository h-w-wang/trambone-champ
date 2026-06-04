#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp"
#include "Util/BGM.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/SFX.hpp"
#include "Util/Image.hpp"
#include "Util/GameObject.hpp"
#include "Keyboard.hpp"
#include "Note.hpp"
#include <SDL.h>
#include <memory>
#include <vector>
#include <string>

class App {
public:
    enum class State { START, SELECT, UPDATE, PAUSE, END };
    State GetCurrentState() const;
    void Start();
    void SelectUpdate();
    void Update();
    void PauseUpdate();
    void End();

private:
    State m_CurrentState = State::START;
    std::shared_ptr<Util::BGM> m_BGM;
    std::shared_ptr<Keyboard> m_Keyboard;

    struct SongInfo {
        std::string folderName;
        std::string displayName;
        float bpm;
        float offsetMs;
    };

    std::vector<SongInfo> m_SongList;
    int m_CurrentSongIndex = -1;

    void LoadSong(int index);

    std::shared_ptr<Util::GameObject> m_Background;
    std::vector<std::shared_ptr<Util::SFX>> m_TromboneNotes;
    std::shared_ptr<Util::GameObject> m_Indicator;
    std::vector<std::shared_ptr<Note>> m_Notes;
    std::vector<std::shared_ptr<Util::GameObject>> m_GuideLines;

    std::shared_ptr<Util::GameObject> m_Pattern;
    std::shared_ptr<Util::Image> m_PatternIdleImage;
    std::shared_ptr<Util::Image> m_PatternPlayImage;

    std::vector<std::shared_ptr<Util::GameObject>> m_SongButtons;
    std::vector<std::shared_ptr<Util::GameObject>> m_PauseButtons;
    std::shared_ptr<Util::GameObject> m_PauseOverlay;

    int m_HoveredSongIndex = -1;
    bool m_PrevMouseClick = false;
    float m_MenuScrollY = 0.0f;
    float m_TotalMenuHeight = 0.0f;

    bool m_WasBlowing = false;
    int m_CurrentNoteIndex = -1;
    Uint32 m_StartTime = 0;
    Uint32 m_CurrentMusicTime = 0;
    Uint32 m_LastPlayTime = 0;
    Uint32 m_TotalPauseDuration = 0;
    Uint32 m_PauseStartTime = 0;
    Uint32 m_RHoldStartTime = 0;
    Uint32 m_LastFrameTime = 0;

    // 🚀 新增：倒數計時控制變數
    bool m_IsCountingDown = false;
    Uint32 m_CountdownStartTime = 0;

    bool m_IsRHolding = false;
    bool m_RequireRRelease = false;

    float m_GlobalOffsetMs = 0.0f;

    // 在 App.hpp 補上這四行計數器
    float m_LastBeat = 0.0f;
    int m_PerfectCount = 0;
    int m_GoodCount = 0;
    int m_MissCount = 0;
};

#endif