#include "playercontroller.h"
#include "combatsystem.h"
#include "skillsystem.h"
#include "collisionsystem.h"
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
    if (pl.dashActive > 0) spd *= 4.5f;

    if (pl.moving) {
        vx /= len; vy /= len;
        Collision::tryMove(m, pl.x, pl.y, vx*spd*dt, vy*spd*dt, 8.f);
        pl.facing = std::atan2(vy, vx);
        pl.facingLeft = vx < 0;
        if (pl.anim != AN_Walk) { pl.anim = AN_Walk; pl.animTimer = 0; }

        if (Skills::hasSkill(m, SK_RAM) && pl.dashActive > 0) {
            for (auto &e : m.enemies) {
                if (e.dead) continue;
                if (distF(e.x, e.y, pl.x, pl.y) < 22 + e.size*0.4f) {
                    if (e.hitFlash <= 0.05f) Combat::hurtEnemy(m, e, 25);
                }
            }
        }
    } else {
        if (pl.atkTimer <= 0 && pl.anim != AN_Hurt) pl.anim = AN_Idle;
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
    if (m.keys.contains(Qt::Key_Shift) && Skills::hasSkill(m, SK_DASH)
        && pl.dashCd <= 0 && pl.dashActive <= 0) {
        Combat::activateDash(m);
    }
    if (m.keys.contains(Qt::Key_T) && Skills::hasSkill(m, SK_TIME_STOP) && !pl.timeStopUsed) {
        Combat::activateTimeStop(m);
    }

    if (pl.invincibility <= 0 && pl.dashActive <= 0) {
        for (auto &e : m.enemies) {
            if (e.dead) continue;
            if (distF(pl.x, pl.y, e.x, e.y) < 10 + e.size*0.35f) {
                if (Skills::hasSkill(m, SK_DODGE) && QRandomGenerator::global()->generateDouble()<0.06) break;
                if (Skills::hasSkill(m, SK_SHIELD) && pl.shieldReadyTimer <= 0) {
                    pl.shieldReadyTimer = 12.f; pl.invincibility = 1.f; break;
                }
                if (Skills::hasSkill(m, SK_THORNS)) Combat::hurtEnemy(m, e, e.damage * 0.5f);
                Combat::hurtPlayer(m, e.damage * Skills::incomingDmgMul(m));
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
                Combat::hurtPlayer(m, b.damage * Skills::incomingDmgMul(m));
            }
        }
    }
}

}}
