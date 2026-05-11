#include "roombuilder.h"
#include "collisionsystem.h"
#include "skillsystem.h"
#include "dialogsystem.h"
#include "levelgen.h"
#include "relicsystem.h"
#include "../model/gamemodel.h"
#include "../core/constants.h"
#include "../core/utils.h"
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

namespace Controller { namespace Room {

using namespace Model;
using namespace Core;

int  worldOf(int room) { return std::min(6, ((room-1)/5) + 1); }
bool isBossRoom(int room) { return room % 5 == 0; }
bool isFinalBossRoom(int room) { return room == 25 || room == 30; }

static float ascensionDmgMul(const GameModel &m)  { return m.ascensionEnabled ? 1.30f : 1.f; }
static float ascensionSpeedMul(const GameModel &m){ return m.ascensionEnabled ? 1.15f : 1.f; }

void spawnMinion(GameModel &m, float x, float y, int hp, float speed, EnemyType type)
{
    Enemy e;
    e.id = m.nextEnemyId++;
    e.x = x; e.y = y;
    e.hp = e.maxHp = hp * Relic::enemyHpMul(m);
    e.speed = speed;
    e.size = 26;
    e.type = type;
    e.damage = 1;
    e.shootCd = (type == ET_Skel) ? rndF(2.f, 3.5f) : 0;
    e.zigDir = (rndI(0,1)==0) ? 1.f : -1.f;
    e.anim = AN_Walk;
    m.enemies.push_back(e);
}

void startGame(GameModel &m)
{
    m.tutorialActive = false;
    m.player = Player();
    m.player.skinIndex  = m.menuSelectedSkin;
    m.player.atkVariant = m.menuSelectedAtk;
    m.player.classId    = m.selectedClassId;
    m.player.relicId    = m.selectedRelicId;
    m.player.gold       = m.blessingStartGold;

    // Apply class stats
    if (m.selectedClassId >= 0 && m.selectedClassId < (int)m.allClasses.size()) {
        const ClassInfo &c = m.allClasses[m.selectedClassId];
        m.player.maxHp     = c.baseHp;
        m.player.hp        = c.baseHp;
        m.player.speed     = c.baseSpeed;
        m.player.fireRate  = c.baseFireRate;
        m.player.damage    = c.baseDamage;
        m.player.grenadeAmmo = c.startGrenade;
        if (c.startSkillId >= 0) m.player.skills.push_back(c.startSkillId);
    }
    // Blessing bonuses
    m.player.maxHp += m.blessingMaxHpBonus;
    m.player.hp = m.player.maxHp;

    // Relics (re-apply after class to be sure)
    if (m.selectedRelicId == REL_GoldenSeed) {
        m.player.maxHp = std::max(1.f, m.player.maxHp - 1);
        m.player.hp = std::min(m.player.maxHp, m.player.hp);
    }
    if (m.selectedRelicId == REL_PhoenixTear) m.runHasFreeRevive = true;

    m.player.x = GW/2.f;
    m.player.y = GH/2.f;
    m.enemies.clear(); m.bullets.clear(); m.particles.clear();
    m.pickups.clear(); m.popups.clear(); m.biomeFx.clear();
    m.fxEffects.clear();
    m.obstacles.clear();
    m.activeCurses.clear();
    m.bossesKilledThisRun = 0;
    m.currentRoom = 1;
    m.bossIndex   = -1;
    m.state       = GS_Playing;
    m.fadeAmount  = 0;
    buildRoom(m, 1, -1);
}

void buildRoom(GameModel &m, int roomIndex, int entryDoor)
{
    m.currentRoom = roomIndex;
    m.enemies.clear(); m.bullets.clear();
    m.pickups.clear();
    m.bossIndex = -1;
    for (int i = 0; i < 4; ++i) m.doorOpen[i] = false;

    // Curse : -1 PV par salle
    if (std::find(m.activeCurses.begin(), m.activeCurses.end(), CRS_BloodTax) != m.activeCurses.end()) {
        if (roomIndex > 1) {
            m.player.hp = std::max(1.f, m.player.hp - 1);
        }
    }

    // Type de la salle
    m.roomType = (RoomType)LevelGen::pickRoomType(m, roomIndex);
    LevelGen::generateObstacles(m, roomIndex);

    if      (entryDoor==0) { m.player.x = DOOR_COL*TILE+TILE/2.f; m.player.y = TILE*2.5f; }
    else if (entryDoor==1) { m.player.x = GW-TILE*2.5f; m.player.y = DOOR_ROW*TILE+TILE/2.f; }
    else if (entryDoor==2) { m.player.x = DOOR_COL*TILE+TILE/2.f; m.player.y = GH-TILE*2.5f; }
    else if (entryDoor==3) { m.player.x = TILE*2.5f; m.player.y = DOOR_ROW*TILE+TILE/2.f; }
    else                   { m.player.x = GW/2.f; m.player.y = GH/2.f; }

    m.layoutIdx = 0;
    Collision::resolveCollision(m, m.player.x, m.player.y, 8.f);

    int worldIdx = worldOf(roomIndex) - 1;
    m.player.inChallenge = (m.roomType == RT_Challenge);
    m.challengeStarted = false;
    m.challengePassed  = true;

    // Salles spéciales : pas d'ennemis
    if (m.roomType == RT_Shop || m.roomType == RT_Forge) {
        // Ouvre toutes les portes deja
        for (int i = 0; i < 4; ++i) m.doorOpen[i] = true;
        m.state = GS_RoomCleared;
        // Prépare shopItems si shop
        if (m.roomType == RT_Shop) {
            m.shopItems.clear(); m.shopPrices.clear();
            std::vector<int> pool;
            for (auto &sk : m.allSkills)
                if (sk.worldTier <= m.worldsUnlocked) pool.push_back(sk.id);
            for (int i = (int)pool.size()-1; i > 0; --i) { int j = rndI(0, i); std::swap(pool[i], pool[j]); }
            for (int i = 0; i < std::min(3, (int)pool.size()); ++i) {
                m.shopItems.push_back(pool[i]);
                m.shopPrices.push_back(20 + rndI(0, 30));
            }
        }
        return;
    }
    if (m.roomType == RT_Challenge) {
        // Quelques ennemis difficiles mais clear == bonus
        for (int i = 0; i < 6; ++i) {
            float ex, ey; int safety = 40;
            do {
                ex = rndF(TILE*2.5f, GW-TILE*2.5f); ey = rndF(TILE*2.5f, GH-TILE*2.5f);
                safety--;
            } while ((distF(ex, ey, m.player.x, m.player.y) < 130
                     || Collision::collidesObstacle(m, ex, ey, 18)) && safety > 0);
            Enemy e; e.id = m.nextEnemyId++; e.x = ex; e.y = ey;
            e.type = ET_Bat; e.hp = e.maxHp = 35; e.speed = rndF(70,95); e.size = 28;
            e.zigDir = (rndI(0,1)==0)?1.f:-1.f; e.anim = AN_Walk;
            m.enemies.push_back(e);
        }
        return;
    }

    if (isBossRoom(roomIndex)) {
        Enemy boss;
        boss.id = m.nextEnemyId++;
        boss.x = GW/2.f; boss.y = GH/2.f - 30;
        boss.subType = worldIdx;
        boss.phase = 1;
        boss.moveCd = rndF(0.8f, 1.6f);
        boss.moveTargetX = boss.x; boss.moveTargetY = boss.y;
        float hpMul = Relic::enemyHpMul(m);
        if (worldIdx == 0) {
            boss.hp = boss.maxHp = 250*hpMul; boss.speed = 55; boss.damage = 1; boss.size = 60;
            boss.shootCd = 1.4f; boss.burstCd = 6; boss.chargeCd = 5.f;
            boss.type = ET_MiniBoss;
        } else if (worldIdx == 1) {
            boss.hp = boss.maxHp = 450*hpMul; boss.speed = 45; boss.damage = 2; boss.size = 64;
            boss.shootCd = 1.2f; boss.burstCd = 5; boss.summonCd = 6.f;
            boss.type = ET_MiniBoss;
        } else if (worldIdx == 2) {
            boss.hp = boss.maxHp = 650*hpMul; boss.speed = 60; boss.damage = 2; boss.size = 70;
            boss.shootCd = 1.0f; boss.burstCd = 4.5f; boss.specialCd = 4.f;
            boss.type = ET_MiniBoss;
        } else if (worldIdx == 3) {
            boss.hp = boss.maxHp = 900*hpMul; boss.speed = 70; boss.damage = 2; boss.size = 78;
            boss.shootCd = 1.6f; boss.chargeCd = 3.f;
            boss.type = ET_MiniBoss;
        } else if (worldIdx == 4) {
            boss.hp = boss.maxHp = 1800*hpMul; boss.speed = 80; boss.damage = 2; boss.size = 84;
            boss.shootCd = 0.7f; boss.burstCd = 3.5f;
            boss.specialCd = 5.f; boss.summonCd = 6.f; boss.chargeCd = 4.5f;
            boss.type = ET_FinalBoss;
        } else { // Lord Malificus (monde 6)
            boss.hp = boss.maxHp = 3000*hpMul; boss.speed = 65; boss.damage = 2; boss.size = 96;
            boss.shootCd = 0.5f; boss.burstCd = 2.5f;
            boss.specialCd = 3.5f; boss.summonCd = 5.f; boss.chargeCd = 3.5f;
            boss.patternCd = 4.f;
            boss.type = ET_TrueFinalBoss;
        }
        boss.hp *= ascensionDmgMul(m); boss.maxHp = boss.hp;
        boss.speed *= ascensionSpeedMul(m);
        m.enemies.push_back(boss);
        m.bossIndex = (int)m.enemies.size() - 1;
        Dialog::triggerBoss(m, worldIdx, 0);
        return;
    }

    // ===== Salle normale (peut etre Elite) =====
    int roomInWorld = ((roomIndex - 1) % 5) + 1;
    int count = 3 + roomInWorld + worldIdx;
    if (count > 12) count = 12;

    auto spawnEnemyHere = [&](Enemy &e, float ex, float ey){
        e.id = m.nextEnemyId++; e.x = ex; e.y = ey;
        e.hp *= Relic::enemyHpMul(m) * ascensionDmgMul(m); e.maxHp = e.hp;
        e.speed *= ascensionSpeedMul(m);
        e.anim = AN_Walk;
        m.enemies.push_back(e);
    };

    for (int i = 0; i < count; ++i) {
        float ex, ey; int safety = 50;
        do {
            ex = rndF(TILE*2.5f, GW-TILE*2.5f);
            ey = rndF(TILE*2.5f, GH-TILE*2.5f);
            safety--;
        } while ((distF(ex, ey, m.player.x, m.player.y) < 130.f
                  || Collision::collidesObstacle(m, ex, ey, 18)) && safety > 0);

        Enemy e;
        float r = QRandomGenerator::global()->generateDouble();
        if (worldIdx == 0) {
            if (r < 0.6) { e.type=ET_Slime; e.hp=e.maxHp=30; e.speed=rndF(45,65); e.size=32; }
            else { e.type=ET_Bat; e.hp=e.maxHp=22; e.speed=rndF(55,75); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
        } else if (worldIdx == 1) {
            if (r < 0.30) { e.type=ET_Slime; e.hp=e.maxHp=40; e.speed=rndF(50,70); e.size=32; }
            else if (r < 0.65) { e.type=ET_Skel; e.hp=e.maxHp=55; e.speed=rndF(45,60); e.size=32; e.shootCd=rndF(1.5f,2.5f); }
            else { e.type=ET_Bat; e.hp=e.maxHp=28; e.speed=rndF(58,78); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
        } else if (worldIdx == 2) {
            if (r < 0.20) { e.type=ET_Slime; e.hp=e.maxHp=50; e.speed=rndF(55,75); e.size=32; }
            else if (r < 0.45) { e.type=ET_Skel; e.hp=e.maxHp=70; e.speed=rndF(45,60); e.size=32; e.shootCd=rndF(1.3f,2.2f); }
            else if (r < 0.65) { e.type=ET_Bat; e.hp=e.maxHp=35; e.speed=rndF(60,80); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
            else if (r < 0.85) { e.type=ET_Brute; e.hp=e.maxHp=110; e.speed=rndF(35,50); e.size=36; e.damage=2; }
            else { e.type=ET_Mage; e.hp=e.maxHp=70; e.speed=rndF(35,50); e.size=32; e.shootCd=rndF(1.8f,2.8f); }
        } else if (worldIdx == 3) {
            if (r < 0.15) { e.type=ET_Slime; e.hp=e.maxHp=70; e.speed=rndF(60,80); e.size=32; }
            else if (r < 0.35) { e.type=ET_Skel; e.hp=e.maxHp=90; e.speed=rndF(50,65); e.size=32; e.shootCd=rndF(1.1f,2.0f); }
            else if (r < 0.55) { e.type=ET_Bat; e.hp=e.maxHp=45; e.speed=rndF(65,90); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
            else if (r < 0.80) { e.type=ET_Brute; e.hp=e.maxHp=160; e.speed=rndF(40,55); e.size=36; e.damage=2; }
            else { e.type=ET_Mage; e.hp=e.maxHp=100; e.speed=rndF(40,55); e.size=32; e.shootCd=rndF(1.4f,2.4f); }
        } else if (worldIdx == 4) {
            if (r < 0.10) { e.type=ET_Slime; e.hp=e.maxHp=90; e.speed=rndF(65,85); e.size=32; }
            else if (r < 0.30) { e.type=ET_Skel; e.hp=e.maxHp=120; e.speed=rndF(55,70); e.size=32; e.shootCd=rndF(1.0f,1.8f); }
            else if (r < 0.50) { e.type=ET_Bat; e.hp=e.maxHp=60; e.speed=rndF(75,100); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
            else if (r < 0.75) { e.type=ET_Brute; e.hp=e.maxHp=210; e.speed=rndF(45,60); e.size=36; e.damage=2; }
            else { e.type=ET_Mage; e.hp=e.maxHp=140; e.speed=rndF(45,60); e.size=32; e.shootCd=rndF(1.2f,2.0f); }
        } else {
            // Monde 6 : ennemis durs
            if (r < 0.20) { e.type=ET_Brute; e.hp=e.maxHp=260; e.speed=rndF(50,65); e.size=36; e.damage=2; }
            else if (r < 0.45) { e.type=ET_Mage; e.hp=e.maxHp=180; e.speed=rndF(50,65); e.size=32; e.shootCd=rndF(1.0f,1.6f); }
            else if (r < 0.70) { e.type=ET_Skel; e.hp=e.maxHp=160; e.speed=rndF(60,75); e.size=32; e.shootCd=rndF(0.9f,1.6f); }
            else { e.type=ET_Bat; e.hp=e.maxHp=80; e.speed=rndF(80,110); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
        }
        spawnEnemyHere(e, ex, ey);
    }

    // Si Elite : ajoute un champion
    if (m.roomType == RT_Elite) {
        Enemy e;
        float ex, ey; int safety = 50;
        do {
            ex = rndF(TILE*3, GW-TILE*3); ey = rndF(TILE*3, GH-TILE*3); safety--;
        } while ((distF(ex, ey, m.player.x, m.player.y) < 150
                 || Collision::collidesObstacle(m, ex, ey, 24)) && safety > 0);
        e.type = ET_Elite; e.elite = true;
        e.eliteVariant = rndI(0, 3);
        e.hp = e.maxHp = (300 + worldIdx*100) * Relic::enemyHpMul(m) * ascensionDmgMul(m);
        e.speed = (45 + worldIdx*5) * ascensionSpeedMul(m); e.damage = 2;
        e.size = 44;
        e.shootCd = rndF(1.0f, 1.8f);
        e.chargeCd = 4.f;
        e.dropChance = 1.f;
        spawnEnemyHere(e, ex, ey);
    }
}

}}
