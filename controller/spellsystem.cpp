#include "spellsystem.h"
#include "skillsystem.h"
#include "combatsystem.h"
#include "../model/gamemodel.h"
#include "../core/utils.h"
#include "../core/constants.h"
#include "../core/palette.h"
#include <QRandomGenerator>
#include <cmath>
#include <algorithm>

namespace Controller { namespace Spell {

using namespace Model;
using namespace Core;

float spellDmgMul(const GameModel &m)
{
    float mul = m.player.spellDmgMul;
    if (Skills::hasSkill(m, SK_ARCANE_BOOST))   mul *= 1.40f;
    if (Skills::hasSkill(m, SK_MANA_SHIELD))     mul *= 3.00f;
    if (Skills::hasSkill(m, SK_ELEMENTAL_FURY) && m.player.frenzyBuff > 0) mul *= 1.30f;
    return mul;
}

static float getCd(const GameModel &m, float baseCd)
{
    float cd = baseCd;
    if (Skills::hasSkill(m, SK_COOLDOWN_MASTERY)) cd *= 0.75f;
    return cd;
}

static void spawnSpellParticles(GameModel &m, float x, float y, QColor col, int count)
{
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = x + rndF(-8,8); p.y = y + rndF(-8,8);
        float a = rndF(0, 6.28f), spd = rndF(60,180);
        p.vx = std::cos(a)*spd; p.vy = std::sin(a)*spd - 40;
        p.color = col; p.life = p.maxLife = rndF(0.4f, 0.8f);
        p.size = (int)rndF(3,6); p.noGravity = true;
        m.particles.push_back(p);
    }
}

void castSpell(GameModel &m, int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 4) return;
    int spellId = m.player.spellSlots[slotIndex];
    if (spellId < 0 || spellId >= SP__COUNT) return;
    if (m.player.spellCds[slotIndex] > 0) return;

    // SK_MANA_SHIELD : coûte 1 PV
    if (Skills::hasSkill(m, SK_MANA_SHIELD)) {
        if (m.player.hp <= 1) return;
        m.player.hp -= 1.0f;
    }

    // Récupérer le cooldown de base
    float baseCd = 8.f;
    for (auto &sp : m.allSpells)
        if (sp.id == (SpellId)spellId) { baseCd = sp.baseCd; break; }

    m.player.spellCds[slotIndex] = getCd(m, baseCd);
    float dmgMul = spellDmgMul(m);

    // SK_SPELL_ECHO : 20% chance de déclencher 2x
    bool echo = Skills::hasSkill(m, SK_SPELL_ECHO)
                && QRandomGenerator::global()->generateDouble() < 0.20;

