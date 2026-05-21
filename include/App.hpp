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

    int m_HoveredSongIndex = -1;
    bool m_PrevMouseClick = false;

    float m_MenuScrollY = 0.0f;
    float m_TargetScrollY = 0.0f;
    Uint32 m_LastFrameTime = 0;
    float m_TotalMenuHeight = 0.0f;

    bool m_WasBlowing = false;
    int m_CurrentNoteIndex = -1;
    Uint32 m_StartTime = 0;
    Uint32 m_CurrentMusicTime = 0;
    Uint32 m_LastPlayTime = 0;

    Uint32 m_TotalPauseDuration = 0;
    Uint32 m_PauseStartTime = 0;
    Uint32 m_RHoldStartTime = 0;

    bool m_IsRHolding = false;
    bool m_RequireRRelease = false; // 🚀 新增：防止 100% 後繼續觸發的按鍵鎖

    float m_GlobalOffsetMs = 0.0f;
};

#endif