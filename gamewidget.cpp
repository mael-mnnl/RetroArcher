#include "gamewidget.h"
#include "core/constants.h"
#include "core/utils.h"
#include "core/palette.h"
#include "model/skillsdata.h"
#include "model/worldsdata.h"
#include "model/savemanager.h"
#include "controller/playercontroller.h"
#include "controller/enemyai.h"
#include "controller/combatsystem.h"
#include "controller/skillsystem.h"
#include "controller/roombuilder.h"
#include "controller/dialogsystem.h"
#include "controller/cheatsystem.h"
#include "controller/tutorialsystem.h"
#include <QPainter>
#include <QKeyEvent>
#include <QSettings>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

using namespace Core;
using namespace Model;
using namespace Palette;

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent),
      m_assets(),
      m_gameView(m_assets),
      m_menuView(m_assets)
{
    setFixedSize(CW * DISPLAY_SCALE, CH * DISPLAY_SCALE);
    setFocusPolicy(Qt::StrongFocus);

    m_assets.load();
    m_assets.buildSkinSprites();

    Model::loadSkills(m_model.allSkills);
    Model::loadWorlds(m_model.worlds);
    Model::loadSettings(m_model);
    m_model.player.skinIndex  = m_model.menuSelectedSkin;
    m_model.player.atkVariant = m_model.menuSelectedAtk;

    m_elapsed.start();
    connect(&m_timer, &QTimer::timeout, this, &GameWidget::tick);
    m_timer.start(16);
}

// ============================================================
//  TICK : orchestration
// ============================================================
void GameWidget::tick()
{
    qint64 now = m_elapsed.nsecsElapsed();
    float dt = std::min(0.05f, float(now - m_lastTickNs) / 1e9f);
    m_lastTickNs = now;
    m_model.globalTime += dt;

    if (m_model.state == GS_Playing || m_model.state == GS_RoomCleared) {
        updateGame(dt);
    } else if (m_model.state == GS_FadeOut) {
        m_model.fadeAmount += dt / 0.4f;
        if (m_model.fadeAmount >= 1.f) {
            m_model.fadeAmount = 1.f;
            if (m_model.currentRoom + 1 > ROOMS_TOTAL) {
                m_model.state = GS_Victory;
                if (m_model.currentRoom > m_model.highScore) m_model.highScore = m_model.currentRoom;
                m_model.worldsUnlocked = 5;
                Model::saveSettings(m_model);
            } else {
                Controller::Skills::generateSkills(m_model);
                m_model.state = GS_SkillSelect;
            }
        }
    } else if (m_model.state == GS_FadeIn) {
        m_model.fadeAmount -= dt / 0.4f;
        if (m_model.fadeAmount <= 0.f) { m_model.fadeAmount = 0.f; m_model.state = GS_Playing; }
        for (auto &p : m_model.particles) { p.x += p.vx*dt; p.y += p.vy*dt; p.life -= dt; }
    }
    update();
}

