#include "Note.hpp"
#include <cmath>
#include <algorithm>

static float IndexToMusicStep(float idx) {
    static const std::vector<std::pair<float, float>> whiteKeyMapping = {
        {0.0f, 0.0f}, {2.0f, 1.0f}, {4.0f, 2.0f}, {5.0f, 3.0f}, {7.0f, 4.0f}, {9.0f, 5.0f}, {11.0f, 6.0f},
        {12.0f, 7.0f}, {14.0f, 8.0f}, {16.0f, 9.0f}, {17.0f, 10.0f}, {19.0f, 11.0f}, {21.0f, 12.0f}, {23.0f, 13.0f}, {24.0f, 14.0f}
    };
    if (idx <= whiteKeyMapping.front().first) return whiteKeyMapping.front().second;
    if (idx >= whiteKeyMapping.back().first) return whiteKeyMapping.back().second;
    for (size_t i = 0; i < whiteKeyMapping.size() - 1; ++i) {
        if (idx >= whiteKeyMapping[i].first && idx <= whiteKeyMapping[i+1].first) {
            float t = (idx - whiteKeyMapping[i].first) / (whiteKeyMapping[i+1].first - whiteKeyMapping[i].first);
            return whiteKeyMapping[i].second + t * (whiteKeyMapping[i+1].second - whiteKeyMapping[i].second);
        }
    }
    return 0.0f;
}

//計算四個控制點之間的平滑曲線位置
static Note::Waypoint InterpolateCatmullRom(const Note::Waypoint& p0, const Note::Waypoint& p1, const Note::Waypoint& p2, const Note::Waypoint& p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    float b1 = 0.5f * (-t3 + 2.0f * t2 - t);
    float b2 = 0.5f * (3.0f * t3 - 5.0f * t2 + 2.0f);
    float b3 = 0.5f * (-3.0f * t3 + 4.0f * t2 + t);
    float b4 = 0.5f * (t3 - t2);

    float beat = p0.beat * b1 + p1.beat * b2 + p2.beat * b3 + p3.beat * b4;
    float y = p0.y * b1 + p1.y * b2 + p2.y * b3 + p3.y * b4;
    return {beat, y};
}

static std::weak_ptr<Util::Image> s_NoteDotImage;
static std::weak_ptr<Util::Image> s_NoteLineImage;

Note::Note(const std::vector<Waypoint>& waypoints) : m_Waypoints(waypoints) {
    size_t n = m_Waypoints.size();

    //將音符折線修成圓滑曲線
    if (n >= 2) {
        for (size_t i = 0; i < n - 1; ++i) {
            Waypoint p0 = (i == 0) ? Waypoint{ m_Waypoints[0].beat - 1.0f, m_Waypoints[0].y } : m_Waypoints[i - 1];
            Waypoint p1 = m_Waypoints[i];
            Waypoint p2 = m_Waypoints[i + 1];
            Waypoint p3 = (i + 2 >= n) ? Waypoint{ m_Waypoints[n - 1].beat + 1.0f, m_Waypoints[n - 1].y } : m_Waypoints[i + 2];

            int subdivisions = 16;
            for (int s = 0; s < subdivisions; ++s) {
                float t = (float)s / subdivisions;
                m_RenderPoints.push_back(InterpolateCatmullRom(p0, p1, p2, p3, t));
            }
        }
        m_RenderPoints.push_back(m_Waypoints.back());
    }

    auto dotImg = s_NoteDotImage.lock();
    if (!dotImg) { dotImg = std::make_shared<Util::Image>(RESOURCE_DIR "/note-dot.png"); s_NoteDotImage = dotImg; }
    auto lineImg = s_NoteLineImage.lock();
    if (!lineImg) { lineImg = std::make_shared<Util::Image>(RESOURCE_DIR "/warmup-light-rotated.png"); s_NoteLineImage = lineImg; }


    auto headDot = std::make_shared<Util::GameObject>();
    headDot->SetDrawable(dotImg); headDot->SetZIndex(5.1f);
    m_GameObjects.push_back(headDot); // m_GameObjects[0]

    auto tailDot = std::make_shared<Util::GameObject>();
    tailDot->SetDrawable(dotImg); tailDot->SetZIndex(5.1f);
    m_GameObjects.push_back(tailDot); // m_GameObjects[1]

    // 建立用於組成平滑曲線的連續小細線
    if (m_RenderPoints.size() >= 2) {
        for (size_t i = 0; i < m_RenderPoints.size() - 1; ++i) {
            auto line = std::make_shared<Util::GameObject>();
            line->SetDrawable(lineImg); line->SetZIndex(5.0f);
            m_GameObjects.push_back(line);
        }
    }
}

float Note::GetTargetTime() const {
    return m_Waypoints.empty() ? 0.0f : m_Waypoints.front().beat;
}

