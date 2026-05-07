#include "roombuilder.h"
#include "collisionsystem.h"
#include "skillsystem.h"
#include "dialogsystem.h"
#include "../model/gamemodel.h"
#include "../core/constants.h"
#include "../core/utils.h"
#include <QRandomGenerator>
#include <algorithm>

namespace Controller { namespace Room {

using namespace Model;
using namespace Core;

int  worldOf(int room) { return std::min(5, ((room-1)/5) + 1); }
bool isBossRoom(int room) { return room % 5 == 0; }
bool isFinalBossRoom(int room) { return room == 25; }

void spawnMinion(GameModel &m, float x, float y, int hp, float speed, EnemyType type)
{
    Enemy e;
    e.id = m.nextEnemyId++;
    e.x = x; e.y = y;
    e.hp = e.maxHp = hp;
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
    m.player.x = GW/2.f;
    m.player.y = GH/2.f;
    m.enemies.clear(); m.bullets.clear(); m.particles.clear();
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
    m.bossIndex = -1;
    for (int i = 0; i < 4; ++i) m.doorOpen[i] = false;

    if      (entryDoor==0) { m.player.x = DOOR_COL*TILE+TILE/2.f; m.player.y = TILE*2.5f; }
    else if (entryDoor==1) { m.player.x = GW-TILE*2.5f; m.player.y = DOOR_ROW*TILE+TILE/2.f; }
    else if (entryDoor==2) { m.player.x = DOOR_COL*TILE+TILE/2.f; m.player.y = GH-TILE*2.5f; }
    else if (entryDoor==3) { m.player.x = TILE*2.5f; m.player.y = DOOR_ROW*TILE+TILE/2.f; }
    else                   { m.player.x = GW/2.f; m.player.y = GH/2.f; }

    m.layoutIdx = 0;
    Collision::resolveCollision(m, m.player.x, m.player.y, 8.f);

    int worldIdx = worldOf(roomIndex) - 1;

    if (isBossRoom(roomIndex) && Skills::hasSkill(m, SK_INVUL_BURST)) {
        m.player.invulBurstTimer = 4.f;
        m.player.invincibility   = std::max(m.player.invincibility, 4.f);
    }

    if (isBossRoom(roomIndex)) {
        Enemy boss;
        boss.id = m.nextEnemyId++;
        boss.x = GW/2.f; boss.y = GH/2.f - 30;
        boss.subType = worldIdx;
        boss.phase = 1;
        boss.moveCd = rndF(0.8f, 1.6f);
        boss.moveTargetX = boss.x; boss.moveTargetY = boss.y;
        if (worldIdx == 0) {
            boss.hp = boss.maxHp = 250; boss.speed = 55; boss.damage = 1; boss.size = 60;
            boss.shootCd = 1.4f; boss.burstCd = 6; boss.chargeCd = 5.f;
            boss.type = ET_MiniBoss;
        } else if (worldIdx == 1) {
            boss.hp = boss.maxHp = 450; boss.speed = 45; boss.damage = 2; boss.size = 64;
            boss.shootCd = 1.2f; boss.burstCd = 5; boss.summonCd = 6.f;
            boss.type = ET_MiniBoss;
        } else if (worldIdx == 2) {
            boss.hp = boss.maxHp = 650; boss.speed = 60; boss.damage = 2; boss.size = 70;
            boss.shootCd = 1.0f; boss.burstCd = 4.5f; boss.specialCd = 4.f;
            boss.type = ET_MiniBoss;
        } else if (worldIdx == 3) {
            boss.hp = boss.maxHp = 900; boss.speed = 70; boss.damage = 2; boss.size = 78;
            boss.shootCd = 1.6f; boss.chargeCd = 3.f;
            boss.type = ET_MiniBoss;
        } else {
            boss.hp = boss.maxHp = 1800; boss.speed = 80; boss.damage = 2; boss.size = 84;
            boss.shootCd = 0.7f; boss.burstCd = 3.5f;
            boss.specialCd = 5.f; boss.summonCd = 6.f; boss.chargeCd = 4.5f;
            boss.type = ET_FinalBoss;
        }
        m.enemies.push_back(boss);
        m.bossIndex = (int)m.enemies.size() - 1;
        Dialog::triggerBoss(m, worldIdx, 0);
        return;
    }

    int roomInWorld = ((roomIndex - 1) % 5) + 1;
    int count = 3 + roomInWorld + worldIdx;
    if (count > 12) count = 12;

    for (int i = 0; i < count; ++i) {
        float ex, ey; int safety = 50;
        do {
            ex = rndF(TILE*2.5f, GW-TILE*2.5f);
            ey = rndF(TILE*2.5f, GH-TILE*2.5f);
            safety--;
        } while ((distF(ex, ey, m.player.x, m.player.y) < 130.f
                  || Collision::collidesObstacle(m, ex, ey, 18)) && safety > 0);

        Enemy e;
        e.id = m.nextEnemyId++;
        e.x = ex; e.y = ey;
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
        } else {
            if (r < 0.10) { e.type=ET_Slime; e.hp=e.maxHp=90; e.speed=rndF(65,85); e.size=32; }
            else if (r < 0.30) { e.type=ET_Skel; e.hp=e.maxHp=120; e.speed=rndF(55,70); e.size=32; e.shootCd=rndF(1.0f,1.8f); }
            else if (r < 0.50) { e.type=ET_Bat; e.hp=e.maxHp=60; e.speed=rndF(75,100); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
            else if (r < 0.75) { e.type=ET_Brute; e.hp=e.maxHp=210; e.speed=rndF(45,60); e.size=36; e.damage=2; }
            else { e.type=ET_Mage; e.hp=e.maxHp=140; e.speed=rndF(45,60); e.size=32; e.shootCd=rndF(1.2f,2.0f); }
        }
        e.anim = AN_Walk;
        m.enemies.push_back(e);
    }
}

}}
