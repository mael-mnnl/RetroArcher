#include "pickupsystem.h"
#include "relicsystem.h"
#include "../model/gamemodel.h"
#include "../model/types.h"
#include "../core/utils.h"
#include "../core/constants.h"
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

namespace Controller { namespace Pickup {

using namespace Model;
using namespace Core;

static void spawnBase(GameModel &m, PickupType t, float x, float y, int value)
{
    Model::Pickup p;
    p.type = t; p.x = x; p.y = y;
    p.vx = rndF(-30,30); p.vy = rndF(-60,-20);
    p.bobPhase = rndF(0, 6.28f);
    p.lifeSec = 20.f;
    p.value = value;
    m.pickups.push_back(p);
}

void spawnGold(GameModel &m, float x, float y, int amount)   { spawnBase(m, PU_Gold,   x, y, amount); }
void spawnHeart(GameModel &m, float x, float y)              { spawnBase(m, PU_Heart,  x, y, 1); }
void spawnPotion(GameModel &m, float x, float y)             { spawnBase(m, PU_Potion, x, y, 3); }
void spawnChest(GameModel &m, float x, float y)              { spawnBase(m, PU_Chest,  x, y, 1); }

void spawnScroll(GameModel &m, float x, float y)
{
    Model::Pickup p; p.type = PU_Scroll; p.x = x; p.y = y;
    p.vx = rndF(-30,30); p.vy = rndF(-60,-20);
    p.bobPhase = rndF(0, 6.28f); p.lifeSec = 20.f; p.value = 1;
    if (!m.allSkills.empty())
        p.scrollSkillId = m.allSkills[rndI(0, (int)m.allSkills.size()-1)].id;
    m.pickups.push_back(p);
}

void tryRandomDrop(GameModel &m, float x, float y, bool elite)
{
    double r = QRandomGenerator::global()->generateDouble();
    if (elite) {
        // Elite : drop garanti rare
        if (r < 0.5) spawnChest(m, x, y);
        else if (r < 0.85) spawnScroll(m, x, y);
        else spawnPotion(m, x, y);
        spawnGold(m, x + rndF(-10,10), y, 5 + rndI(0,10));
        return;
    }
    // Normal drops
    if (r < 0.20) spawnGold(m, x, y, 1 + rndI(0,3));
    else if (r < 0.25) spawnHeart(m, x, y);
    else if (r < 0.27) spawnScroll(m, x, y);
}

void update(GameModel &m, float dt)
{
    for (auto &p : m.pickups) {
        // physics + petit bounce ground
        p.vy += 200*dt;
        p.x += p.vx*dt; p.y += p.vy*dt;
        // Bound dans la salle
        if (p.y > GH - TILE*1.5f) { p.y = GH - TILE*1.5f; p.vy = -std::abs(p.vy)*0.4f; p.vx *= 0.7f; }
        if (p.x < TILE*1.3f) { p.x = TILE*1.3f; p.vx = std::abs(p.vx)*0.5f; }
        if (p.x > GW-TILE*1.3f) { p.x = GW-TILE*1.3f; p.vx = -std::abs(p.vx)*0.5f; }
        p.bobPhase += dt * 3.f;
        p.lifeSec -= dt;

        // Pickup if player near
        if (distF(p.x, p.y, m.player.x, m.player.y) < 22) {
            switch (p.type) {
            case PU_Gold:
                m.player.gold += (int)(p.value * Relic::goldMul(m));
                break;
            case PU_Heart:
                m.player.hp = std::min(m.player.maxHp, m.player.hp + 1.f);
                break;
            case PU_Potion:
                m.player.hp = std::min(m.player.maxHp, m.player.hp + 3.f);
                break;
            case PU_Scroll:
                m.player.tempScrollSkillId = p.scrollSkillId;
                m.player.skills.push_back(p.scrollSkillId);
                break;
            case PU_Chest:
                m.player.gold += (int)(15 * Relic::goldMul(m));
                m.player.hp = std::min(m.player.maxHp, m.player.hp + 1.f);
                break;
            }
            p.lifeSec = 0;
        }
    }
    m.pickups.erase(std::remove_if(m.pickups.begin(), m.pickups.end(),
                    [](const Model::Pickup &p){ return p.lifeSec <= 0; }), m.pickups.end());
}

}}
