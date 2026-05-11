#ifndef MODEL_ENTITIES_H
#define MODEL_ENTITIES_H

#include "types.h"
#include <QColor>
#include <QString>
#include <QSet>
#include <vector>

namespace Model {

struct Particle {
    float x, y, vx, vy;
    QColor color;
    float life, maxLife;
    int size;
    bool noGravity = false;
};

struct Bullet {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float damage = 0;
    bool pierce = false;
    int pierceLeft = 0;
    int bounce = 0;
    bool enemy = false;
    bool dead = false;
    float angle = 0;
    bool grenade = false;
    float grenadeFuse = 0;
    float grenadeTotal = 0;
    bool splitChild = false;
    bool icy = false;
    bool poison = false;
    bool burn = false;
    bool homing = false;
    bool ghost = false;
    QSet<int> hitIds;
};

struct Enemy {
    int id = 0;
    float x = 0, y = 0;
    float hp = 0, maxHp = 0;
    float speed = 0;
    float damage = 1;
    float size = 28;
    EnemyType type = ET_Slime;
    bool dead = false;
    bool corpse = false;
    float corpseFadeTimer = 0;
    float hitFlash = 0;
    float animTimer = 0;
    AnimType anim = AN_Idle;
    bool facingLeft = false;

    float shootCd = 0;
    float aimAngle = 0;
    float zigDir = 1;
    float zigTimer = 0;

    int phase = 1;
    float burstCd = 0;
    float moveCd = 0;
    float moveTargetX = 0;
    float moveTargetY = 0;
    float specialCd = 0;
    int subType = 0;

    float slowTimer = 0;
    float poisonTimer = 0;
    float poisonDps = 0;
    float burnTimer = 0;
    float burnDps = 0;
    float deathMark = 0;

    // Boss patterns
    float chargeCd = 0;
    bool  charging = false;
    float chargeVx = 0, chargeVy = 0;
    float chargeT = 0;
    float summonCd = 0;
    float trailCd = 0;
    bool  fragmentDropped = false;
    int   dialogStage = 0;

    // Elite / Malificus
    bool  elite = false;
    int   eliteVariant = 0;   // sprite key (0..3)
    float auraPhase = 0;      // pour aura colorée pulsée
    float dropChance = 0;     // pour drop garanti
    int   malificusPattern = 0;  // pattern courant (rotation)
    float patternCd = 0;
};

struct Player {
    float x = 0, y = 0;
    float hp = 5, maxHp = 5;
    float speed = 155;
    float fireRate = 0.7f;
    float damage = 10;
    float fireCd = 0;
    float invincibility = 0;
    std::vector<int> skills;
    float size = 22;
    float facing = 0;
    bool moving = false;
    float animTimer = 0;
    AnimType anim = AN_Idle;
    bool facingLeft = false;
    float atkTimer = 0;

    int skinIndex = 0;
    int atkVariant = 0;
    int grenadeAmmo = 0;
    float grenadeCd = 0;

    // Active abilities + states
    float dashCd = 0;
    float dashActive = 0;
    float timeStopCd = 0;
    float timeStopActive = 0;
    bool  timeStopUsed = false;
    bool  phoenixUsed = false;
    float regenTimer = 0;
    float regenAfterKillCd = 0;
    int   killStreakCount = 0;
    float killStreakTimer = 0;
    int   rageStacks = 0;
    float rageStackTimer = 0;
    float adrenalineTimer = 0;
    float shieldReadyTimer = 0;
    float invulBurstTimer = 0;
    int   bossesKilled = 0;
    float stillSec = 0;
    int   frostNovaKills = 0;
    int   nextEnemyMarked = -1;
    float burstQueueTimer = 0;
    float chargeShotPower = 0;

    // ----- v8 new fields -----
    int   classId = 0;
    int   relicId = -1;
    int   gold = 0;
    int   tempScrollSkillId = -1;   // parchemin actif (1 boss)
    int   tempScrollBossId  = -1;
    bool  challengeNoHit = true;    // pour salle defi
    bool  inChallenge = false;
    float dashAimX = 0, dashAimY = 0;
    bool  hasDashAim = false;
};

struct Skill {
    int id;
    int worldTier;
    QString name;
    QString desc;
    QString icon;
    QColor color;
};

struct WorldInfo {
    QString name;
    QString flavor;
    QString bossKey;
    QString bossNameFr;
    int     bossFrameCount;
    int     bossFrameW;
    int     bossFrameH;
    float   bossFps;
    QColor  accent;
};

// ------- Nouveaux types pour v8 -------

struct Pickup {
    PickupType type = PU_Gold;
    float x = 0, y = 0;
    float vx = 0, vy = 0;     // pour le bounce initial
    float bobPhase = 0;
    float lifeSec = 20.f;     // disparait apres
    int   value = 1;          // gold/potion strength
    int   scrollSkillId = -1; // si Scroll
};

struct ObstacleTile {
    int col, row;
    int kind;                 // 0 = pillar, 1 = crate, 2 = brazier/torch
    float anim = 0;           // pour braseros animes
};

struct CurseInfo {
    int id;
    QString name;
    QString desc;
    QColor color;
};

struct RelicInfo {
    int id;
    QString name;
    QString desc;
    QColor color;
};

struct ClassInfo {
    int id;
    QString name;
    QString desc;
    QColor accent;
    float baseHp;
    float baseSpeed;
    float baseFireRate;
    float baseDamage;
    int   startGrenade;
    int   startSkillId;       // -1 = aucun
};

struct ScorePopup {
    QString text;
    QColor  color;
    float   x, y;
    float   life = 0.9f;
    float   maxLife = 0.9f;
    float   scale = 1.0f;
};

// Effet visuel d'arrière-plan/sol par monde
struct BiomeParticle {
    float x, y, vx, vy;
    QColor color;
    float life, maxLife;
    int size;
};

struct LeaderEntry {
    int   roomReached;
    int   worldReached;
    int   classId;
    int   relicId;
    int   skillsCount;
    qint64 timestamp;
};

// FX sprite-based (explosions, portails, etc.)
struct FxEffect {
    QString sheetKey;   // "fx_fire", "fx_portal", ...
    int  frameCount;
    int  frameW;
    int  frameH;
    float fps;
    float x, y;
    float scale;
    float timer = 0;
    bool  alive = true;
};
}

#endif
