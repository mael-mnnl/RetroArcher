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

    // ----- v8 -----
    RoomType roomType = RT_Normal;
    std::vector<ObstacleTile> obstacles;
    std::vector<Pickup>       pickups;
    std::vector<ScorePopup>   popups;
    std::vector<BiomeParticle> biomeFx;
    std::vector<FxEffect>     fxEffects;

    // Curses
    std::vector<int> activeCurses;
    std::vector<int> curseChoices;

    // Relics
    int selectedRelicId = -1;
    std::vector<int> relicChoices;

    // Class selection
    int  selectedClassId = 0;
    bool runHasFreeRevive = false;   // pour Phoenix Tear

    // Meta-progression "blessings"
    int  blessingMaxHpBonus = 0;     // +1 PV max au depart
    int  blessingStartGold  = 0;     // +50 gold au depart
    bool blessingFreeSkill  = false; // +1 skill choice au debut
    int  blessingPointsSpent= 0;
    int  blessingPointsAvail= 0;

    // Ascension (mode hardcore)
    bool ascensionUnlocked = false;
    bool ascensionEnabled  = false;

    // Shop / Forge state
    std::vector<int> shopItems;      // skill IDs en vente
    std::vector<int> shopPrices;
    int   forgeSelectedSkillIdx = -1;

    // Challenge state
    bool challengeStarted = false;
    bool challengePassed  = true;

    // Combo / score popups
    int   killStreakBig = 0;         // streak pour affichage popup
    float killStreakBigTimer = 0;
    float perfectRoomTimer = 0;
    int   lastBossHurtCount = 0;     // pour traquer no-hit boss

    // Lord Malificus (monde 6)
    bool malificusUnlocked = false;
    int  bossesKilledThisRun = 0;

    // Transitions
    QColor fadeColor = QColor(0,0,0);

    // Leaderboard
    std::vector<LeaderEntry> leaderboard;

    // Tables look-up
    std::vector<CurseInfo> allCurses;
    std::vector<RelicInfo> allRelics;
    std::vector<ClassInfo> allClasses;
};

}

#endif
