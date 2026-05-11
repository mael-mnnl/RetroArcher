#include "scoresystem.h"
#include "../model/gamemodel.h"
#include "../core/utils.h"
#include <QString>
#include <QColor>
#include <QDateTime>
#include <algorithm>

namespace Controller { namespace Score {

using namespace Model;

void popup(GameModel &m, float x, float y, const QString &text, const QColor &col) {
    Model::ScorePopup p;
    p.x = x; p.y = y;
    p.text = text; p.color = col;
    p.life = p.maxLife = 0.9f; p.scale = 1.0f;
    m.popups.push_back(p);
}

void onKill(GameModel &m)
{
    m.killStreakBigTimer = 4.0f;
    m.killStreakBig++;
    if (m.killStreakBig == 3) popup(m, m.player.x, m.player.y - 40, "x3 KILLS!", QColor("#ffcc44"));
    else if (m.killStreakBig == 5) popup(m, m.player.x, m.player.y - 40, "x5 RAMPAGE!", QColor("#ff8844"));
    else if (m.killStreakBig == 10) popup(m, m.player.x, m.player.y - 40, "x10 UNSTOPPABLE!", QColor("#ff4488"));
    else if (m.killStreakBig == 20) popup(m, m.player.x, m.player.y - 40, "GODLIKE!", QColor("#ff00ff"));
}

void update(GameModel &m, float dt)
{
    for (auto &p : m.popups) {
        p.life -= dt;
        p.y -= 30 * dt;
        float t = 1.f - p.life / p.maxLife;
        p.scale = 1.0f + t * 0.4f;
    }
    m.popups.erase(std::remove_if(m.popups.begin(), m.popups.end(),
                   [](const ScorePopup &p){ return p.life <= 0; }), m.popups.end());

    if (m.killStreakBigTimer > 0) {
        m.killStreakBigTimer -= dt;
        if (m.killStreakBigTimer <= 0) m.killStreakBig = 0;
    }
}

void addToLeaderboard(GameModel &m)
{
    LeaderEntry e;
    e.roomReached  = m.currentRoom;
    e.worldReached = std::min(6, ((m.currentRoom-1)/5)+1);
    e.classId      = m.selectedClassId;
    e.relicId      = m.selectedRelicId;
    e.skillsCount  = (int)m.player.skills.size();
    e.timestamp    = QDateTime::currentSecsSinceEpoch();
    m.leaderboard.push_back(e);
    // garder top 10 par roomReached
    std::sort(m.leaderboard.begin(), m.leaderboard.end(),
              [](const LeaderEntry &a, const LeaderEntry &b){ return a.roomReached > b.roomReached; });
    if (m.leaderboard.size() > 10) m.leaderboard.resize(10);
}

}}
