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
    enum class State { START, SELECT, UPDATE, END }; // 🚀 新增 SELECT 狀態
    State GetCurrentState() const;
    void Start();
    void SelectUpdate(); // 🚀 新增選歌畫面的更新函式
    void Update();
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

    std::shared_ptr<Util::GameObject> m_Pattern;
    std::shared_ptr<Util::Image> m_PatternIdleImage;
    std::shared_ptr<Util::Image> m_PatternPlayImage;

    bool m_WasBlowing = false;
    int m_CurrentNoteIndex = -1;

    Uint32 m_StartTime = 0;
    Uint32 m_CurrentMusicTime = 0;
    Uint32 m_LastPlayTime = 0;

    std::vector<std::shared_ptr<Util::GameObject>> m_GuideLines;
};

#endif