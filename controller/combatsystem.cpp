#include "combatsystem.h"
#include "skillsystem.h"
#include "collisionsystem.h"
#include "dialogsystem.h"
#include "pickupsystem.h"
#include "scoresystem.h"
#include "relicsystem.h"
#include "cursesystem.h"
#include "../model/gamemodel.h"
#include "../model/types.h"
#include "../core/utils.h"
#include "../core/constants.h"
#include "../core/palette.h"
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

namespace Controller { namespace Combat {

using namespace Model;
using namespace Core;

void shootPlayer(GameModel &m)
{
    Player &pl = m.player;
    Enemy *near = nullptr;
    float nd = 1e9f;
    for (auto &e : m.enemies) {
        if (e.dead) continue;
        float d = distF(pl.x, pl.y, e.x, e.y);
        if (pl.nextEnemyMarked == e.id) { near = &e; nd = d; break; }
        if (d < nd) { nd = d; near = &e; }
    }
    if (!near) return;

    bool prc = Skills::pierceLevel(m) > 0;
    int  bnc = Skills::bounceLevel(m);
    bool tri = Skills::hasSkill(m, SK_TRI);
    bool dbl = Skills::hasSkill(m, SK_DBL);
    bool dia = Skills::hasSkill(m, SK_DIA);
    bool quad = Skills::hasSkill(m, SK_QUAD_NERFED);
    bool crit = Skills::hasSkill(m, SK_CRIT_NERFED);
    bool icy = Skills::hasSkill(m, SK_FRZ_NERFED) || Skills::hasSkill(m, SK_FREEZE_HIT)
            || Skills::hasSkill(m, SK_FROST_AURA);
    bool poi = Skills::hasSkill(m, SK_POISON);
    bool brn = Skills::hasSkill(m, SK_BLAZE);
    bool hom = Skills::hasSkill(m, SK_HOMING);
    bool ghost = Skills::hasSkill(m, SK_GHOST_ARROW);
    float speed = 320.f;
    if (Skills::hasSkill(m, SK_RANGE)) speed *= 1.30f;

    float dmg = pl.damage * Skills::damageMul(m) * Relic::damageDealtMul(m);
    if (Skills::hasSkill(m, SK_HUNTER) && nd > 200.f) dmg *= 1.25f;
    if (Skills::hasSkill(m, SK_DEATH_MARK) && pl.nextEnemyMarked == near->id) {
        dmg *= 1.30f; pl.nextEnemyMarked = -1;
    }
    if (Skills::hasSkill(m, SK_FINISHER) && near->hp < near->maxHp * 0.30f) dmg *= 1.35f;
    if (crit && QRandomGenerator::global()->generateDouble() < 0.18) dmg *= 1.7f;

    float a = std::atan2(near->y - pl.y, near->x - pl.x);
    pl.facing = a;
    pl.facingLeft = std::cos(a) < 0;

    float triDmg = dmg * 0.85f;
    float quadDmg = dmg * 0.75f;

    auto fire = [&](float ang, float ox, float oy, float useDmg) {
        Bullet b;
        b.x = pl.x + ox; b.y = pl.y + oy;
        b.vx = std::cos(ang)*speed; b.vy = std::sin(ang)*speed;
        b.angle = ang;
        b.damage = useDmg;
        b.pierce = prc; b.pierceLeft = Skills::pierceLevel(m);
        b.bounce = bnc;
        b.enemy = false;
        b.icy = icy; b.poison = poi; b.burn = brn; b.homing = hom; b.ghost = ghost;
        m.bullets.push_back(b);
    };

    if (quad) {
        float perp = a + M_PI/2;
        for (float off : {-18.f, -6.f, 6.f, 18.f})
            fire(a, std::cos(perp)*off, std::sin(perp)*off, quadDmg);
    } else if (tri) {
        fire(a-0.28f, 0, 0, triDmg);
        fire(a, 0, 0, triDmg);
        fire(a+0.28f, 0, 0, triDmg);
    } else if (dbl) {
        float perp = a + M_PI/2;
        fire(a, std::cos(perp)*8, std::sin(perp)*8, dmg);
        fire(a, -std::cos(perp)*8, -std::sin(perp)*8, dmg);
    } else {
        float useDmg = Skills::hasSkill(m, SK_FIRST_HIT) ? dmg * 1.5f : dmg;
        fire(a, 0, 0, useDmg);
    }
    if (dia) { fire(a + M_PI/4, 0, 0, dmg*0.85f); fire(a - M_PI/4, 0, 0, dmg*0.85f); }
    if (Skills::hasSkill(m, SK_ECHO) && QRandomGenerator::global()->generateDouble() < 0.18)
        pl.burstQueueTimer = 0.08f;
}

void throwGrenade(GameModel &m)
{
    float a = m.player.facing;
    Bullet b;
    b.x = m.player.x; b.y = m.player.y;
    b.vx = std::cos(a)*220; b.vy = std::sin(a)*220;
    b.angle = a; b.damage = 0; b.enemy = false;
    b.grenade = true; b.grenadeFuse = 1.0f; b.grenadeTotal = 1.0f;
    m.bullets.push_back(b);
}

void activateDash(GameModel &m)
{
    m.player.dashActive = 0.4f;
    m.player.dashCd = 3.f;
    for (int i=0; i<14; ++i) {
        Particle p; p.x = m.player.x + rndF(-6,6); p.y = m.player.y + rndF(-4,8);
        p.vx = rndF(-30,30); p.vy = rndF(-40,-10);
        p.color = QColor("#00ddff"); p.life=p.maxLife=0.4f; p.size=3; p.noGravity=true;
        m.particles.push_back(p);
    }
}

void activateTimeStop(GameModel &m)
{
    m.player.timeStopActive = 4.f;
    m.player.timeStopUsed = true;
    for (int i=0; i<32; ++i) {
        Particle p; p.x = m.player.x; p.y = m.player.y;
        float a = rndF(0, 6.28f);
        p.vx = std::cos(a)*120; p.vy = std::sin(a)*120;
        p.color = QColor("#7799ff"); p.life=p.maxLife=0.8f; p.size=4; p.noGravity=true;
        m.particles.push_back(p);
    }
}

void hurtPlayer(GameModel &m, float d)
{
    if (m.cheatInvincible) return;
    if (m.player.invincibility > 0 || m.player.dashActive > 0) return;
    m.player.hp = std::max(0.f, m.player.hp - d);
    float invul = 0.8f;
    if (Skills::hasSkill(m, SK_RESILIENCE)) invul *= 1.5f;
    m.player.invincibility = invul;
    m.player.anim = AN_Hurt;
    m.player.animTimer = 0; m.player.atkTimer = 0.4f;

    if (Skills::hasSkill(m, SK_RAGE_STACK)) {
        m.player.rageStacks = std::min(10, m.player.rageStacks + 1);
        m.player.rageStackTimer = 12.f;
    }
    if (Skills::hasSkill(m, SK_ADRENALINE)) m.player.adrenalineTimer = 3.f;

    for (int i=0; i<8; ++i) {
        Particle p;
        p.x = m.player.x + rndF(-10,10); p.y = m.player.y + rndF(-10,10);
        p.vx = rndF(-100,100); p.vy = rndF(-140,-40);
        p.color = Palette::C_HP; p.life = p.maxLife = rndF(0.3f,0.6f);
        p.size = (int)rndF(3,6);
        m.particles.push_back(p);
    }
}

void hurtEnemy(GameModel &m, Enemy &e, float dmg, bool fromExplosion)
{
    e.hp -= dmg; e.hitFlash = 0.18f;

    if (Skills::hasSkill(m, SK_FRZ_NERFED))   e.slowTimer = std::max(e.slowTimer, 1.0f);
    if (Skills::hasSkill(m, SK_FROST_FATE))   e.slowTimer = std::max(e.slowTimer, 1.5f);
    if (Skills::hasSkill(m, SK_FREEZE_HIT) && QRandomGenerator::global()->generateDouble()<0.12
        && e.type < ET_FinalBoss) e.slowTimer = std::max(e.slowTimer, 1.0f);
    if (Skills::hasSkill(m, SK_STICKY))       e.slowTimer = std::max(e.slowTimer, 1.0f);
    if (Skills::hasSkill(m, SK_POISON))       { e.poisonTimer = 3.0f; e.poisonDps = 4.f; }
    if (Skills::hasSkill(m, SK_BLAZE) && QRandomGenerator::global()->generateDouble() < 0.20) {
        e.burnTimer = 4.0f; e.burnDps = 3.f;
    }
    if (Skills::hasSkill(m, SK_LIFESTEAL))
        m.player.hp = std::min(m.player.maxHp, m.player.hp + dmg * 0.04f);

    const QColor *pal = (e.type==ET_Slime) ? Palette::PAL_SLIME :
                        (e.type==ET_Mage || e.type==ET_FinalBoss) ? Palette::PAL_FIRE :
                        Palette::PAL_BONE;
    bool icy = Skills::hasSkill(m, SK_FRZ_NERFED) || Skills::hasSkill(m, SK_FREEZE_HIT)
            || Skills::hasSkill(m, SK_FROST_AURA);
    for (int i=0; i<5; ++i) {
        Particle p; p.x = e.x + rndF(-8,8); p.y = e.y + rndF(-8,8);
        p.vx = rndF(-70,70); p.vy = rndF(-110,-20);
        p.color = icy ? Palette::PAL_ICE[rndI(0,1)] : pal[rndI(0,1)];
        p.life = p.maxLife = rndF(0.3f,0.6f); p.size = (int)rndF(2,5);
        m.particles.push_back(p);
    }

    if (e.hp <= 0) {
        e.dead = true; e.anim = AN_Death; e.animTimer = 0;

        // BloodOrb : +0.5 PV tous les 2 kills
        if (Relic::has(m, REL_BloodOrb)) {
            static int killAcc = 0; killAcc++;
            if (killAcc % 2 == 0) m.player.hp = std::min(m.player.maxHp, m.player.hp + 1.f);
        }

        // Score popup + streak
        Score::onKill(m);

        // Drop loot (pas pour les boss qui ont leur propre logique fragment)
        if (e.type < ET_MiniBoss) Pickup::tryRandomDrop(m, e.x, e.y, e.elite);
        if (e.elite) {
            // Drop garanti
            Pickup::spawnChest(m, e.x, e.y);
            Pickup::spawnGold(m, e.x + Core::rndF(-15,15), e.y, 20);
        }

        if ((e.type == ET_MiniBoss || e.type == ET_FinalBoss || e.type == ET_TrueFinalBoss)
            && !e.fragmentDropped) {
            e.fragmentDropped = true;
            Dialog::triggerBoss(m, e.subType, 99);
            for (int i = 0; i < 40; ++i) {
                float a = (i/40.f) * 6.28f;
                Particle pt; pt.x = e.x; pt.y = e.y;
                pt.vx = std::cos(a)*rndF(60,180);
                pt.vy = std::sin(a)*rndF(60,180) - 60;
                pt.color = (i%3==0) ? QColor("#ffd544") :
                           (i%3==1) ? QColor("#bb88ff") : QColor("#ffffff");
                pt.life = pt.maxLife = rndF(0.8f, 1.6f);
                pt.size = (int)rndF(4, 8); pt.noGravity = (i%2==0);
                m.particles.push_back(pt);
            }
        }

        m.player.killStreakCount++;
        m.player.killStreakTimer = 5.f;
        m.player.frostNovaKills++;

        if (Skills::hasSkill(m, SK_VAMP_NERFED) && QRandomGenerator::global()->generateDouble()<0.12)
            m.player.hp = std::min(m.player.maxHp, m.player.hp + 1.f);
        if (Skills::hasSkill(m, SK_SOUL_DRAIN) && m.player.regenAfterKillCd <= 0) {
            m.player.hp = std::min(m.player.maxHp, m.player.hp + 1.f);
            m.player.regenAfterKillCd = 5.f;
        }
        if (Skills::hasSkill(m, SK_DEATH_MARK)) {
            int closestId = -1; float md = 1e9f;
            for (auto &en : m.enemies) {
                if (en.dead || &en == &e) continue;
                float d = distF(en.x, en.y, e.x, e.y);
                if (d < md) { md = d; closestId = en.id; }
            }
            m.player.nextEnemyMarked = closestId;
        }
        if (Skills::hasSkill(m, SK_NECRO) && QRandomGenerator::global()->generateDouble() < 0.12) {
            // simple alli-like minion : reuse spawnMinion-like
            Enemy mn;
            mn.id = m.nextEnemyId++;
            mn.x = e.x; mn.y = e.y;
            mn.hp = mn.maxHp = 1; mn.speed = 80; mn.size = 26;
            mn.type = ET_Skel; mn.damage = 1; mn.shootCd = rndF(2.f,3.5f);
            mn.zigDir = (rndI(0,1)==0)?1.f:-1.f; mn.anim = AN_Walk;
            m.enemies.push_back(mn);
        }
        if (Skills::hasSkill(m, SK_EXPLODE_KILL) && !fromExplosion) {
            float r = 45.f * (Skills::hasSkill(m, SK_BIG_BOOM) ? 1.35f : 1.f);
            for (auto &en : m.enemies) {
                if (en.dead) continue;
                if (distF(en.x, en.y, e.x, e.y) < r) hurtEnemy(m, en, 12, true);
            }
            for (int i=0;i<14;++i) {
                Particle p; p.x = e.x; p.y = e.y;
                float ang = rndF(0,6.28f), spd = rndF(60,160);
                p.vx = std::cos(ang)*spd; p.vy = std::sin(ang)*spd-40;
                p.color = (i%2==0)?Palette::PAL_FIRE[0]:Palette::PAL_FIRE[1];
                p.life=p.maxLife=rndF(0.3f,0.7f); p.size=4;
                m.particles.push_back(p);
            }
        }
        if (e.burnTimer > 0 && Skills::hasSkill(m, SK_INFERNO)) {
            for (auto &en : m.enemies) {
                if (en.dead) continue;
                if (distF(en.x, en.y, e.x, e.y) < 50) hurtEnemy(m, en, 10, true);
            }
        }
        if (Skills::hasSkill(m, SK_VOLCANO) && QRandomGenerator::global()->generateDouble() < 0.08) {
            for (int i=0;i<6;++i) {
                Bullet b; b.x = e.x + rndF(-40,40); b.y = e.y + rndF(-50,-30);
                b.vx = 0; b.vy = 200; b.angle = M_PI/2; b.damage = 10; b.enemy = false;
                m.bullets.push_back(b);
            }
        }
        if (Skills::hasSkill(m, SK_FROST_NOVA) && m.player.frostNovaKills >= 5) {
            m.player.frostNovaKills = 0;
            for (auto &en : m.enemies)
                if (!en.dead && en.type < ET_FinalBoss)
                    en.slowTimer = std::max(en.slowTimer, 2.f);
            for (int i=0;i<24;++i) {
                Particle p; p.x = m.player.x; p.y = m.player.y;
                float ang = (i/24.f)*6.28f;
                p.vx = std::cos(ang)*180; p.vy = std::sin(ang)*180;
                p.color = Palette::PAL_ICE[rndI(0,1)];
                p.life = p.maxLife = 0.8f; p.size = 4; p.noGravity = true;
                m.particles.push_back(p);
            }
        }

        for (int i=0;i<12;++i) {
            float ang = (i/12.f)*6.28f;
            Particle p; p.x = e.x; p.y = e.y;
            p.vx = std::cos(ang)*rndF(50,130); p.vy = std::sin(ang)*rndF(50,130);
            p.color = pal[rndI(0,1)];
            p.life = p.maxLife = rndF(0.4f,0.8f);
            p.size = (int)rndF(3,6);
            m.particles.push_back(p);
        }
    } else {
        e.anim = AN_Hurt; e.animTimer = 0;
    }
}

void updateBullets(GameModel &m, float dt)
{
    for (size_t bi = 0; bi < m.bullets.size(); ++bi) {
        Bullet &b = m.bullets[bi];
        if (b.dead) continue;

        if (b.grenade) {
            float drag = 0.985f; b.vx *= drag; b.vy *= drag;
            b.x += b.vx*dt; b.y += b.vy*dt; b.grenadeFuse -= dt;
            if (b.x < TILE*1.2f || b.x > GW-TILE*1.2f) b.vx *= -0.6f;
            if (b.y < TILE*1.2f || b.y > GH-TILE*1.2f) b.vy *= -0.6f;
            b.x = clampF(b.x, TILE*1.2f, GW-TILE*1.2f);
            b.y = clampF(b.y, TILE*1.2f, GH-TILE*1.2f);

            if (b.enemy && b.grenadeTotal > 1.5f) {
                if (distF(b.x, b.y, m.player.x, m.player.y) < 30 && m.player.invincibility <= 0)
                    if ((int)(m.globalTime*4) != (int)((m.globalTime-dt)*4)) hurtPlayer(m, 1);
                if (b.grenadeFuse <= 0) b.dead = true;
                continue;
            }

            if (b.grenadeFuse <= 0) {
                float exX = b.x, exY = b.y;
                float rad = 70.f * (Skills::hasSkill(m, SK_BIG_BOOM) ? 1.35f : 1.f);
                float dmg = 50.f;
                for (auto &e : m.enemies)
                    if (!e.dead && distF(e.x, e.y, exX, exY) < rad + e.size*0.3f)
                        hurtEnemy(m, e, dmg, true);
                for (int i=0; i<32; ++i) {
                    float ang = (i/32.f)*6.28f + rndF(-0.1f, 0.1f), spd = rndF(80, 250);
                    Particle p; p.x = exX; p.y = exY;
                    p.vx = std::cos(ang)*spd; p.vy = std::sin(ang)*spd-60;
                    p.color = (i%2==0)?Palette::PAL_FIRE[0]:Palette::PAL_FIRE[1];
                    p.life = p.maxLife = rndF(0.4f, 0.9f); p.size = (int)rndF(4, 8);
                    m.particles.push_back(p);
                }
                b.dead = true;
            }
            continue;
        }

        if (b.homing && !b.enemy) {
            Enemy *near = nullptr; float nd = 1e9f;
            for (auto &e : m.enemies) {
                if (e.dead) continue;
                float d = distF(b.x, b.y, e.x, e.y);
                if (d < nd && d < 200) { nd = d; near = &e; }
            }
            if (near) {
                float ta = std::atan2(near->y - b.y, near->x - b.x);
                float ca = b.angle, diff = ta - ca;
                while (diff > M_PI) diff -= 2*M_PI;
                while (diff < -M_PI) diff += 2*M_PI;
                float turn = std::min(std::abs(diff), 3.5f * dt);
                b.angle += (diff > 0) ? turn : -turn;
                float speed = std::hypot(b.vx, b.vy);
                b.vx = std::cos(b.angle)*speed; b.vy = std::sin(b.angle)*speed;
            }
        }

        b.x += b.vx*dt; b.y += b.vy*dt;
        float marg = TILE * 1.0f;
        if (!b.ghost) {
            if (b.x < marg || b.x > GW-marg) {
                if (b.bounce > 0) { b.vx *= -1; b.bounce--; b.angle = std::atan2(b.vy, b.vx); }
                else b.dead = true;
            }
            if (b.y < marg || b.y > GH-marg) {
                if (b.bounce > 0) { b.vy *= -1; b.bounce--; b.angle = std::atan2(b.vy, b.vx); }
                else b.dead = true;
            }
        } else {
            if (b.x < -50 || b.x > GW+50 || b.y < -50 || b.y > GH+50) b.dead = true;
        }
        if (!b.dead && !b.ghost && Collision::collidesObstacle(m, b.x, b.y, 4)) b.dead = true;

        if (!b.dead && !b.enemy) {
            for (auto &e : m.enemies) {
                if (e.dead || b.hitIds.contains(e.id)) continue;
                if (distF(b.x, b.y, e.x, e.y) < 8 + e.size*0.4f) {
                    hurtEnemy(m, e, b.damage);
                    if (Skills::hasSkill(m, SK_KNOCKBACK) && !e.dead) {
                        float ka = b.angle;
                        e.x += std::cos(ka)*8; e.y += std::sin(ka)*8;
                    }
                    if (Skills::hasSkill(m, SK_ICE_SHARDS) && !b.splitChild
                        && QRandomGenerator::global()->generateDouble() < 0.20) {
                        for (int k=0; k<4; ++k) {
                            float a = (k/4.f)*6.28f;
                            Bullet sb; sb.x = b.x; sb.y = b.y;
                            sb.vx = std::cos(a)*240; sb.vy = std::sin(a)*240;
                            sb.angle = a; sb.damage = b.damage*0.4f; sb.enemy = false;
                            sb.splitChild = true; sb.icy = true; sb.hitIds.insert(e.id);
                            m.bullets.push_back(sb);
                        }
                    }
                    if (Skills::hasSkill(m, SK_SPLT_NERFED) && !b.splitChild) {
                        float perp = b.angle + M_PI/2;
                        for (int sgn=-1; sgn<=1; sgn+=2) {
                            Bullet sb; sb.x = b.x; sb.y = b.y;
                            sb.vx = std::cos(perp)*sgn*240; sb.vy = std::sin(perp)*sgn*240;
                            sb.angle = perp + (sgn<0?M_PI:0); sb.damage = b.damage*0.35f;
                            sb.splitChild = true; sb.enemy = false; sb.hitIds.insert(e.id);
                            m.bullets.push_back(sb);
                        }
                    }
                    if (!b.pierce || b.pierceLeft <= 0) { b.dead = true; }
                    else { b.pierceLeft--; b.hitIds.insert(e.id); }
                    break;
                }
            }
        }
    }
    m.bullets.erase(std::remove_if(m.bullets.begin(), m.bullets.end(),
                    [](const Bullet &b){ return b.dead; }), m.bullets.end());
}

void updateParticles(GameModel &m, float dt)
{
    for (auto &p : m.particles) {
        p.x += p.vx*dt; p.y += p.vy*dt;
        if (!p.noGravity) p.vy += 180*dt;
        p.life -= dt;
    }
    m.particles.erase(std::remove_if(m.particles.begin(), m.particles.end(),
                      [](const Particle &p){ return p.life <= 0; }), m.particles.end());
}

}}