    auto docast = [&]() {
        switch ((SpellId)spellId) {

        case SP_Teleport: {
            float ang = m.player.facing;
            float dist = 200.f;
            float nx = m.player.x + std::cos(ang)*dist;
            float ny = m.player.y + std::sin(ang)*dist;
            nx = clampF(nx, TILE*2.f, GW - TILE*2.f);
            ny = clampF(ny, TILE*2.f, GH - TILE*2.f);
            spawnSpellParticles(m, m.player.x, m.player.y, QColor("#00eeff"), 12);
            m.player.x = nx; m.player.y = ny;
            m.player.invincibility = 0.3f;
            spawnSpellParticles(m, m.player.x, m.player.y, QColor("#00eeff"), 12);
            break;
        }

        case SP_Lightning: {
            float dmg = 40.f * dmgMul;
            std::vector<Enemy*> targets;
            for (auto &e : m.enemies) {
                if (e.dead) continue;
                targets.push_back(&e);
            }
            std::sort(targets.begin(), targets.end(), [&](Enemy *a, Enemy *b){
                return distF(a->x,a->y,m.player.x,m.player.y)
                     < distF(b->x,b->y,m.player.x,m.player.y);
            });
            int chains = 3;
            for (auto *ep : targets) {
                if (chains-- <= 0) break;
                Combat::hurtEnemy(m, *ep, dmg);
                ep->stunTimer = 0.6f;
                spawnSpellParticles(m, ep->x, ep->y, QColor("#ffff44"), 8);
            }
            break;
        }

        case SP_Invisibility:
            m.player.invincibility = 3.0f;
            m.player.adrenalineTimer = 3.0f;  // vitesse boost via adrenaline
            spawnSpellParticles(m, m.player.x, m.player.y, QColor("#aaaaff"), 16);
            break;

        case SP_Repulsion: {
            for (auto &e : m.enemies) {
                if (e.dead) continue;
                float dx = e.x - m.player.x, dy = e.y - m.player.y;
                float d = std::hypot(dx, dy);
                if (d < 120.f && d > 0.1f) {
                    e.x += (dx/d) * 80.f;
                    e.y += (dy/d) * 80.f;
                    e.x = clampF(e.x, TILE*1.5f, GW - TILE*1.5f);
                    e.y = clampF(e.y, TILE*1.5f, GH - TILE*1.5f);
                    e.stunTimer = 0.4f;
                }
            }
            spawnSpellParticles(m, m.player.x, m.player.y, QColor("#ff8844"), 20);
            break;
        }

        case SP_Fireball: {
            float dmg = 80.f * dmgMul;
            float rad = 60.f;
            float fx = m.player.x + std::cos(m.player.facing)*180.f;
            float fy = m.player.y + std::sin(m.player.facing)*180.f;
            fx = clampF(fx, TILE*1.5f, GW - TILE*1.5f);
            fy = clampF(fy, TILE*1.5f, GH - TILE*1.5f);
            for (auto &e : m.enemies) {
                if (e.dead) continue;
                if (distF(e.x, e.y, fx, fy) < rad + e.size*0.3f) {
                    Combat::hurtEnemy(m, e, dmg);
                    e.burnTimer = 4.f; e.burnDps = 5.f;
                }
            }
            for (int i=0; i<28; ++i) {
                float a = rndF(0, 6.28f), spd = rndF(60,200);
                Particle p; p.x = fx; p.y = fy;
                p.vx = std::cos(a)*spd; p.vy = std::sin(a)*spd - 60;
                p.color = (i%2==0)?Palette::PAL_FIRE[0]:Palette::PAL_FIRE[1];
                p.life = p.maxLife = rndF(0.4f, 0.9f); p.size = (int)rndF(4,8);
                m.particles.push_back(p);
            }
            break;
        }

        case SP_Blizzard: {
            for (auto &e : m.enemies) {
                if (e.dead || e.type >= ET_FinalBoss) continue;
                e.slowTimer = std::max(e.slowTimer, 2.5f);
                e.stunTimer = std::max(e.stunTimer, 0.5f);
                spawnSpellParticles(m, e.x, e.y, QColor("#aaddff"), 4);
            }
            spawnSpellParticles(m, m.player.x, m.player.y, QColor("#88ccff"), 24);
            break;
        }

        case SP_Laser: {
            float dmg = 60.f * dmgMul;
            float a = m.player.facing;
            float lx = m.player.x, ly = m.player.y;
            for (int step = 0; step < 30; ++step) {
                lx += std::cos(a)*18.f; ly += std::sin(a)*18.f;
                if (lx < TILE || lx > GW-TILE || ly < TILE || ly > GH-TILE) break;
                for (auto &e : m.enemies) {
                    if (e.dead || e.hitFlash > 0.14f) continue;
                    if (distF(lx, ly, e.x, e.y) < e.size*0.5f + 6) {
                        Combat::hurtEnemy(m, e, dmg);
                    }
                }
                Particle p; p.x = lx + rndF(-3,3); p.y = ly + rndF(-3,3);
                p.vx = rndF(-20,20); p.vy = rndF(-20,20);
                p.color = QColor("#ff00ff"); p.life = p.maxLife = 0.2f; p.size = 3; p.noGravity = true;
                m.particles.push_back(p);
            }
            break;
        }

        case SP_ShieldWall:
            m.player.invincibility = 2.0f;
            spawnSpellParticles(m, m.player.x, m.player.y, QColor("#ffd700"), 20);
            break;

        default: break;
        }
    };

    docast();
    if (echo) docast();