// ============================================================
//  UPDATE GAME : pure orchestration (ne touche pas au QPainter)
// ============================================================
void GameWidget::updateGame(float dt)
{
    float worldDt = dt;
    if (m_model.player.timeStopActive > 0) worldDt *= 0.25f;

    Controller::PlayerCtl::update(m_model, dt);
    Controller::EnemyAI::update(m_model, worldDt);
    Controller::Combat::updateBullets(m_model, worldDt);
    Controller::Combat::updateParticles(m_model, dt);

    if (m_model.dlgTimer > 0)        m_model.dlgTimer        -= dt;
    if (m_model.dlgFadeIn > 0)       m_model.dlgFadeIn       -= dt;
    if (m_model.cheatFlashTimer > 0) m_model.cheatFlashTimer -= dt;

    if (m_model.tutorialActive) Controller::Tutorial::update(m_model, dt);

    using namespace Controller;

    // SK_FIRE_TRAIL
    if (Skills::hasSkill(m_model, SK_FIRE_TRAIL) && m_model.player.moving
        && (int)(m_model.globalTime*8) % 2 == 0) {
        Particle p;
        p.x = m_model.player.x + rndF(-6,6); p.y = m_model.player.y + rndF(-4,8);
        p.vx = rndF(-15,15); p.vy = rndF(-30,-10);
        p.color = (rndI(0,1)==0)?PAL_FIRE[0]:PAL_FIRE[1];
        p.life = p.maxLife = 0.7f; p.size = 4; p.noGravity = true;
        m_model.particles.push_back(p);
        for (auto &e : m_model.enemies) {
            if (e.dead) continue;
            if (distF(e.x, e.y, m_model.player.x, m_model.player.y) < 30) {
                if ((int)(m_model.globalTime*5) != (int)((m_model.globalTime-dt)*5))
                    Combat::hurtEnemy(m_model, e, 4.f);
            }
        }
    }

    // SK_DECAY
    if (Skills::hasSkill(m_model, SK_DECAY)) {
        for (auto &e : m_model.enemies) {
            if (e.dead) continue;
            if (distF(e.x, e.y, m_model.player.x, m_model.player.y) < 60) {
                e.hp -= dt * 1.f;
                if (e.hp <= 0 && !e.dead) Combat::hurtEnemy(m_model, e, 0.01f);
            }
        }
    }

    // SK_REGEN / BONE_SHIELD
    if (Skills::hasSkill(m_model, SK_REGEN)) {
        m_model.player.regenTimer += dt;
        if (m_model.player.regenTimer >= 5.f) {
            m_model.player.regenTimer = 0;
            m_model.player.hp = std::min(m_model.player.maxHp, m_model.player.hp + 1.f);
        }
    }
    if (Skills::hasSkill(m_model, SK_BONE_SHIELD)) {
        m_model.player.regenTimer += dt;
        if (m_model.player.regenTimer >= 8.f) {
            m_model.player.regenTimer = 0;
            m_model.player.hp = std::min(m_model.player.maxHp, m_model.player.hp + 1.f);
        }
    }

    if (m_model.player.adrenalineTimer > 0)   m_model.player.adrenalineTimer   -= dt;
    if (m_model.player.regenAfterKillCd > 0)  m_model.player.regenAfterKillCd  -= dt;
    if (m_model.player.killStreakTimer > 0) {
        m_model.player.killStreakTimer -= dt;
        if (m_model.player.killStreakTimer <= 0) m_model.player.killStreakCount = 0;
    }
    if (m_model.player.shieldReadyTimer > 0)  m_model.player.shieldReadyTimer  -= dt;
    if (m_model.player.invulBurstTimer > 0)   m_model.player.invulBurstTimer   -= dt;
    if (m_model.player.timeStopActive > 0)    m_model.player.timeStopActive    -= dt;
    if (m_model.player.timeStopCd > 0)        m_model.player.timeStopCd        -= dt;
    if (m_model.player.dashCd > 0)            m_model.player.dashCd            -= dt;
    if (m_model.player.dashActive > 0)        m_model.player.dashActive        -= dt;
    if (m_model.player.rageStackTimer > 0) {
        m_model.player.rageStackTimer -= dt;
        if (m_model.player.rageStackTimer <= 0) m_model.player.rageStacks = 0;
    }

    // Cleanup corpses
    for (auto &e : m_model.enemies)
        if (e.dead && e.anim==AN_Death) {
            e.corpseFadeTimer += dt;
            if (e.corpseFadeTimer > 0.6f) e.corpse = true;
        }
    m_model.enemies.erase(std::remove_if(m_model.enemies.begin(), m_model.enemies.end(),
                          [](const Enemy &e){ return e.corpse; }), m_model.enemies.end());

    if (m_model.player.hp <= 0 && m_model.state != GS_GameOver) {
        if (Skills::hasSkill(m_model, SK_PHOENIX) && !m_model.player.phoenixUsed) {
            m_model.player.phoenixUsed = true;
            m_model.player.hp = m_model.player.maxHp * 0.5f;
            m_model.player.invincibility = 2.5f;
            for (int i=0; i<24; ++i) {
                float a = rndF(0, 6.28f);
                Particle p; p.x = m_model.player.x; p.y = m_model.player.y;
                p.vx = std::cos(a)*rndF(80,180); p.vy = std::sin(a)*rndF(80,180);
                p.color = QColor("#ff7733"); p.life=p.maxLife=1.f;
                p.size = 5; p.noGravity = true;
                m_model.particles.push_back(p);
            }
            return;
        }
        if (Skills::hasSkill(m_model, SK_LAST_HOPE)
            && QRandomGenerator::global()->generateDouble() < 0.5) {
            m_model.player.hp = 1; m_model.player.invincibility = 1.5f;
            return;
        }
        m_model.state = GS_GameOver;
        if (!m_model.cheatInvincible && !m_model.tutorialActive
            && m_model.currentRoom > m_model.highScore) m_model.highScore = m_model.currentRoom;
        if (!m_model.cheatInvincible && !m_model.tutorialActive) Model::saveSettings(m_model);
        return;
    }

    if (m_model.state == GS_Playing) {
        bool anyAlive = false;
        for (auto &e : m_model.enemies) if (!e.dead) { anyAlive = true; break; }
        if (!anyAlive) {
            if (m_model.tutorialActive) return;
            if (Room::isBossRoom(m_model.currentRoom)) {
                m_model.player.bossesKilled++;
                int worldCleared = Room::worldOf(m_model.currentRoom);
                if (!m_model.tutorialActive) {
                    int newFragments = std::max(m_model.fragments, worldCleared);
                    if (newFragments > m_model.fragments) m_model.fragments = newFragments;
                    m_model.worldsUnlocked = std::max(m_model.worldsUnlocked,
                                                     std::min(5, m_model.fragments + 1));
                    Model::saveSettings(m_model);
                }
                if (Room::isFinalBossRoom(m_model.currentRoom)) {
                    m_model.state = GS_Victory;
                    if (m_model.currentRoom > m_model.highScore) m_model.highScore = m_model.currentRoom;
                    if (!m_model.tutorialActive) Model::saveSettings(m_model);
                    return;
                }
            }
            m_model.state = GS_RoomCleared;
            for (int i = 0; i < 4; ++i) m_model.doorOpen[i] = true;
        }
    }
    if (m_model.state == GS_RoomCleared) checkDoorTransition();
}

