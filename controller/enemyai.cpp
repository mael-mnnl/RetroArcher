#include "enemyai.h"
#include "combatsystem.h"
#include "collisionsystem.h"
#include "dialogsystem.h"
#include "roombuilder.h"
#include "../model/gamemodel.h"
#include "../model/types.h"
#include "../core/utils.h"
#include "../core/constants.h"
#include "../core/palette.h"
#include <QRandomGenerator>
#include <cmath>
#include <algorithm>

namespace Controller { namespace EnemyAI {

using namespace Model;
using namespace Core;

void update(GameModel &m, float dt)
{
    int origSize = (int)m.enemies.size();
    for (int idx = 0; idx < origSize; ++idx) {
        Enemy &e = m.enemies[idx];
        if (e.hitFlash > 0) e.hitFlash -= dt;
        if (e.slowTimer > 0) e.slowTimer -= dt;
        if (e.dead) { e.animTimer += dt; continue; }

        float adt = dt * (e.slowTimer > 0 ? 0.5f : 1.f);
        e.animTimer += adt;

        if (e.poisonTimer > 0) {
            e.poisonTimer -= dt;
            e.hp -= e.poisonDps * dt;
            if (e.hp <= 0) { Combat::hurtEnemy(m, e, 0.01f); continue; }
        }
        if (e.burnTimer > 0) {
            e.burnTimer -= dt;
            e.hp -= e.burnDps * dt;
            if (e.hp <= 0) { Combat::hurtEnemy(m, e, 0.01f); continue; }
        }

        float dx = m.player.x - e.x, dy = m.player.y - e.y;
        float d = std::hypot(dx, dy);
        float collR = e.size * 0.4f;

        if (e.type == ET_Slime) {
            if (d > 0) {
                Collision::tryMove(m, e.x, e.y, dx/d * e.speed * adt, dy/d * e.speed * adt, collR);
                e.facingLeft = dx < 0;
            }
            if (e.anim==AN_Hurt && e.animTimer>0.4f) { e.anim=AN_Walk; e.animTimer=0; }
            else if (e.anim!=AN_Hurt) e.anim = AN_Walk;
        } else if (e.type == ET_Skel || e.type == ET_Minion) {
            e.aimAngle = std::atan2(dy, dx); e.facingLeft = dx<0;
            if (d > 0) {
                float ds = 0;
                if (d < 150) ds = -0.4f; else if (d > 220) ds = 1.f;
                if (ds != 0) Collision::tryMove(m, e.x, e.y, dx/d*e.speed*ds*adt, dy/d*e.speed*ds*adt, collR);
            }
            e.shootCd -= adt;
            if (e.shootCd <= 0 && d < 320 && e.type != ET_Minion) {
                e.shootCd = rndF(1.8f, 2.8f);
                Bullet b; b.x = e.x; b.y = e.y;
                b.vx = std::cos(e.aimAngle)*175; b.vy = std::sin(e.aimAngle)*175;
                b.angle = e.aimAngle; b.damage = 1; b.enemy = true;
                m.bullets.push_back(b);
            }
            if (e.anim==AN_Hurt && e.animTimer>0.4f) { e.anim=AN_Walk; e.animTimer=0; }
            else if (e.anim!=AN_Hurt) e.anim = (d>5)?AN_Walk:AN_Idle;
        } else if (e.type == ET_Bat) {
            e.zigTimer -= adt;
            if (e.zigTimer <= 0) { e.zigDir *= -1; e.zigTimer = rndF(0.3f, 0.7f); }
            if (d > 0) {
                float px = -dy/d, py = dx/d;
                Collision::tryMove(m, e.x, e.y,
                    (dx/d*e.speed + px*35*e.zigDir)*adt,
                    (dy/d*e.speed + py*35*e.zigDir)*adt, collR);
                e.facingLeft = dx < 0;
            }
            if (e.anim==AN_Hurt && e.animTimer>0.4f) { e.anim=AN_Walk; e.animTimer=0; }
            else if (e.anim!=AN_Hurt) e.anim = AN_Walk;
        } else if (e.type == ET_Brute) {
            if (d > 0) {
                Collision::tryMove(m, e.x, e.y, dx/d*e.speed*adt, dy/d*e.speed*adt, collR);
                e.facingLeft = dx < 0;
            }
            if (e.anim==AN_Hurt && e.animTimer>0.4f) { e.anim=AN_Walk; e.animTimer=0; }
            else if (e.anim!=AN_Hurt) e.anim = AN_Walk;
        } else if (e.type == ET_Mage) {
            e.aimAngle = std::atan2(dy, dx); e.facingLeft = dx<0;
            if (d > 0) {
                float ds = 0;
                if (d < 200) ds = -0.5f; else if (d > 280) ds = 0.7f;
                if (ds != 0) Collision::tryMove(m, e.x, e.y, dx/d*e.speed*ds*adt, dy/d*e.speed*ds*adt, collR);
            }
            e.shootCd -= adt;
            if (e.shootCd <= 0 && d < 380) {
                e.shootCd = rndF(2.0f, 3.0f);
                for (float off : {-0.3f, 0.f, 0.3f}) {
                    Bullet b; b.x = e.x; b.y = e.y;
                    float a = e.aimAngle + off;
                    b.vx = std::cos(a)*160; b.vy = std::sin(a)*160;
                    b.angle = a; b.damage = 1; b.enemy = true;
                    m.bullets.push_back(b);
                }
            }
            if (e.anim==AN_Hurt && e.animTimer>0.4f) { e.anim=AN_Walk; e.animTimer=0; }
            else if (e.anim!=AN_Hurt) e.anim = (d>5)?AN_Walk:AN_Idle;
        }
        else if (e.type == ET_MiniBoss || e.type == ET_FinalBoss) {
            if (e.type == ET_FinalBoss) {
                int oldPhase = e.phase;
                if (e.hp < e.maxHp*0.75f && e.phase==1) e.phase=2;
                if (e.hp < e.maxHp*0.50f && e.phase==2) e.phase=3;
                if (e.hp < e.maxHp*0.25f && e.phase==3) e.phase=4;
                if (e.phase != oldPhase) Dialog::triggerBoss(m, e.subType, e.phase);
            } else {
                int oldPhase = e.phase;
                if (e.hp < e.maxHp*0.5f && e.phase==1) e.phase=2;
                if (e.phase != oldPhase) Dialog::triggerBoss(m, e.subType, 1);
            }

            e.moveCd -= adt;
            if (e.moveCd <= 0 && !e.charging) {
                e.moveCd = rndF(0.7f, 1.5f);
                float pad = TILE * 2.5f;
                e.moveTargetX = rndF(pad, GW-pad);
                e.moveTargetY = rndF(pad, GH-pad);
            }

            if (e.subType == 0) {
                // Ver de Feu : trail + occasional charge
                if (e.charging) {
                    e.x += e.chargeVx * adt; e.y += e.chargeVy * adt;
                    e.chargeT -= adt;
                    if (e.chargeT <= 0) e.charging = false;
                    if (e.x < TILE*1.5f || e.x > GW-TILE*1.5f) e.charging=false;
                    if (e.y < TILE*1.5f || e.y > GH-TILE*1.5f) e.charging=false;
                } else {
                    float mdx = e.moveTargetX-e.x, mdy = e.moveTargetY-e.y, md = std::hypot(mdx,mdy);
                    if (md > 5) Collision::tryMove(m, e.x, e.y, mdx/md*e.speed*adt, mdy/md*e.speed*adt, collR);
                    e.facingLeft = mdx < 0;
                }
                e.chargeCd -= adt;
                if (e.chargeCd <= 0 && d < 350 && !e.charging) {
                    e.chargeCd = (e.phase==2) ? 3.f : 5.f;
                    e.charging = true; e.chargeT = 1.0f;
                    float ca = std::atan2(dy, dx);
                    float cspd = (e.phase==2) ? 280.f : 220.f;
                    e.chargeVx = std::cos(ca)*cspd; e.chargeVy = std::sin(ca)*cspd;
                }
                e.trailCd -= adt;
                if (e.trailCd <= 0) {
                    e.trailCd = (e.phase==2) ? 0.4f : 0.7f;
                    Bullet pool; pool.x = e.x; pool.y = e.y; pool.vx = 0; pool.vy = 0;
                    pool.angle = 0; pool.damage = 0;
                    pool.enemy = true; pool.grenade = true;
                    pool.grenadeFuse = (e.phase==2) ? 3.f : 2.5f;
                    pool.grenadeTotal = pool.grenadeFuse;
                    m.bullets.push_back(pool);
                }
                e.shootCd -= adt;
                if (e.shootCd <= 0) {
                    e.shootCd = (e.phase==2) ? 1.0f : 1.4f;
                    float a = std::atan2(dy, dx);
                    for (float off : {-0.2f, 0.f, 0.2f}) {
                        Bullet b; b.x = e.x; b.y = e.y;
                        b.vx = std::cos(a+off)*180; b.vy = std::sin(a+off)*180;
                        b.angle = a+off; b.damage = e.damage; b.enemy = true;
                        m.bullets.push_back(b);
                    }
                }
            } else if (e.subType == 1) {
                // Executeur : invocation
                float mdx = e.moveTargetX-e.x, mdy = e.moveTargetY-e.y, md = std::hypot(mdx,mdy);
                if (md > 5) Collision::tryMove(m, e.x, e.y, mdx/md*e.speed*adt, mdy/md*e.speed*adt, collR);
                e.facingLeft = mdx < 0;
                e.summonCd -= adt;
                if (e.summonCd <= 0) {
                    e.summonCd = (e.phase==2) ? 4.f : 7.f;
                    int n = (e.phase==2) ? 4 : 2;
                    for (int i=0; i<n; ++i) {
                        float ang = (i/float(n))*6.28f + rndF(-0.2f, 0.2f);
                        float sx = e.x + std::cos(ang)*40, sy = e.y + std::sin(ang)*40;
                        Room::spawnMinion(m, sx, sy, 25, 70, ET_Skel);
                    }
                    for (int i=0; i<16; ++i) {
                        Particle p; p.x = e.x; p.y = e.y;
                        float a = (i/16.f)*6.28f;
                        p.vx = std::cos(a)*100; p.vy = std::sin(a)*100;
                        p.color = QColor("#aabbcc"); p.life=p.maxLife=0.7f; p.size=4; p.noGravity=true;
                        m.particles.push_back(p);
                    }
                }
                e.shootCd -= adt;
                if (e.shootCd <= 0) {
                    e.shootCd = (e.phase==2) ? 0.9f : 1.3f;
                    float a = std::atan2(dy, dx);
                    Bullet b; b.x = e.x; b.y = e.y;
                    b.vx = std::cos(a)*180; b.vy = std::sin(a)*180;
                    b.angle = a; b.damage = e.damage; b.enemy = true;
                    m.bullets.push_back(b);
                }
                e.burstCd -= adt;
                if (e.burstCd <= 0) {
                    e.burstCd = (e.phase==2) ? 4.f : 6.f;
                    for (int i=0; i<10; ++i) {
                        float a = (i/10.f)*6.28f;
                        Bullet b; b.x = e.x; b.y = e.y;
                        b.vx = std::cos(a)*120; b.vy = std::sin(a)*120;
                        b.angle = a; b.damage = 1; b.enemy = true;
                        m.bullets.push_back(b);
                    }
                }
            } else if (e.subType == 2) {
                // Demon Vase : teleport
                e.specialCd -= adt;
                if (e.specialCd <= 0) {
                    e.specialCd = (e.phase==2) ? 2.5f : 4.f;
                    for (int i=0; i<14; ++i) {
                        Particle p; p.x = e.x; p.y = e.y;
                        float a = rndF(0,6.28f);
                        p.vx = std::cos(a)*120; p.vy = std::sin(a)*120;
                        p.color = QColor("#aa44ff"); p.life=p.maxLife=0.5f; p.size=4; p.noGravity=true;
                        m.particles.push_back(p);
                    }
                    e.x = m.player.x + rndF(-160,160); e.y = m.player.y + rndF(-100,100);
                    e.x = clampF(e.x, TILE*2, GW-TILE*2);
                    e.y = clampF(e.y, TILE*2, GH-TILE*2);
                    for (int i=0; i<14; ++i) {
                        Particle p; p.x = e.x; p.y = e.y;
                        float a = rndF(0,6.28f);
                        p.vx = std::cos(a)*60; p.vy = std::sin(a)*60;
                        p.color = QColor("#dd66ff"); p.life=p.maxLife=0.4f; p.size=3; p.noGravity=true;
                        m.particles.push_back(p);
                    }
                }
                e.shootCd -= adt;
                if (e.shootCd <= 0) {
                    e.shootCd = (e.phase==2) ? 0.7f : 1.f;
                    float a = std::atan2(dy, dx);
                    int n = (e.phase==2) ? 5 : 3;
                    for (int i=0; i<n; ++i) {
                        float off = (i-(n-1)/2.f) * 0.20f;
                        Bullet b; b.x = e.x; b.y = e.y;
                        b.vx = std::cos(a+off)*200; b.vy = std::sin(a+off)*200;
                        b.angle = a+off; b.damage = e.damage; b.enemy = true;
                        m.bullets.push_back(b);
                    }
                }
                if (e.phase == 2) {
                    e.burstCd -= adt;
                    if (e.burstCd <= 0) {
                        e.burstCd = 4.f;
                        Room::spawnMinion(m, e.x+30, e.y, 20, 80, ET_Slime);
                        Room::spawnMinion(m, e.x-30, e.y, 20, 80, ET_Slime);
                    }
                }
            } else if (e.subType == 3) {
                // Minotaure : charges en croix
                if (e.charging) {
                    e.x += e.chargeVx * adt; e.y += e.chargeVy * adt;
                    e.chargeT -= adt;
                    if (e.chargeT <= 0 || e.x < TILE*1.5f || e.x > GW-TILE*1.5f
                        || e.y < TILE*1.5f || e.y > GH-TILE*1.5f) {
                        e.charging = false;
                        for (int i=0; i<16; ++i) {
                            Particle p; p.x = e.x; p.y = e.y;
                            float a = (i/16.f)*6.28f;
                            p.vx = std::cos(a)*200; p.vy = std::sin(a)*200;
                            p.color = QColor("#cc8822"); p.life=p.maxLife=0.6f; p.size=5;
                            m.particles.push_back(p);
                        }
                    }
                } else {
                    float mdx = e.moveTargetX-e.x, mdy = e.moveTargetY-e.y, md = std::hypot(mdx,mdy);
                    if (md > 5) Collision::tryMove(m, e.x, e.y, mdx/md*e.speed*adt, mdy/md*e.speed*adt, collR);
                    e.facingLeft = mdx < 0;
                }
                e.chargeCd -= adt;
                if (e.chargeCd <= 0 && !e.charging) {
                    e.chargeCd = (e.phase==2) ? 1.8f : 3.f;
                    e.charging = true; e.chargeT = (e.phase==2) ? 1.0f : 1.2f;
                    float ca = std::atan2(dy, dx);
                    float cspd = (e.phase==2) ? 480.f : 360.f;
                    e.chargeVx = std::cos(ca)*cspd; e.chargeVy = std::sin(ca)*cspd;
                    e.facingLeft = std::cos(ca) < 0;
                }
                if (e.phase == 2) {
                    e.burstCd -= adt;
                    if (e.burstCd <= 0) {
                        e.burstCd = 5.f;
                        for (int dir=0; dir<4; ++dir) {
                            float a = dir * (M_PI/2);
                            for (int i=0; i<6; ++i) {
                                Bullet b; b.x = e.x; b.y = e.y;
                                b.vx = std::cos(a)*(120+i*40); b.vy = std::sin(a)*(120+i*40);
                                b.angle = a; b.damage = 1; b.enemy = true;
                                m.bullets.push_back(b);
                            }
                        }
                    }
                }
            } else if (e.subType == 4) {
                // Gardien du Givre - 4 phases
                if (e.phase >= 2) {
                    e.specialCd -= adt;
                    if (e.specialCd <= 0) {
                        e.specialCd = (e.phase==4) ? 1.5f : (e.phase==3 ? 2.f : 3.f);
                        for (int i=0; i<14; ++i) {
                            Particle p; p.x=e.x; p.y=e.y;
                            float a=rndF(0,6.28f);
                            p.vx = std::cos(a)*120; p.vy = std::sin(a)*120;
                            p.color = Palette::PAL_ICE[rndI(0,1)];
                            p.life=p.maxLife=0.5f; p.size=4; p.noGravity=true;
                            m.particles.push_back(p);
                        }
                        e.x = clampF(m.player.x+rndF(-180,180), TILE*2, GW-TILE*2);
                        e.y = clampF(m.player.y+rndF(-110,110), TILE*2, GH-TILE*2);
                        for (int i=0; i<14; ++i) {
                            Particle p; p.x=e.x; p.y=e.y;
                            float a=rndF(0,6.28f);
                            p.vx = std::cos(a)*60; p.vy = std::sin(a)*60;
                            p.color = Palette::PAL_ICE[rndI(0,1)];
                            p.life=p.maxLife=0.3f; p.size=3; p.noGravity=true;
                            m.particles.push_back(p);
                        }
                    }
                } else {
                    float mdx = e.moveTargetX-e.x, mdy = e.moveTargetY-e.y, md = std::hypot(mdx,mdy);
                    if (md > 5) Collision::tryMove(m, e.x, e.y, mdx/md*e.speed*adt, mdy/md*e.speed*adt, collR);
                    e.facingLeft = mdx < 0;
                }

                e.shootCd -= adt;
                if (e.shootCd <= 0) {
                    e.shootCd = (e.phase==4) ? 0.30f
                              : (e.phase==3 ? 0.45f
                              : (e.phase==2 ? 0.6f : 0.8f));
                    float a = std::atan2(dy, dx);
                    int n = e.phase + 1;
                    for (int i=0; i<n; ++i) {
                        float off = (n>1) ? (i-(n-1)/2.f)*0.18f : 0;
                        Bullet b; b.x = e.x; b.y = e.y;
                        b.vx = std::cos(a+off)*220; b.vy = std::sin(a+off)*220;
                        b.angle = a+off; b.damage = e.damage; b.enemy = true;
                        m.bullets.push_back(b);
                    }
                }

                if (e.phase >= 3) {
                    e.burstCd -= adt;
                    if (e.burstCd <= 0) {
                        e.burstCd = (e.phase==4) ? 2.0f : 3.0f;
                        int rays = (e.phase==4) ? 20 : 14;
                        float baseAng = m.globalTime * 0.7f;
                        for (int i=0; i<rays; ++i) {
                            float a = baseAng + (i/float(rays))*6.28f;
                            Bullet b; b.x = e.x; b.y = e.y;
                            b.vx = std::cos(a)*180; b.vy = std::sin(a)*180;
                            b.angle = a; b.damage = 1; b.enemy = true;
                            m.bullets.push_back(b);
                        }
                    }
                }
                if (e.phase >= 2) {
                    e.summonCd -= adt;
                    if (e.summonCd <= 0) {
                        e.summonCd = (e.phase==4) ? 4.f : (e.phase==3 ? 5.f : 7.f);
                        int n = (e.phase==4) ? 3 : 2;
                        for (int i=0; i<n; ++i) {
                            float ang = (i/float(n))*6.28f + rndF(-0.3f, 0.3f);
                            float sx = e.x + std::cos(ang)*60, sy = e.y + std::sin(ang)*60;
                            sx = clampF(sx, TILE*2, GW-TILE*2);
                            sy = clampF(sy, TILE*2, GH-TILE*2);
                            Room::spawnMinion(m, sx, sy, 30, 100, ET_Bat);
                        }
                    }
                }
                if (e.phase == 4) {
                    e.chargeCd -= adt;
                    if (e.chargeCd <= 0 && !e.charging) {
                        e.chargeCd = 3.5f;
                        e.charging = true; e.chargeT = 0.8f;
                        float ca = std::atan2(dy, dx);
                        e.chargeVx = std::cos(ca)*450; e.chargeVy = std::sin(ca)*450;
                    }
                    if (e.charging) {
                        e.x += e.chargeVx * adt; e.y += e.chargeVy * adt;
                        e.chargeT -= adt;
                        if (e.chargeT <= 0) e.charging = false;
                        e.x = clampF(e.x, TILE*1.5f, GW-TILE*1.5f);
                        e.y = clampF(e.y, TILE*1.5f, GH-TILE*1.5f);
                    }
                }
            }
            if (e.anim==AN_Hurt && e.animTimer>0.4f) { e.anim=AN_Walk; e.animTimer=0; }
            else if (e.anim!=AN_Hurt) e.anim = AN_Walk;
        }
    }
}

}}
