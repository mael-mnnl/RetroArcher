#ifndef MODEL_GAMEMODEL_H
#define MODEL_GAMEMODEL_H

#include "types.h"
#include "entities.h"
#include <QString>
#include <QColor>
#include <QSet>
#include <QVector>
#include <vector>
#include <array>

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

    // v8
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

    // Class
    int  selectedClassId = 0;
    bool runHasFreeRevive = false;

    // Meta-progression
    int  blessingMaxHpBonus = 0;
    int  blessingStartGold  = 0;
    bool blessingFreeSkill  = false;
    int  blessingPointsSpent= 0;
    int  blessingPointsAvail= 0;

    // Ascension
    bool ascensionUnlocked = false;
    bool ascensionEnabled  = false;

    // Shop / Forge
    std::vector<int> shopItems;
    std::vector<int> shopPrices;
    int   forgeSelectedSkillIdx = -1;

    // Challenge
    bool challengeStarted = false;
    bool challengePassed  = true;

    // Combo / score
    int   killStreakBig = 0;
    float killStreakBigTimer = 0;
    float perfectRoomTimer = 0;
    int   lastBossHurtCount = 0;

    // Lord Malificus
    bool malificusUnlocked = false;
    int  bossesKilledThisRun = 0;

    // Transitions
    QColor fadeColor = QColor(0,0,0);

    // Leaderboard
    std::vector<LeaderEntry> leaderboard;

    // Data tables
    std::vector<CurseInfo>  allCurses;
    std::vector<RelicInfo>  allRelics;
    std::vector<ClassInfo>  allClasses;

    // ── Sorts ──
    std::vector<SpellInfo> allSpells;    // catalogue de tous les sorts

    // ── Marché noir ──
    std::vector<int>   blackMarketSpells;   // SpellId en vente (4 max)
    std::vector<int>   blackMarketPrices;

    // ── Menu pause ──
    int pauseTab        = 0;  // 0=compétences 1=inventaire 2=sorts
    int pauseSkillHover = 0;
    int pauseInvHover   = 0;  // hover dans le sac
    int pauseSpellHover = 0;
    GameState stateBeforePause = GS_Playing;

    // ── Entités de gameplay nouvelles ──
    std::vector<ShadowClone> shadowClones;
    std::vector<LavaTile>    lavaTiles;
};

}

#endif