void GameWidget::checkDoorTransition()
{
    int door = -1;
    if (m_model.doorOpen[0] && m_model.player.y < TILE*1.0f
        && std::abs(m_model.player.x - DOOR_COL*TILE - TILE/2.f) < TILE*0.7f) door = 0;
    else if (m_model.doorOpen[1] && m_model.player.x > GW-TILE*1.0f
        && std::abs(m_model.player.y - DOOR_ROW*TILE - TILE/2.f) < TILE*0.7f) door = 1;
    else if (m_model.doorOpen[2] && m_model.player.y > GH-TILE*1.0f
        && std::abs(m_model.player.x - DOOR_COL*TILE - TILE/2.f) < TILE*0.7f) door = 2;
    else if (m_model.doorOpen[3] && m_model.player.x < TILE*1.0f
        && std::abs(m_model.player.y - DOOR_ROW*TILE - TILE/2.f) < TILE*0.7f) door = 3;
    if (door >= 0) { m_model.exitDoor = door; m_model.state = GS_FadeOut; m_model.fadeAmount = 0; }
}

// ============================================================
//  PAINT : dispatch selon l'etat
// ============================================================
void GameWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.scale(DISPLAY_SCALE, DISPLAY_SCALE);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(0, 0, CW, CH, Qt::black);

    switch (m_model.state) {
    case GS_Menu:         m_menuView.drawMenu(p, m_model);            break;
    case GS_SkinSelect:   m_menuView.drawSkinSelect(p, m_model);      break;
    case GS_Discoveries:  m_menuView.drawDiscoveries(p, m_model);     break;
    case GS_Lore:         m_menuView.drawLore(p, m_model);            break;
    case GS_Tutorial:
    case GS_Playing:
    case GS_RoomCleared:
    case GS_FadeOut:
    case GS_FadeIn:
        m_gameView.render(p, m_model);
        if (m_model.state == GS_FadeOut || m_model.state == GS_FadeIn) {
            QColor fade(0,0,0); fade.setAlphaF(std::min(1.f, std::max(0.f, m_model.fadeAmount)));
            p.fillRect(0, 0, CW, CH, fade);
        }
        break;
    case GS_SkillSelect:
        m_gameView.render(p, m_model);
        m_menuView.drawSkillSelect(p, m_model);
        break;
    case GS_GameOver:
        m_gameView.render(p, m_model);
        m_menuView.drawEndScreen(p, m_model, false);
        break;
    case GS_Victory:
        m_gameView.render(p, m_model);
        m_menuView.drawEndScreen(p, m_model, true);
        break;
    }
}

