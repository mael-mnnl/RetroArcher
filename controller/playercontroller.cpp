#include "playercontroller.h"
#include "combatsystem.h"
#include "skillsystem.h"
#include "collisionsystem.h"
#include "cursesystem.h"
#include "relicsystem.h"
#include "spellsystem.h"
#include "../model/gamemodel.h"
#include "../core/utils.h"
#include "../core/constants.h"
#include <QKeyEvent>
#include <QtGlobal>
#include <QRandomGenerator>
#include <cmath>
#include <algorithm>

namespace Controller { namespace PlayerCtl {

using namespace Model;
using namespace Core;

void update(GameModel &m, float dt)
{
    Player &pl = m.player;
    pl.animTimer += dt;
    if (pl.atkTimer > 0) pl.atkTimer -= dt;
    if (pl.invincibility > 0) pl.invincibility -= dt;
    if (pl.grenadeCd > 0) pl.grenadeCd -= dt;

    float vx = 0, vy = 0;
    if (m.keys.contains(Qt::Key_Left)  || m.keys.contains(Qt::Key_A) || m.keys.contains(Qt::Key_Q)) vx -= 1;
    if (m.keys.contains(Qt::Key_Right) || m.keys.contains(Qt::Key_D)) vx += 1;
    if (m.keys.contains(Qt::Key_Up)    || m.keys.contains(Qt::Key_W) || m.keys.contains(Qt::Key_Z)) vy -= 1;
    if (m.keys.contains(Qt::Key_Down)  || m.keys.contains(Qt::Key_S)) vy += 1;

    float len = std::hypot(vx, vy);
    pl.moving = (len > 0);
    pl.stillSec = pl.moving ? 0 : (pl.stillSec + dt);

    float spd = pl.speed * Skills::speedMul(m);
    if (Skills::hasSkill(m, SK_ACCELERATE)) spd *= pl.accelerateMul;
    if (pl.dashActive > 0) spd *= 2.5f;   // dash réduit : 2.5x au lieu de 4.5x

    if (pl.moving) {
        vx /= len; vy /= len;
        Collision::tryMove(m, pl.x, pl.y, vx*spd*dt, vy*spd*dt, 8.f);
        pl.facing = std::atan2(vy, vx);
        pl.facingLeft = vx < 0;
        if (pl.anim != AN_Walk) { pl.anim = AN_Walk; pl.animTimer = 0; }

        // SK_MOMENTUM
        if (Skills::hasSkill(m, SK_MOMENTUM))
            pl.momentumDist += spd * dt;

        if (Skills::hasSkill(m, SK_RAM) && pl.dashActive > 0) {
            for (auto &e : m.enemies) {
                if (e.dead) continue;
                if (distF(e.x, e.y, pl.x, pl.y) < 22 + e.size*0.4f) {
                    if (e.hitFlash <= 0.05f) Combat::hurtEnemy(m, e, 25);
                    // SK_DIVE_DASH : étourdit
                    if (Skills::hasSkill(m, SK_DIVE_DASH)) e.stunTimer = std::max(e.stunTimer, 0.8f);
                }
            }
        }
    } else {
        if (pl.atkTimer <= 0 && pl.anim != AN_Hurt) pl.anim = AN_Idle;
        pl.momentumDist = 0;
    }
    pl.x = clampF(pl.x, -TILE, GW+TILE);
    pl.y = clampF(pl.y, -TILE, GH+TILE);

    pl.fireCd -= dt;
    bool anyEnemy = false;
    for (auto &e : m.enemies) if (!e.dead) { anyEnemy = true; break; }
    float effFireRate = pl.fireRate * Skills::fireRateMul(m);
    if (!pl.moving && pl.fireCd <= 0 && anyEnemy) {
        pl.fireCd = effFireRate;
        Combat::shootPlayer(m);
        pl.anim = AN_Atk; pl.animTimer = 0; pl.atkTimer = 0.4f;
        if (Skills::hasSkill(m, SK_BURST_FIRE) && QRandomGenerator::global()->generateDouble() < 0.25)
            pl.burstQueueTimer = 0.10f;

        // SK_LUCKY_SHOT
        if (Skills::hasSkill(m, SK_LUCKY_SHOT)) {
            pl.shotCounter++;
            if (pl.shotCounter >= 7) { pl.shotCounter = 0; /* handled in shootPlayer via flag */ }
        }
    }
    if (pl.burstQueueTimer > 0) {
        pl.burstQueueTimer -= dt;
        if (pl.burstQueueTimer <= 0 && anyEnemy) Combat::shootPlayer(m);
    }
    if (pl.moving && pl.fireCd < effFireRate*0.5f) pl.fireCd = effFireRate*0.5f;

    if (m.keys.contains(Qt::Key_G) && pl.grenadeAmmo > 0 && pl.grenadeCd <= 0) {
        Combat::throwGrenade(m); pl.grenadeAmmo--; pl.grenadeCd = 0.3f;
        if (Skills::hasSkill(m, SK_DOUBLE_GRENADE) && pl.grenadeAmmo > 0) {
            Combat::throwGrenade(m); pl.grenadeAmmo--;
        }
    }

    // Dash (SHIFT)
    bool canDash = Skills::hasSkill(m, SK_DASH) && pl.dashCd <= 0 && pl.dashActive <= 0
                   && pl.dashCharges > 0;
    if (m.keys.contains(Qt::Key_Shift) && canDash) {
        if (pl.hasDashAim) {
            float ang = std::atan2(pl.dashAimY - pl.y, pl.dashAimX - pl.x);
            pl.facing = ang;
            pl.facingLeft = std::cos(ang) < 0;
            pl.hasDashAim = false;
        }
        Combat::activateDash(m);
        pl.dashCd *= Curse::dashCdMul(m);
        pl.dashCharges--;

        // SK_SHADOW_DASH : clone
        if (Skills::hasSkill(m, SK_SHADOW_DASH)) {
            ShadowClone sc; sc.x = pl.x; sc.y = pl.y; sc.life = 2.0f;
            m.shadowClones.push_back(sc);
        }
        // SK_SPECTRAL_DASH : invulnérabilité extra
        if (Skills::hasSkill(m, SK_SPECTRAL_DASH)) pl.invincibility += 1.0f;
    }

    // Arrêt du temps (T)
    if (m.keys.contains(Qt::Key_T) && Skills::hasSkill(m, SK_TIME_STOP) && !pl.timeStopUsed) {
        Combat::activateTimeStop(m);
    }

    // Pas du Vide (Maj+T)
    if (m.keys.contains(Qt::Key_T) && m.keys.contains(Qt::Key_Shift)
        && Skills::hasSkill(m, SK_VOID_STEP) && pl.voidStepCd <= 0) {
        pl.voidStepCd     = 20.f;
        pl.voidStepActive = 2.0f;
        pl.invincibility  = 2.0f;
    }

    // Collisions joueur
    if (pl.invincibility <= 0 && pl.dashActive <= 0 && pl.voidStepActive <= 0) {
        for (auto &e : m.enemies) {
            if (e.dead || e.stunTimer > 0) continue;
            if (distF(pl.x, pl.y, e.x, e.y) < 10 + e.size*0.35f) {
                if (Skills::hasSkill(m, SK_DODGE) && QRandomGenerator::global()->generateDouble()<0.06) break;
                if (Skills::hasSkill(m, SK_SHIELD) && pl.shieldReadyTimer <= 0) {
                    pl.shieldReadyTimer = 12.f; pl.invincibility = 1.f; break;
                }
                if (Skills::hasSkill(m, SK_THORNS)) Combat::hurtEnemy(m, e, e.damage * 0.5f);
                float dmgIncoming = e.damage * Skills::incomingDmgMul(m) * Curse::playerIncomingMul(m);
                // SK_STONE_SKIN
                if (Skills::hasSkill(m, SK_STONE_SKIN)) dmgIncoming = std::max(0.5f, dmgIncoming - 1.f);
                // SK_IRON_WILL
                if (Skills::hasSkill(m, SK_IRON_WILL) && pl.hp < pl.maxHp * 0.30f) dmgIncoming *= 0.6f;
                // SK_BARRIER_ROOM
                if (Skills::hasSkill(m, SK_BARRIER_ROOM) && !pl.barrierRoomUsed) {
                    pl.barrierRoomUsed = true; pl.invincibility = 0.5f; break;
                }
                Combat::hurtPlayer(m, dmgIncoming);
                break;
            }
        }
        for (auto &b : m.bullets) {
            if (!b.enemy || b.dead) continue;
            if (distF(pl.x, pl.y, b.x, b.y) < 12) {
                if (Skills::hasSkill(m, SK_DODGE) && QRandomGenerator::global()->generateDouble()<0.06) {
                    b.dead = true; break;
                }
                if (Skills::hasSkill(m, SK_REFLECT) && QRandomGenerator::global()->generateDouble()<0.20) {
                    b.enemy = false; b.vx *= -1; b.vy *= -1; b.angle += M_PI; continue;
                }
                if (Skills::hasSkill(m, SK_SHIELD) && pl.shieldReadyTimer <= 0) {
                    pl.shieldReadyTimer = 12.f; pl.invincibility = 1.f; b.dead = true; break;
                }
                b.dead = true;
                float dmgIncoming = b.damage * Skills::incomingDmgMul(m) * Curse::playerIncomingMul(m);
                if (Skills::hasSkill(m, SK_STONE_SKIN)) dmgIncoming = std::max(0.5f, dmgIncoming - 1.f);
                if (Skills::hasSkill(m, SK_IRON_WILL) && pl.hp < pl.maxHp * 0.30f) dmgIncoming *= 0.6f;
                if (Skills::hasSkill(m, SK_BARRIER_ROOM) && !pl.barrierRoomUsed) {
                    pl.barrierRoomUsed = true; pl.invincibility = 0.5f; break;
                }
                Combat::hurtPlayer(m, dmgIncoming);
            }
        }
    }
}

}}
