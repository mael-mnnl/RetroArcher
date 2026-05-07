#include "savemanager.h"
#include "gamemodel.h"
#include <QSettings>
#include <algorithm>

namespace Model {

void loadSettings(GameModel &m)
{
    QSettings s("RetroArcher", "RetroArcher");
    m.highScore        = s.value("highScore", 0).toInt();
    m.fragments        = std::max(0, std::min(5, s.value("fragments", 0).toInt()));
    m.worldsUnlocked   = std::max(1, std::min(5, m.fragments + 1));
    m.menuSelectedSkin = s.value("skin", 0).toInt() % 4;
    m.menuSelectedAtk  = s.value("atk", 0).toInt() % 3;
}

void saveSettings(const GameModel &m)
{
    QSettings s("RetroArcher", "RetroArcher");
    s.setValue("highScore",       m.highScore);
    s.setValue("fragments",       m.fragments);
    s.setValue("worldsUnlocked",  m.worldsUnlocked);
}

}
