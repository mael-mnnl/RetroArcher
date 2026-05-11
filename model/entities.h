#ifndef MODEL_ENTITIES_H
#define MODEL_ENTITIES_H

#include "types.h"
#include <QColor>
#include <QString>
#include <QSet>
#include <vector>
#include <array>

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
    bool electric = false;
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
    float stunTimer = 0;   // étourdissement (immobile, ne tire pas)
    int   corrodStacks = 0; // stacks d'acide (jusqu'à 3, -15% def chacun)

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
    int   eliteVariant = 0;
    float auraPhase = 0;
    float dropChance = 0;
    int   malificusPattern = 0;
    float patternCd = 0;
};

// Pièce d'équipement (droppée par ennemis)
struct EquipItem {
    EquipType   type     = EQ_Weapon;
    EquipRarity rarity   = ER_Common;
    QString     name;
    QString     desc;
    QColor      color;
    // Modificateurs de stats
    float dmgMul      = 1.0f;
    float speedMul    = 1.0f;
    float fireRateMul = 1.0f;
    float spellDmgMul = 1.0f;
    int   shieldMax   = 0;    // points de shield (armure)
    float dodgeBonus  = 0.0f;
    int   goldBonus   = 0;    // bonus or par kill
    bool  empty       = true; // slot vide
};

// Informations sur un sort
struct SpellInfo {
    SpellId  id;
    QString  name;
    QString  desc;
    QColor   color;
    float    baseCd;     // cooldown de base en secondes
    int      baseCost;   // prix au marché noir
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

    // v8 fields
    int   classId = 0;
    int   relicId = -1;
    int   gold = 0;
    int   tempScrollSkillId = -1;
    int   tempScrollBossId  = -1;
    bool  challengeNoHit = true;
    bool  inChallenge = false;
    float dashAimX = 0, dashAimY = 0;
    bool  hasDashAim = false;

    // ── Sorts (4 slots, touches 1-4) ──
    std::array<int, 4>   spellSlots = {-1, -1, -1, -1};  // SpellId ou -1
    std::array<float, 4> spellCds   = {0, 0, 0, 0};

    // ── Bouclier (issu de l'armure) ──
    int   shieldHp    = 0;   // points actuels
    int   shieldMax   = 0;   // points max (0 = pas de bouclier)
    float shieldRegen = 0;   // timer de regen entre salles

    // ── Équipement (3 slots : arme, armure, accessoire) ──
    EquipItem equipped[EQ__COUNT];

    // ── Inventaire (sac) ──
    std::vector<EquipItem> bag;   // max 8 items

    // ── Compteurs pour nouveaux skills ──
    float overloadTimer  = 0;  // SK_OVERLOAD : temps sans dégâts
    bool  overloadReady  = false;
    int   reaperKills    = 0;  // SK_REAPER_STACKS
    float frenzyTimer    = 0;  // SK_FRENZY : timer des 10s
    int   frenzyKills    = 0;
    float frenzyBuff     = 0;  // SK_FRENZY : durée du buff actif
    float momentumDist   = 0;  // SK_MOMENTUM : distance parcourue
    float awakenBuff     = 0;  // SK_AWAKENING : timer buff salle
    float awakenMul      = 1.0f;
    int   roomKills      = 0;  // kills dans la salle courante
    bool  barrierRoomUsed= false; // SK_BARRIER_ROOM
    bool  warcryDone     = false; // SK_WARCRY
    float voidStepCd     = 0;  // SK_VOID_STEP
    float voidStepActive = 0;
    bool  soulStoneUsed  = false; // SK_SOUL_STONE
    float spellDmgMul    = 1.0f;  // multiplicateur sorts (équipement + skills)
    int   dashCharges    = 1;     // SK_DOUBLE_DASH
    int   dashChargesMax = 1;
    float dashChargeTimer= 0;
    int   shotCounter    = 0;     // SK_LUCKY_SHOT compteur
    float accelerateMul  = 1.0f;  // SK_ACCELERATE
    float accelerateTimer= 0;
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

struct Pickup {
    PickupType type = PU_Gold;
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float bobPhase = 0;
    float lifeSec = 20.f;
    int   value = 1;
    int   scrollSkillId = -1;
    EquipItem equipItem;   // rempli si type == PU_EquipDrop
};

struct ObstacleTile {
    int col, row;
    int kind;
    float anim = 0;
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
    int   startSkillId;
};

struct ScorePopup {
    QString text;
    QColor  color;
    float   x, y;
    float   life = 0.9f;
    float   maxLife = 0.9f;
    float   scale = 1.0f;
};

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

struct FxEffect {
    QString sheetKey;
    int  frameCount;
    int  frameW;
    int  frameH;
    float fps;
    float x, y;
    float scale;
    float timer = 0;
    bool  alive = true;
};

// Clone fantôme laissé par SK_SHADOW_DASH
struct ShadowClone {
    float x, y;
    float life = 2.0f;
};

// Flaque de lave (SK_MAGMA)
struct LavaTile {
    float x, y;
    float life = 4.0f;
};

}

#endif