float Note::GetDuration() const {
    return m_Waypoints.size() < 2 ? 0.0f : (m_Waypoints.back().beat - m_Waypoints.front().beat);
}

void Note::Update(float currentBeat, float deltaBeat, bool isBlowing, float currentY) {
    float speed = 150.0f;
    if (m_RenderPoints.size() < 2) return;

    //更新頭端與尾端兩顆唯一存在的白色圓點的位置
    float headX = -350.0f + (m_RenderPoints.front().beat - currentBeat) * speed;
    m_GameObjects[0]->m_Transform.translation = {headX, m_RenderPoints.front().y};
    m_GameObjects[0]->m_Transform.scale = {0.5f, 0.5f};

    float tailX = -350.0f + (m_RenderPoints.back().beat - currentBeat) * speed;
    m_GameObjects[1]->m_Transform.translation = {tailX, m_RenderPoints.back().y};
    m_GameObjects[1]->m_Transform.scale = {0.5f, 0.5f};

    //更新所有細化平滑後的微小長條線段
    size_t numSegments = m_RenderPoints.size() - 1;
    for (size_t i = 0; i < numSegments; ++i) {
        auto& line = m_GameObjects[2 + i]; // 跳過前兩個頭尾 Dot
        float x1 = -350.0f + (m_RenderPoints[i].beat - currentBeat) * speed;
        float x2 = -350.0f + (m_RenderPoints[i+1].beat - currentBeat) * speed;
        float dx = x2 - x1;
        float dy = m_RenderPoints[i+1].y - m_RenderPoints[i].y;

        line->m_Transform.translation = {(x1 + x2) / 2.0f, (m_RenderPoints[i].y + m_RenderPoints[i+1].y) / 2.0f};
        line->m_Transform.rotation = std::atan2(dy, dx);
        line->m_Transform.scale = {std::sqrt(dx * dx + dy * dy) / 256.0f, 0.5f};
    }

    //長條即時音高追蹤與判定
    float targetTime = GetTargetTime();
    float duration = GetDuration();
    if (currentBeat >= targetTime && currentBeat <= targetTime + duration) {
        int idx = -1;
        for (size_t i = 0; i < numSegments; ++i) {
            if (currentBeat >= m_RenderPoints[i].beat && currentBeat <= m_RenderPoints[i+1].beat) {
                idx = static_cast<int>(i);
                break;
            }
        }

        if (idx != -1) {
            float segStartBeat = m_RenderPoints[idx].beat;
            float segEndBeat = m_RenderPoints[idx+1].beat;
            float segLength = segEndBeat - segStartBeat;

            float expectedY = m_RenderPoints[idx].y;
            if (segLength > 0.0f) {
                float t = (currentBeat - segStartBeat) / segLength;
                expectedY = m_RenderPoints[idx].y + t * (m_RenderPoints[idx+1].y - m_RenderPoints[idx].y);
            }

            int expectedPitch = std::clamp(static_cast<int>(std::round((expectedY + 300.0f) / 25.0f)), 0, 24);
            int currentPitchIdx = std::clamp(static_cast<int>(std::round((currentY + 300.0f) / 25.0f)), 0, 24);
            float stepDiff = std::abs(IndexToMusicStep((float)currentPitchIdx) - IndexToMusicStep((float)expectedPitch));

            if (isBlowing && m_IsActivated && stepDiff <= 4.5f) {
                m_HitBeatAmount += deltaBeat;
                float acc = 0.0f;
                if (stepDiff <= 0.5f) acc = 1.00f;
                else if (stepDiff <= 1.5f) acc = 0.90f;
                else if (stepDiff <= 2.5f) acc = 0.80f;
                else if (stepDiff <= 3.5f) acc = 0.70f;
                else if (stepDiff <= 4.5f) acc = 0.61f;

                if (acc > m_MaxAccuracy) m_MaxAccuracy = acc;
            }
        }
    }
}

bool Note::IsOut(float currentBeat) const {
    if (m_Waypoints.empty()) return true;
    return currentBeat > (m_Waypoints.back().beat + 0.5f); // 滾出畫面左側後自動釋放
}

std::string Note::GetScoreResult() const {
    float duration = GetDuration();
    if (duration <= 0) return "NASTY";
    float hitPercent = m_HitBeatAmount / duration;
    if (hitPercent < 0.1f) return "NASTY";

    if (m_MaxAccuracy >= 0.88f && hitPercent >= 0.88f) return "PERFECTO";
    if (m_MaxAccuracy >= 0.79f && hitPercent >= 0.79f) return "NICE";
    if (m_MaxAccuracy >= 0.70f && hitPercent >= 0.72f) return "OK";
    if (m_MaxAccuracy >= 0.61f && hitPercent >= 0.61f) return "MEH";
    return "NASTY";
}