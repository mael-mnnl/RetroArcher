#ifndef MODEL_GAMEMODEL_H
#define MODEL_GAMEMODEL_H

#include "types.h"
#include "entities.h"
#include <QString>
#include <QColor>
#include <QSet>
#include <QVector>
#include <vector>

namespace Model {

struct GameModel {
    GameState state = GS_Menu;
    int currentRoom = 1;
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<Particle> particles;
    int bossIndex = -1;
    int nextEnemyId = 1;

    int  layoutIdx = 0;
    bool doorOpen[4] = {false, false, false, false};
    int  exitDoor = 0;
    float fadeAmount = 0;
    int  discoveryHover = 0;

    QSet<int> keys;
    float globalTime = 0;

    std::vector<Skill> allSkills;
    std::vector<int>   skillChoices;
    QVector<WorldInfo> worlds;

    int highScore       = 0;
    int worldsUnlocked  = 1;
    int fragments       = 0;
    int menuSelectedSkin = 0;
    int menuSelectedAtk  = 0;

    // Cheats
    bool    cheatInvincible = false;
    QString cheatInput;
    float   cheatFlashTimer = 0;
    bool    cheatFocused = false;

    // Tutorial
    int   tutStep = 0;
    bool  tutorialActive = false;
    float tutAdvanceCd = 0;

    // Boss dialog
    QString dlgSpeaker;
    QString dlgText;
    QColor  dlgColor;
    float   dlgTimer = 0;
    float   dlgFadeIn = 0;
};

}

#endif
