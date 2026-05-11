#include "gamewidget.h"
#include "core/constants.h"
#include "core/utils.h"
#include "core/palette.h"
#include "model/skillsdata.h"
#include "model/worldsdata.h"
#include "model/contentdata.h"
#include "model/savemanager.h"
#include "controller/playercontroller.h"
#include "controller/enemyai.h"
#include "controller/combatsystem.h"
#include "controller/skillsystem.h"
#include "controller/roombuilder.h"
#include "controller/dialogsystem.h"
#include "controller/cheatsystem.h"
#include "controller/tutorialsystem.h"
#include "controller/cursesystem.h"
#include "controller/relicsystem.h"
#include "controller/pickupsystem.h"
#include "controller/scoresystem.h"
#include "controller/biomefxsystem.h"
#include "controller/levelgen.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
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
    setMouseTracking(true);

    m_assets.load();
    m_assets.buildSkinSprites();

    Model::loadSkills(m_model.allSkills);
    Model::loadWorlds(m_model.worlds);
    Model::loadCurses(m_model);
    Model::loadRelics(m_model);
    Model::loadClasses(m_model);
    Model::loadSettings(m_model);
    m_model.player.skinIndex  = m_model.menuSelectedSkin;
    m_model.player.atkVariant = m_model.menuSelectedAtk;

    m_elapsed.start();
    connect(&m_timer, &QTimer::timeout, this, &GameWidget::tick);
    m_timer.start(16);
}

void GameWidget::launchSelection()
{
    // Quand le joueur a choisi classe + relique, on demarre la partie
    Controller::Room::startGame(m_model);
}

void GameWidget::onBossDefeated()
{
    if (m_model.tutorialActive) return;
    m_model.player.bossesKilled++;
    m_model.bossesKilledThisRun++;
    int worldCleared = Controller::Room::worldOf(m_model.currentRoom);
    int newFragments = std::max(m_model.fragments, worldCleared);
    if (newFragments > m_model.fragments) {
        m_model.fragments = newFragments;
        m_model.blessingPointsAvail = std::max(0, m_model.fragments - m_model.blessingPointsSpent);
    }
    m_model.worldsUnlocked = std::max(m_model.worldsUnlocked, std::min(6, m_model.fragments + 1));
    if (m_model.fragments >= 5) m_model.malificusUnlocked = true;
    Model::saveSettings(m_model);
}