    // SK_ELEMENTAL_FURY : boost dégâts flèches 4s
    if (Skills::hasSkill(m, SK_ELEMENTAL_FURY))
        m.player.frenzyBuff = std::max(m.player.frenzyBuff, 4.0f);
}

void update(GameModel &m, float dt)
{
    // Décompter les cooldowns de sorts
    for (auto &cd : m.player.spellCds)
        if (cd > 0) cd -= dt;

    // Gestion laves (SK_MAGMA)
    for (auto &lv : m.lavaTiles) {
        lv.life -= dt;
        for (auto &e : m.enemies) {
            if (e.dead) continue;
            if (distF(lv.x, lv.y, e.x, e.y) < 20) {
                e.burnTimer = std::max(e.burnTimer, 2.0f);
                e.burnDps = std::max(e.burnDps, 4.f);
            }
        }
    }
    m.lavaTiles.erase(std::remove_if(m.lavaTiles.begin(), m.lavaTiles.end(),
        [](const LavaTile &l){ return l.life <= 0; }), m.lavaTiles.end());

    // Clones fantômes (SK_SHADOW_DASH)
    for (auto &sc : m.shadowClones) sc.life -= dt;
    m.shadowClones.erase(std::remove_if(m.shadowClones.begin(), m.shadowClones.end(),
        [](const ShadowClone &sc){ return sc.life <= 0; }), m.shadowClones.end());
    // Clones distraient les ennemis
    for (auto &e : m.enemies) {
        if (e.dead || m.shadowClones.empty()) continue;
        for (auto &sc : m.shadowClones) {
            if (distF(e.x, e.y, sc.x, sc.y) < 60 && rndF(0,1) < 0.3f) {
                float ang = std::atan2(sc.y - e.y, sc.x - e.x);
                e.x += std::cos(ang)*e.speed*0.5f;
                e.y += std::sin(ang)*e.speed*0.5f;
            }
        }
    }

    // SK_VAMPIRE_AURA
    if (Skills::hasSkill(m, SK_VAMPIRE_AURA)) {
        for (auto &e : m.enemies) {
            if (e.dead) continue;
            if (distF(e.x, e.y, m.player.x, m.player.y) < 60) {
                e.hp -= dt * 1.0f;
                m.player.hp = std::min(m.player.maxHp, m.player.hp + dt * 0.8f);
                if (e.hp <= 0 && !e.dead) Combat::hurtEnemy(m, e, 0.01f);
            }
        }
    }

    // SK_AWAKENING : buff aléatoire par salle
    if (m.player.awakenBuff > 0) m.player.awakenBuff -= dt;

    // SK_FRENZY buff timer
    if (m.player.frenzyBuff > 0) m.player.frenzyBuff -= dt;
    if (m.player.frenzyTimer > 0) {
        m.player.frenzyTimer -= dt;
        if (m.player.frenzyTimer <= 0) m.player.frenzyKills = 0;
    }

    // SK_OVERLOAD
    if (m.player.overloadTimer < 8.f) m.player.overloadTimer += dt;
    else m.player.overloadReady = true;

    // SK_VOID_STEP cooldown
    if (m.player.voidStepCd > 0) m.player.voidStepCd -= dt;
    if (m.player.voidStepActive > 0) {
        m.player.voidStepActive -= dt;
        if (m.player.voidStepActive <= 0) m.player.invincibility = 0;
    }

    // SK_DOUBLE_DASH : recharge des charges
    if (m.player.dashCharges < m.player.dashChargesMax) {
        m.player.dashChargeTimer -= dt;
        if (m.player.dashChargeTimer <= 0) {
            m.player.dashCharges++;
            m.player.dashChargeTimer = 3.0f;
        }
    }

    // SK_ACCELERATE
    if (m.player.moving) {
        m.player.accelerateTimer += dt;
        m.player.accelerateMul = 1.f + std::min(0.30f, m.player.accelerateTimer * 0.1f);
    } else {
        m.player.accelerateTimer = 0;
        m.player.accelerateMul = 1.f;
    }

    // Régénération du bouclier entre salles (full regen sur transition)
    // Géré dans buildRoom
}

}}