// ============================================================
//  INPUT
// ============================================================
void GameWidget::keyPressEvent(QKeyEvent *event)
{
    int k = event->key();
    m_model.keys.insert(k);

    if (m_model.state == GS_Menu) {
        if (m_model.cheatFocused) {
            if (k == Qt::Key_Return || k == Qt::Key_Enter) {
                Controller::Cheat::tryValidate(m_model);
                m_model.cheatFocused = false;
            } else if (k == Qt::Key_Escape) {
                m_model.cheatFocused = false; m_model.cheatInput.clear();
            } else if (k == Qt::Key_Backspace) {
                if (!m_model.cheatInput.isEmpty()) m_model.cheatInput.chop(1);
            } else if (m_model.cheatInput.size() < 10) {
                QString t = event->text();
                for (QChar ch : t) if (ch.isLetterOrNumber()) m_model.cheatInput.append(ch);
            }
            return;
        }
        if (k==Qt::Key_Space || k==Qt::Key_Return) Controller::Room::startGame(m_model);
        else if (k==Qt::Key_C) m_model.state = GS_SkinSelect;
        else if (k==Qt::Key_D) { m_model.state = GS_Discoveries; m_model.discoveryHover = 0; }
        else if (k==Qt::Key_T) Controller::Tutorial::start(m_model);
        else if (k==Qt::Key_L) m_model.state = GS_Lore;
        else if (k==Qt::Key_X) m_model.cheatFocused = true;
    } else if (m_model.state == GS_Lore) {
        if (k==Qt::Key_Escape || k==Qt::Key_Return) m_model.state = GS_Menu;
    } else if (m_model.state == GS_SkinSelect) {
        if (k==Qt::Key_Left || k==Qt::Key_A) m_model.menuSelectedSkin = (m_model.menuSelectedSkin+3)%4;
        else if (k==Qt::Key_Right || k==Qt::Key_D) m_model.menuSelectedSkin = (m_model.menuSelectedSkin+1)%4;
        else if (k==Qt::Key_Up || k==Qt::Key_W) m_model.menuSelectedAtk = (m_model.menuSelectedAtk+2)%3;
        else if (k==Qt::Key_Down || k==Qt::Key_S) m_model.menuSelectedAtk = (m_model.menuSelectedAtk+1)%3;
        else if (k==Qt::Key_Escape || k==Qt::Key_Return || k==Qt::Key_Space) {
            QSettings s("RetroArcher","RetroArcher");
            s.setValue("skin", m_model.menuSelectedSkin);
            s.setValue("atk",  m_model.menuSelectedAtk);
            m_model.state = GS_Menu;
        }
    } else if (m_model.state == GS_Discoveries) {
        if (k==Qt::Key_Escape || k==Qt::Key_Return) m_model.state = GS_Menu;
        else if (k==Qt::Key_Left  || k==Qt::Key_A) m_model.discoveryHover = (m_model.discoveryHover+4)%5;
        else if (k==Qt::Key_Right || k==Qt::Key_D) m_model.discoveryHover = (m_model.discoveryHover+1)%5;
    } else if (m_model.state == GS_GameOver || m_model.state == GS_Victory) {
        if (k==Qt::Key_Space) Controller::Room::startGame(m_model);
        else if (k==Qt::Key_Escape) m_model.state = GS_Menu;
    } else if (m_model.state == GS_SkillSelect) {
        int idx = -1;
        if (k==Qt::Key_1) idx = 0; else if (k==Qt::Key_2) idx = 1; else if (k==Qt::Key_3) idx = 2;
        if (idx >= 0 && idx < (int)m_model.skillChoices.size()) {
            int chosen = m_model.skillChoices[idx];
            // Callback : enchaîne sur la salle suivante
            Controller::Skills::applySkill(m_model, chosen, [](Model::GameModel &mm){
                int entryDoor = (mm.exitDoor + 2) % 4;
                Controller::Room::buildRoom(mm, mm.currentRoom + 1, entryDoor);
                mm.state = GS_FadeIn;
                mm.fadeAmount = 1.f;
            });
        }
    }
}

void GameWidget::keyReleaseEvent(QKeyEvent *event) { m_model.keys.remove(event->key()); }
void GameWidget::mousePressEvent(QMouseEvent *) {}