void GameWidget::tick()
{
    qint64 now = m_elapsed.nsecsElapsed();
    float dt = std::min(0.05f, float(now - m_lastTickNs) / 1e9f);
    m_lastTickNs = now;
    m_model.globalTime += dt;

    // Biome FX et fx effects tournent dès qu'on est en jeu
    if (m_model.state == GS_Playing || m_model.state == GS_RoomCleared
        || m_model.state == GS_Shop || m_model.state == GS_Forge
        || m_model.state == GS_Challenge) {
        Controller::BiomeFX::update(m_model, dt);
        Controller::BiomeFX::updateFx(m_model, dt);
        Controller::Pickup::update(m_model, dt);
        Controller::Score::update(m_model, dt);
    }

    if (m_model.state == GS_Playing || m_model.state == GS_RoomCleared) {
        updateGame(dt);
    } else if (m_model.state == GS_FadeOut) {
        m_model.fadeAmount += dt / 0.4f;
        if (m_model.fadeAmount >= 1.f) {
            m_model.fadeAmount = 1.f;
            int maxRoom = m_model.malificusUnlocked ? 30 : ROOMS_TOTAL;
            if (m_model.currentRoom + 1 > maxRoom) {
                m_model.state = GS_Victory;
                if (m_model.currentRoom > m_model.highScore) m_model.highScore = m_model.currentRoom;
                m_model.worldsUnlocked = m_model.malificusUnlocked ? 6 : 5;
                Controller::Score::addToLeaderboard(m_model);
                Model::saveSettings(m_model);
            } else {
                // Apres un boss : proposer aussi une malediction
                bool wasBoss = Controller::Room::isBossRoom(m_model.currentRoom);
                if (wasBoss && m_model.currentRoom > 5) {
                    Controller::Curse::generateChoices(m_model);
                    if (!m_model.curseChoices.empty()) {
                        m_model.state = GS_CurseSelect;
                        return;
                    }
                }
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

    if (Skills::hasSkill(m_model, SK_DECAY)) {
        for (auto &e : m_model.enemies) {
            if (e.dead) continue;
            if (distF(e.x, e.y, m_model.player.x, m_model.player.y) < 60) {
                e.hp -= dt * 1.f;
                if (e.hp <= 0 && !e.dead) Combat::hurtEnemy(m_model, e, 0.01f);
            }
        }
    }

    if (Skills::hasSkill(m_model, SK_REGEN) && !Curse::active(m_model, CRS_NoRegen)) {
        m_model.player.regenTimer += dt;
        if (m_model.player.regenTimer >= 5.f) {
            m_model.player.regenTimer = 0;
            m_model.player.hp = std::min(m_model.player.maxHp, m_model.player.hp + 1.f);
        }
    }
    if (Skills::hasSkill(m_model, SK_BONE_SHIELD) && !Curse::active(m_model, CRS_NoRegen)) {
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

    // Track damage taken in challenge room
    if (m_model.player.inChallenge) {
        // (le no-hit est detecte par hurtPlayer - on traque via le hp)
        if (m_model.player.invincibility > 0.5f && m_model.challengeStarted)
            m_model.challengePassed = false;
        m_model.challengeStarted = true;
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
        // Phoenix Tear relic
        if (m_model.selectedRelicId == REL_PhoenixTear && m_model.runHasFreeRevive) {
            m_model.runHasFreeRevive = false;
            m_model.player.hp = m_model.player.maxHp;
            m_model.player.invincibility = 3.f;
            return;
        }
        if (Skills::hasSkill(m_model, SK_PHOENIX) && !m_model.player.phoenixUsed) {
            m_model.player.phoenixUsed = true;
            m_model.player.hp = m_model.player.maxHp * 0.5f;
            m_model.player.invincibility = 2.5f;
            return;
        }
        if (Skills::hasSkill(m_model, SK_LAST_HOPE)
            && QRandomGenerator::global()->generateDouble() < 0.5) {
            m_model.player.hp = 1; m_model.player.invincibility = 1.5f;
            return;
        }
        // BloodOrb : perd 1 skill aleatoire
        if (m_model.selectedRelicId == REL_BloodOrb && !m_model.player.skills.empty()) {
            int idx = rndI(0, (int)m_model.player.skills.size()-1);
            m_model.player.skills.erase(m_model.player.skills.begin() + idx);
        }
        m_model.state = GS_GameOver;
        if (!m_model.cheatInvincible && !m_model.tutorialActive
            && m_model.currentRoom > m_model.highScore) m_model.highScore = m_model.currentRoom;
        if (!m_model.cheatInvincible && !m_model.tutorialActive) {
            Score::addToLeaderboard(m_model);
            Model::saveSettings(m_model);
        }
        return;
    }

    if (m_model.state == GS_Playing) {
        bool anyAlive = false;
        for (auto &e : m_model.enemies) if (!e.dead) { anyAlive = true; break; }
        if (!anyAlive) {
            if (m_model.tutorialActive) return;
            if (Room::isBossRoom(m_model.currentRoom)) {
                onBossDefeated();
                int maxRoom = m_model.malificusUnlocked ? 30 : ROOMS_TOTAL;
                if (m_model.currentRoom == maxRoom) {
                    m_model.state = GS_Victory;
                    if (m_model.currentRoom > m_model.highScore) m_model.highScore = m_model.currentRoom;
                    m_model.ascensionUnlocked = true;
                    Score::addToLeaderboard(m_model);
                    Model::saveSettings(m_model);
                    return;
                }
            }
            // Challenge réussi : récompense
            if (m_model.player.inChallenge && m_model.challengePassed) {
                Pickup::spawnChest(m_model, m_model.player.x + 30, m_model.player.y);
                Pickup::spawnGold (m_model, m_model.player.x - 30, m_model.player.y, 30);
                Score::popup(m_model, m_model.player.x, m_model.player.y - 30,
                             "DEFI REUSSI !", QColor("#88ff88"));
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
    if (door >= 0) {
        m_model.exitDoor = door;
        m_model.state = GS_FadeOut;
        m_model.fadeAmount = 0;
        // Couleur de fade selon biome
        int wi = std::min(6, ((m_model.currentRoom-1)/5)+1) - 1;
        if (wi >= 0 && wi < m_model.worlds.size())
            m_model.fadeColor = m_model.worlds[wi].accent;
        else m_model.fadeColor = QColor(0,0,0);
    }
}

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
    case GS_Blessings:    m_menuView.drawBlessings(p, m_model, m_blessingHover); break;
    case GS_Leaderboard:  m_menuView.drawLeaderboard(p, m_model);     break;
    case GS_ClassSelect:  m_menuView.drawClassSelect(p, m_model, m_classHover);  break;
    case GS_RelicSelect:  m_menuView.drawRelicSelect(p, m_model, m_relicHover);  break;
    case GS_Tutorial:
    case GS_Playing:
    case GS_RoomCleared:
    case GS_FadeOut:
    case GS_FadeIn:
        m_gameView.render(p, m_model);
        if (m_model.state == GS_FadeOut || m_model.state == GS_FadeIn) {
            QColor fade = m_model.fadeColor;
            fade.setAlphaF(std::min(1.f, std::max(0.f, m_model.fadeAmount)));
            p.fillRect(0, 0, CW, CH, fade);
        }
        break;
    case GS_Shop:
        m_gameView.render(p, m_model);
        m_menuView.drawShop(p, m_model, m_shopHover);
        break;
    case GS_Forge:
        m_gameView.render(p, m_model);
        m_menuView.drawForge(p, m_model, m_forgeHover);
        break;
    case GS_Challenge:
        m_gameView.render(p, m_model);
        break;
    case GS_SkillSelect:
        m_gameView.render(p, m_model);
        m_menuView.drawSkillSelect(p, m_model);
        break;
    case GS_CurseSelect:
        m_gameView.render(p, m_model);
        m_menuView.drawCurseSelect(p, m_model);
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
        if (k==Qt::Key_Space || k==Qt::Key_Return) {
            // Lancer aventure : class select -> relic select -> jeu
            m_classHover = 0; m_relicHover = 0;
            m_model.state = GS_ClassSelect;
        }
        else if (k==Qt::Key_C) m_model.state = GS_SkinSelect;
        else if (k==Qt::Key_D) { m_model.state = GS_Discoveries; m_model.discoveryHover = 0; }
        else if (k==Qt::Key_T) Controller::Tutorial::start(m_model);
        else if (k==Qt::Key_L) m_model.state = GS_Lore;
        else if (k==Qt::Key_B) { m_model.state = GS_Blessings; m_blessingHover = 0; }
        else if (k==Qt::Key_K) m_model.state = GS_Leaderboard;
        else if (k==Qt::Key_A && m_model.ascensionUnlocked) {
            m_model.ascensionEnabled = !m_model.ascensionEnabled;
        }
        else if (k==Qt::Key_X) m_model.cheatFocused = true;
    } else if (m_model.state == GS_ClassSelect) {
        int n = (int)m_model.allClasses.size();
        if (k==Qt::Key_Left || k==Qt::Key_A) m_classHover = (m_classHover+n-1)%n;
        else if (k==Qt::Key_Right || k==Qt::Key_D) m_classHover = (m_classHover+1)%n;
        else if (k==Qt::Key_Space || k==Qt::Key_Return) {
            m_model.selectedClassId = m_classHover;
            Controller::Relic::generateChoices(m_model);
            m_model.state = GS_RelicSelect;
        }
        else if (k==Qt::Key_Escape) m_model.state = GS_Menu;
        else if (k>=Qt::Key_1 && k<=Qt::Key_4) {
            m_classHover = std::min(n-1, k - Qt::Key_1);
        }
    } else if (m_model.state == GS_RelicSelect) {
        if (k==Qt::Key_Escape) m_model.state = GS_ClassSelect;
        else if (k==Qt::Key_0) {
            m_model.selectedRelicId = -1;
            launchSelection();
        }
        else if (k>=Qt::Key_1 && k<=Qt::Key_3) {
            int idx = k - Qt::Key_1;
            if (idx < (int)m_model.relicChoices.size()) {
                m_model.selectedRelicId = m_model.relicChoices[idx];
                launchSelection();
            }
        }
    } else if (m_model.state == GS_Blessings) {
        if (k==Qt::Key_Escape) m_model.state = GS_Menu;
        else if (k==Qt::Key_Up || k==Qt::Key_W) m_blessingHover = (m_blessingHover+2)%3;
        else if (k==Qt::Key_Down || k==Qt::Key_S) m_blessingHover = (m_blessingHover+1)%3;
        else if (k==Qt::Key_Space || k==Qt::Key_Return) {
            int costs[3] = {1, 1, 2};
            if (m_blessingHover == 2 && m_model.blessingFreeSkill) return;
            if (m_model.blessingPointsAvail >= costs[m_blessingHover]) {
                m_model.blessingPointsAvail -= costs[m_blessingHover];
                m_model.blessingPointsSpent += costs[m_blessingHover];
                if (m_blessingHover == 0) m_model.blessingMaxHpBonus++;
                else if (m_blessingHover == 1) m_model.blessingStartGold += 25;
                else m_model.blessingFreeSkill = true;
                Model::saveSettings(m_model);
            }
        }
    } else if (m_model.state == GS_Leaderboard) {
        if (k==Qt::Key_Escape || k==Qt::Key_Return) m_model.state = GS_Menu;
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
        else if (k==Qt::Key_Left  || k==Qt::Key_A) m_model.discoveryHover = (m_model.discoveryHover+5)%6;
        else if (k==Qt::Key_Right || k==Qt::Key_D) m_model.discoveryHover = (m_model.discoveryHover+1)%6;
    } else if (m_model.state == GS_GameOver || m_model.state == GS_Victory) {
        if (k==Qt::Key_Space) { m_model.state = GS_Menu; }
        else if (k==Qt::Key_Escape) m_model.state = GS_Menu;
    } else if (m_model.state == GS_SkillSelect) {
        int idx = -1;
        if (k==Qt::Key_1) idx = 0; else if (k==Qt::Key_2) idx = 1; else if (k==Qt::Key_3) idx = 2;
        if (idx >= 0 && idx < (int)m_model.skillChoices.size()) {
            int chosen = m_model.skillChoices[idx];
            Controller::Skills::applySkill(m_model, chosen, [](Model::GameModel &mm){
                int entryDoor = (mm.exitDoor + 2) % 4;
                Controller::Room::buildRoom(mm, mm.currentRoom + 1, entryDoor);
                // Si nouvelle salle est shop/forge, le state est mis directement par buildRoom
                if (mm.state != GS_RoomCleared) {
                    mm.state = GS_FadeIn;
                    mm.fadeAmount = 1.f;
                }
            });
        }
    } else if (m_model.state == GS_CurseSelect) {
        int idx = -1;
        if (k==Qt::Key_1) idx = 0; else if (k==Qt::Key_2) idx = 1;
        if (idx >= 0 && idx < (int)m_model.curseChoices.size()) {
            Controller::Curse::apply(m_model, m_model.curseChoices[idx]);
            Controller::Skills::generateSkills(m_model);
            m_model.state = GS_SkillSelect;
        }
    } else if (m_model.state == GS_Shop) {
        int idx = -1;
        if (k==Qt::Key_1) idx = 0; else if (k==Qt::Key_2) idx = 1; else if (k==Qt::Key_3) idx = 2;
        if (idx >= 0 && idx < (int)m_model.shopItems.size()) {
            if (m_model.player.gold >= m_model.shopPrices[idx]) {
                m_model.player.gold -= m_model.shopPrices[idx];
                m_model.player.skills.push_back(m_model.shopItems[idx]);
                m_model.shopItems.erase(m_model.shopItems.begin() + idx);
                m_model.shopPrices.erase(m_model.shopPrices.begin() + idx);
            }
        } else if (k==Qt::Key_Escape) {
            m_model.state = GS_Playing;  // sortir du shop = chercher porte
        }
    } else if (m_model.state == GS_Forge) {
        int n = (int)m_model.player.skills.size();
        if (n > 0) {
            if (k==Qt::Key_Left || k==Qt::Key_A) m_forgeHover = (m_forgeHover+n-1)%n;
            else if (k==Qt::Key_Right || k==Qt::Key_D) m_forgeHover = (m_forgeHover+1)%n;
            else if (k==Qt::Key_Up || k==Qt::Key_W) { m_forgeHover = std::max(0, m_forgeHover-6); }
            else if (k==Qt::Key_Down || k==Qt::Key_S) { m_forgeHover = std::min(n-1, m_forgeHover+6); }
            else if (k==Qt::Key_Space || k==Qt::Key_Return) {
                int oldId = m_model.player.skills[m_forgeHover];
                int oldTier = 1;
                for (auto &s : m_model.allSkills) if (s.id == oldId) { oldTier = s.worldTier; break; }
                int newTier = std::min(m_model.worldsUnlocked, oldTier + 1);
                // Choisir un nouveau skill du tier sup
                std::vector<int> pool;
                for (auto &s : m_model.allSkills)
                    if (s.worldTier == newTier && s.id != oldId) pool.push_back(s.id);
                if (!pool.empty()) {
                    int newId = pool[rndI(0, (int)pool.size()-1)];
                    m_model.player.skills[m_forgeHover] = newId;
                    m_forgeHover = 0;
                }
            }
        }
        if (k==Qt::Key_Escape) m_model.state = GS_Playing;
    }
}

void GameWidget::keyReleaseEvent(QKeyEvent *event) { m_model.keys.remove(event->key()); }

void GameWidget::mousePressEvent(QMouseEvent *e) {
    // Dash directionnel : memorise la cible
    if (e->button() == Qt::RightButton
        && (m_model.state == GS_Playing || m_model.state == GS_RoomCleared)) {
        float mx = e->pos().x() / float(DISPLAY_SCALE);
        float my = e->pos().y() / float(DISPLAY_SCALE);
        m_model.player.dashAimX = mx;
        m_model.player.dashAimY = my;
        m_model.player.hasDashAim = true;
    }
    // Click sur Shop/Forge/Blessings
    if (e->button() == Qt::LeftButton) {
        // En entree de RoomCleared dans Shop / Forge, on declenche le menu special
        if (m_model.state == GS_RoomCleared && m_model.roomType == Model::RT_Shop)
            m_model.state = GS_Shop;
        else if (m_model.state == GS_RoomCleared && m_model.roomType == Model::RT_Forge)
            m_model.state = GS_Forge;
    }
}

void GameWidget::mouseMoveEvent(QMouseEvent *) {}
