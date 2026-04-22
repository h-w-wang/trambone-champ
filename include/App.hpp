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
    enum class State { START, UPDATE, END };
    State GetCurrentState() const;
    void Start();
    void Update();
    void End();

private:
    State m_CurrentState = State::START;
    std::shared_ptr<Util::BGM> m_BGM;
    std::shared_ptr<Keyboard> m_Keyboard;

    // --- 歌曲資料結構與清單 ---
    struct SongInfo {
        std::string folderName;
        std::string displayName;
        float bpm;       // 這首歌的節拍速度
        float offsetMs;  // 延遲補償（正數代表音符提早，負數代表音符延後）
    };

    std::vector<SongInfo> m_SongList;
    int m_CurrentSongIndex = -1;

    void LoadSong(int index);

    // --- 視覺與音效物件 ---
    std::shared_ptr<Util::GameObject> m_Background;
    std::vector<std::shared_ptr<Util::SFX>> m_TromboneNotes;
    std::shared_ptr<Util::GameObject> m_Indicator;
    std::vector<std::shared_ptr<Note>> m_Notes;

    // --- 玩家控制的 Pattern 相關 ---
    std::shared_ptr<Util::GameObject> m_Pattern;
    std::shared_ptr<Util::Image> m_PatternIdleImage;
    std::shared_ptr<Util::Image> m_PatternPlayImage;

    bool m_WasBlowing = false;
    int m_CurrentNoteIndex = -1;

    // --- 時間與狀態管理 ---
    Uint32 m_StartTime = 0;
    Uint32 m_CurrentMusicTime = 0;
    Uint32 m_LastPlayTime = 0;

    // --- 視覺輔助 ---
    std::vector<std::shared_ptr<Util::GameObject>> m_GuideLines;
};

#endif