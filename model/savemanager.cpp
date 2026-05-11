#include "savemanager.h"
#include "gamemodel.h"
#include <QSettings>
#include <QStringList>
#include <algorithm>

namespace Model {

void loadSettings(GameModel &m)
{
    QSettings s("RetroArcher", "RetroArcher");
    m.highScore         = s.value("highScore", 0).toInt();
    m.fragments         = std::max(0, std::min(6, s.value("fragments", 0).toInt()));
    m.worldsUnlocked    = std::max(1, std::min(5, m.fragments + 1));
    m.menuSelectedSkin  = s.value("skin", 0).toInt() % 4;
    m.menuSelectedAtk   = s.value("atk",  0).toInt() % 3;
    // v8 :
    m.malificusUnlocked   = s.value("malificusUnlocked", false).toBool();
    m.ascensionUnlocked   = s.value("ascensionUnlocked", false).toBool();
    m.blessingMaxHpBonus  = s.value("blessingMaxHpBonus", 0).toInt();
    m.blessingStartGold   = s.value("blessingStartGold",  0).toInt();
    m.blessingFreeSkill   = s.value("blessingFreeSkill",  false).toBool();
    m.blessingPointsSpent = s.value("blessingPointsSpent",0).toInt();
    m.blessingPointsAvail = std::max(0, m.fragments - m.blessingPointsSpent);

    // Leaderboard
    m.leaderboard.clear();
    int n = s.beginReadArray("leaderboard");
    for (int i = 0; i < n; ++i) {
        s.setArrayIndex(i);
        LeaderEntry e;
        e.roomReached  = s.value("room",  0).toInt();
        e.worldReached = s.value("world", 0).toInt();
        e.classId      = s.value("class", 0).toInt();
        e.relicId      = s.value("relic",-1).toInt();
        e.skillsCount  = s.value("skills",0).toInt();
        e.timestamp    = s.value("ts",    qint64(0)).toLongLong();
        m.leaderboard.push_back(e);
    }
    s.endArray();
}

void saveSettings(const GameModel &m)
{
    QSettings s("RetroArcher", "RetroArcher");
    s.setValue("highScore",         m.highScore);
    s.setValue("fragments",         m.fragments);
    s.setValue("worldsUnlocked",    m.worldsUnlocked);
    s.setValue("malificusUnlocked", m.malificusUnlocked);
    s.setValue("ascensionUnlocked", m.ascensionUnlocked);
    s.setValue("blessingMaxHpBonus",m.blessingMaxHpBonus);
    s.setValue("blessingStartGold", m.blessingStartGold);
    s.setValue("blessingFreeSkill", m.blessingFreeSkill);
    s.setValue("blessingPointsSpent",m.blessingPointsSpent);

    s.beginWriteArray("leaderboard");
    for (int i = 0; i < (int)m.leaderboard.size(); ++i) {
        s.setArrayIndex(i);
        const LeaderEntry &e = m.leaderboard[i];
        s.setValue("room",   e.roomReached);
        s.setValue("world",  e.worldReached);
        s.setValue("class",  e.classId);
        s.setValue("relic",  e.relicId);
        s.setValue("skills", e.skillsCount);
        s.setValue("ts",     e.timestamp);
    }
    s.endArray();
}

}
