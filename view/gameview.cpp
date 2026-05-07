#include "gameview.h"
#include "assetmanager.h"
#include "../model/gamemodel.h"
#include "../core/constants.h"
#include "../core/palette.h"
#include "../controller/skillsystem.h"
#include "../controller/roombuilder.h"
#include <QPainter>
#include <QFontMetrics>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace View {

using namespace Model;
using namespace Core;
using namespace Palette;

GameView::GameView(const AssetManager &assets) : m_assets(assets) {}

void GameView::drawSpriteAt(QPainter &p, const QImage &sheet, int frameCount, float fps,
                            float x, float y, float scale, bool flipX, float timer)
{
    if (sheet.isNull() || frameCount <= 0) return;
    int frame = std::max(0, std::min(frameCount-1, (int)(timer*fps) % frameCount));
    if (frame < 0) frame = 0;
    int fw = sheet.width()/frameCount, fh = sheet.height();
    float dw = fw*scale, dh = fh*scale;
    float dx = x - dw/2.f, dy = y - dh*0.85f;
    p.save();
    if (flipX) {
        p.translate(int(x), 0); p.scale(-1, 1);
        p.drawImage(QRectF(-dw/2.f, dy, dw, dh), sheet, QRectF(frame*fw, 0, fw, fh));
    } else {
        p.drawImage(QRectF(dx, dy, dw, dh), sheet, QRectF(frame*fw, 0, fw, fh));
    }
    p.restore();
}

void GameView::drawBossInGame(QPainter &p, const GameModel &m, const Enemy &boss, float scaleMul)
{
    int w = boss.subType;
    if (w < 0 || w >= m.worlds.size()) return;
    const WorldInfo &wi = m.worlds[w];
    QImage sheet = m_assets.bossSheets[wi.bossKey];
    if (sheet.isNull()) return;
    int fc = wi.bossFrameCount;
    int fw = wi.bossFrameW, fh = wi.bossFrameH;
    int frame = (int)(boss.animTimer * wi.bossFps) % fc;
    float scale = (boss.size / 60.f) * 0.85f * scaleMul;
    float dw = fw*scale, dh = fh*scale;
    float dx = boss.x - dw/2.f, dy = boss.y - dh*0.6f;
    p.save();
    if (boss.facingLeft) {
        p.translate(int(boss.x), 0); p.scale(-1, 1);
        p.drawImage(QRectF(-dw/2.f, dy, dw, dh), sheet, QRectF(frame*fw, 0, fw, fh));
    } else {
        p.drawImage(QRectF(dx, dy, dw, dh), sheet, QRectF(frame*fw, 0, fw, fh));
    }
    p.restore();
}

void GameView::drawRoom(QPainter &p, const GameModel &m)
{
    for (int r=1; r<ROWS-1; ++r) for (int c=1; c<COLS-1; ++c) {
        int x = c*TILE, y = r*TILE;
        p.fillRect(x, y, TILE, TILE, ((r+c)%2==0)?C_FL1:C_FL2);
    }
    for (int c=0; c<COLS; ++c) {
        p.fillRect(c*TILE, 0, TILE, TILE, C_WT);
        p.fillRect(c*TILE, TILE-8, TILE, 8, C_WF);
        p.fillRect(c*TILE, (ROWS-1)*TILE, TILE, TILE, C_WT);
        p.fillRect(c*TILE, (ROWS-1)*TILE, TILE, 8, C_WF);
    }
    for (int r=0; r<ROWS; ++r) {
        p.fillRect(0, r*TILE, TILE, TILE, C_WT);
        p.fillRect(TILE-8, r*TILE, 8, TILE, C_WF);
        p.fillRect((COLS-1)*TILE, r*TILE, TILE, TILE, C_WT);
        p.fillRect((COLS-1)*TILE, r*TILE, 8, TILE, C_WF);
    }
    auto drawDoor = [&](int col, int row, bool open) {
        int x = col*TILE, y = row*TILE;
        p.fillRect(x+4, y+4, TILE-8, TILE-8, open?C_DO:C_DC);
        if (open) {
            p.fillRect(x+6, y+6, TILE-12, TILE-12, QColor(20,10,5));
            QColor glow("#ffd544");
            int alpha = 120 + (int)(std::sin(m.globalTime*4) * 40);
            glow.setAlpha(std::max(0, std::min(255, alpha)));
            p.setPen(QPen(glow, 1)); p.setBrush(Qt::NoBrush);
            p.drawRect(x+4, y+4, TILE-9, TILE-9); p.setPen(Qt::NoPen);
        }
    };
    drawDoor(DOOR_COL, 0, m.doorOpen[0]);
    drawDoor(COLS-1, DOOR_ROW, m.doorOpen[1]);
    drawDoor(DOOR_COL, ROWS-1, m.doorOpen[2]);
    drawDoor(0, DOOR_ROW, m.doorOpen[3]);
}

void GameView::drawProgressionBar(QPainter &p, const GameModel &m, int x, int y, int w)
{
    int h = 8;
    p.fillRect(QRectF(x, y, w, h), QColor(20, 15, 10));
    p.setPen(QPen(C_GOLD, 1)); p.setBrush(Qt::NoBrush);
    p.drawRect(x, y, w, h); p.setPen(Qt::NoPen);
    float per = float(w) / ROOMS_TOTAL;
    for (int i = 1; i <= ROOMS_TOTAL; ++i) {
        float rx = x + (i-1)*per;
        QColor col = (i < m.currentRoom) ? QColor(40, 110, 40) :
                     (i == m.currentRoom) ? QColor(220, 180, 40) : QColor(60, 50, 40);
        p.fillRect(QRectF(rx+0.5f, y+1, per-1, h-2), col);
    }
    auto drawMarker = [&](int room, bool isFinal) {
        float cx = x + (room-0.5f)*per, cy = y + h/2.f;
        if (isFinal) {
            p.setBrush(QColor("#ff2222")); p.setPen(QPen(QColor("#ffaa44"), 1));
            float s = 7; QPolygonF star;
            for (int i = 0; i < 8; ++i) {
                float ang = i*M_PI/4.f - M_PI/2.f;
                float rr = (i%2==0) ? s : s*0.5f;
                star << QPointF(cx + std::cos(ang)*rr, cy + std::sin(ang)*rr);
            }
            p.drawPolygon(star);
        } else {
            int wIdx = (room/5) - 1;
            QColor mc = (wIdx >= 0 && wIdx < m.worlds.size()) ? m.worlds[wIdx].accent : QColor("#cc2244");
            p.setBrush(mc); p.setPen(QPen(QColor("#ffcc44"), 1));
            p.drawEllipse(QPointF(cx, cy), 5, 5);
        }
        p.setPen(Qt::NoPen); p.setBrush(Qt::NoBrush);
    };
    drawMarker(5, false); drawMarker(10, false); drawMarker(15, false);
    drawMarker(20, false); drawMarker(25, true);
}

void GameView::drawHUD(QPainter &p, const GameModel &m)
{
    p.fillRect(0, GH, GW, HUD, C_HUD);
    p.setPen(QPen(C_HB, 2)); p.drawRect(0, GH, GW, HUD-1); p.setPen(Qt::NoPen);
    QFont font("monospace", 9, QFont::Bold);
    p.setFont(font); p.setPen(C_GOLD); p.drawText(8, GH+18, "PV");
    int hpDisplay = std::min((int)m.player.maxHp, 8);
    for (int i = 0; i < hpDisplay; ++i) {
        bool full = i < m.player.hp;
        p.fillRect(QRectF(28+i*15, GH+8, 12, 12), full?C_HP:C_HPB);
        if (full) p.fillRect(QRectF(30+i*15, GH+10, 4, 3), QColor("#ff6688"));
    }
    if ((int)m.player.maxHp > 8) {
        p.setFont(QFont("monospace", 7));
        p.setPen(C_GOLD);
        p.drawText(28+8*15+4, GH+18, QString("+%1").arg((int)m.player.maxHp - 8));
    }
    if (m.player.grenadeAmmo > 0 || Controller::Skills::hasSkill(m, SK_GRN)) {
        QFont gf("monospace", 7, QFont::Bold);
        p.setFont(gf); p.setPen(QColor("#ff8844"));
        p.drawText(8, GH+33, QString("[G]x%1").arg(m.player.grenadeAmmo));
    }
    if (Controller::Skills::hasSkill(m, SK_DASH)) {
        QFont gf("monospace", 7, QFont::Bold);
        p.setFont(gf);
        p.setPen(m.player.dashCd <= 0 ? QColor("#00ddff") : QColor("#557799"));
        p.drawText(60, GH+33, QString("[SHIFT]%1").arg(m.player.dashCd > 0 ? QString::number(m.player.dashCd, 'f', 1) : "OK"));
    }
    if (Controller::Skills::hasSkill(m, SK_TIME_STOP) && !m.player.timeStopUsed) {
        QFont gf("monospace", 7, QFont::Bold);
        p.setFont(gf); p.setPen(QColor("#7799ff"));
        p.drawText(140, GH+33, "[T]");
    }
    // Boss bar
    const Enemy *boss = nullptr;
    for (auto &e : m.enemies) if (e.type==ET_MiniBoss || e.type==ET_FinalBoss) { boss = &e; break; }
    if (boss && !boss->dead) {
        float bw = 240, bh = 11;
        float bx = GW/2.f - bw/2.f, by = GH+10;
        p.fillRect(QRectF(bx, by, bw, bh), C_HPB);
        QColor bc = (boss->type == ET_FinalBoss) ?
            ((boss->phase==4) ? QColor("#ff00ff") :
             (boss->phase==3) ? QColor("#ff00aa") :
             (boss->phase==2) ? QColor("#ff5500") : C_BE) :
            ((boss->phase == 2) ? QColor("#ff5500") : QColor("#cc3344"));
        p.fillRect(QRectF(bx, by, bw * boss->hp/boss->maxHp, bh), bc);
        p.setPen(QPen(C_GOLD, 1)); p.drawRect(QRectF(bx, by, bw, bh)); p.setPen(Qt::NoPen);
        QFont sf("monospace", 7, QFont::Bold); p.setFont(sf); p.setPen(C_GOLD);
        int wIdx = boss->subType;
        QString name = (wIdx>=0 && wIdx<m.worlds.size()) ? m.worlds[wIdx].bossNameFr : "BOSS";
        QString label = (boss->type == ET_FinalBoss) ?
            QString("%1 - PHASE %2").arg(name).arg(boss->phase) :
            ((boss->phase==2) ? QString("%1 - FUREUR").arg(name) : name);
        QFontMetrics fm(sf);
        p.drawText(GW/2 - fm.horizontalAdvance(label)/2, by-2, label);
    }
    drawProgressionBar(p, m, 8, GH+38, GW-16);
    QFont sf("monospace", 7, QFont::Bold); p.setFont(sf); p.setPen(C_GOLD);
    int worldNum = Controller::Room::worldOf(m.currentRoom);
    QString roomTxt = QString("M%1 SALLE %2/%3").arg(worldNum).arg(m.currentRoom).arg(ROOMS_TOTAL);
    if (m.state == GS_RoomCleared) roomTxt += " > PORTE";
    p.drawText(8, GH+56, roomTxt);
    QFont skf("monospace", 6, QFont::Bold); p.setFont(skf);
    QFontMetrics skfm(skf);
    int sx = GW - 8;
    for (int i = (int)m.player.skills.size()-1; i >= 0; --i) {
        const Skill *sk = nullptr;
        for (auto &s : m.allSkills) if (s.id == m.player.skills[i]) { sk = &s; break; }
        if (!sk) continue;
        int tw = skfm.horizontalAdvance(sk->icon) + 6;
        sx -= tw + 2;
        if (sx < 200) break;
        QColor bgc = sk->color; bgc.setAlpha(60);
        p.fillRect(QRectF(sx, GH+56, tw, 8), bgc);
        p.setPen(sk->color); p.drawText(sx+2, GH+62, sk->icon); p.setPen(Qt::NoPen);
    }
}

void GameView::drawDialogBubble(QPainter &p, const GameModel &m)
{
    if (m.dlgTimer <= 0) return;
    QFont nf("monospace", 8, QFont::Bold);
    QFont tf("monospace", 8);
    QFontMetrics nfm(nf), tfm(tf);

    QStringList words = m.dlgText.split(' ');
    QStringList lines;
    QString cur;
    int maxW = GW - 60;
    for (auto &w : words) {
        QString test = cur.isEmpty() ? w : cur + " " + w;
        if (tfm.horizontalAdvance(test) > maxW) {
            lines << cur; cur = w;
        } else cur = test;
    }
    if (!cur.isEmpty()) lines << cur;

    int boxH = 18 + (int)lines.size() * 12 + 4;
    int boxW = maxW + 20;
    int x = GW/2 - boxW/2, y = 14;

    float fadeIn = std::min(1.f, std::max(0.f, 1.f - m.dlgFadeIn / 0.25f));
    float fadeOut = std::min(1.f, m.dlgTimer / 0.4f);
    float alpha = std::min(fadeIn, fadeOut);

    QColor bg(15, 8, 22); bg.setAlphaF(0.92f * alpha);
    p.fillRect(QRectF(x, y, boxW, boxH), bg);
    QColor border = m.dlgColor; border.setAlphaF(alpha);
    p.setPen(QPen(border, 2)); p.setBrush(Qt::NoBrush);
    p.drawRect(x, y, boxW, boxH); p.setPen(Qt::NoPen);

    p.setFont(nf);
    QColor sp = m.dlgColor; sp.setAlphaF(alpha);
    p.setPen(sp);
    p.drawText(x + 8, y + 13, m.dlgSpeaker);

    p.setFont(tf);
    QColor tc("#eeddff"); tc.setAlphaF(alpha);
    p.setPen(tc);
    int ly = y + 26;
    for (auto &line : lines) { p.drawText(x + 8, ly, line); ly += 12; }
    p.setPen(Qt::NoPen);
}

void GameView::drawTutorialOverlay(QPainter &p, const GameModel &m)
{
    if (!m.tutorialActive) return;
    QFont sf("monospace", 7, QFont::Bold); p.setFont(sf); QFontMetrics sfm(sf);
    QString tag = QString("TUTORIEL  -  Etape %1/4").arg(std::min(4, m.tutStep+1));
    int tw = sfm.horizontalAdvance(tag);
    QColor bg(0,0,0,180); p.fillRect(QRectF(GW-tw-12, 4, tw+8, 12), bg);
    p.setPen(QColor("#ffd544"));
    p.drawText(GW-tw-8, 13, tag);
    p.setPen(Qt::NoPen);
}

void GameView::render(QPainter &p, const GameModel &m)
{
    drawRoom(p, m);

    for (auto &pa : m.particles) {
        float a = std::max(0.f, pa.life/pa.maxLife);
        QColor c = pa.color; c.setAlphaF(a);
        p.fillRect(QRectF(pa.x - pa.size/2.f, pa.y - pa.size/2.f, pa.size, pa.size), c);
    }

    for (auto &e : m.enemies) {
        if (e.type == ET_MiniBoss || e.type == ET_FinalBoss) {
            drawBossInGame(p, m, e, 1.f);
            if (e.hitFlash > 0 && !e.dead) {
                p.save();
                p.setCompositionMode(QPainter::CompositionMode_Plus);
                p.setOpacity(std::min(0.7f, e.hitFlash * 4));
                drawBossInGame(p, m, e, 1.f);
                p.restore();
            }
            if (!e.dead && e.hp < e.maxHp) {
                float bw = e.size * 1.5f, bh = 5;
                float bx = e.x - bw/2.f, by = e.y - e.size*0.9f - 18;
                p.fillRect(QRectF(bx, by, bw, bh), C_HPB);
                QColor hpCol = (e.type == ET_FinalBoss) ? C_BE : QColor("#cc4422");
                p.fillRect(QRectF(bx, by, bw * e.hp/e.maxHp, bh), hpCol);
            }
            continue;
        }
        int variant = -1;
        if      (e.type==ET_Slime)   variant = 0;
        else if (e.type==ET_Skel)    variant = 1;
        else if (e.type==ET_Bat)     variant = 2;
        else if (e.type==ET_Brute)   variant = 3;
        else if (e.type==ET_Mage)    variant = 4;
        else if (e.type==ET_Minion)  variant = 1;
        if (variant < 0) continue;

        AnimType anim = e.anim;
        const QImage &sheet = m_assets.sprOrc[variant][anim];
        float scale = 1.f;
        if      (e.type==ET_Slime)  scale = 1.05f;
        else if (e.type==ET_Skel)   scale = 1.10f;
        else if (e.type==ET_Bat)    scale = 0.95f;
        else if (e.type==ET_Brute)  scale = 1.30f;
        else if (e.type==ET_Mage)   scale = 1.10f;
        else if (e.type==ET_Minion) scale = 0.85f;
        int fc = 6; float fps = 8.f;
        switch (anim) {
            case AN_Idle:  fc = AssetManager::s_idleFrameCount; fps = 8.f; break;
            case AN_Walk:  fc = AssetManager::s_walkFrameCount; fps = 12.f; break;
            case AN_Atk:   fc = 6; fps = 14.f; break;
            case AN_Hurt:  fc = AssetManager::s_hurtFrameCount; fps = 10.f; break;
            case AN_Death: fc = AssetManager::s_deathFrameCount; fps = 7.f; break;
        }
        float fadeAlpha = 1.f;
        if (e.dead && e.corpseFadeTimer > 0)
            fadeAlpha = std::max(0.f, 1.f - e.corpseFadeTimer/0.6f);
        if (fadeAlpha < 1.f) p.setOpacity(fadeAlpha);
        drawSpriteAt(p, sheet, fc, fps, e.x, e.y, scale, e.facingLeft, e.animTimer);
        if (fadeAlpha < 1.f) p.setOpacity(1.f);
        if (e.slowTimer > 0 && !e.dead) {
            for (int i=0; i<3; ++i) {
                float ph = m.globalTime*2 + i*2.f;
                float fx = e.x + std::cos(ph)*e.size*0.5f;
                float fy = e.y - e.size*0.5f + std::sin(ph*1.3f)*6;
                p.fillRect(QRectF(fx-1.5f, fy-1.5f, 3, 3), QColor("#aaddff"));
            }
        }
        if (e.hitFlash > 0 && !e.dead) {
            p.save();
            p.setCompositionMode(QPainter::CompositionMode_Plus);
            p.setOpacity(std::min(0.7f, e.hitFlash * 4));
            drawSpriteAt(p, sheet, fc, fps, e.x, e.y, scale, e.facingLeft, e.animTimer);
            p.restore();
        }
        if (!e.dead && e.hp < e.maxHp) {
            float bw = e.size * 1.2f, bh = 4;
            float bx = e.x - bw/2.f, by = e.y - e.size/2.f - 14;
            p.fillRect(QRectF(bx, by, bw, bh), C_HPB);
            p.fillRect(QRectF(bx, by, bw * e.hp/e.maxHp, bh), C_HP);
        }
    }

    for (auto &b : m.bullets) {
        if (b.dead) continue;
        if (b.grenade) {
            float t = (b.grenadeTotal > 0) ? (b.grenadeTotal - b.grenadeFuse)/b.grenadeTotal : 0;
            float pulse = 0.5f + 0.5f*std::sin(t*30);
            int sz = 7 + int(pulse*3);
            QColor base = b.enemy ? QColor("#aa3300") : QColor(60,50,40);
            p.fillRect(QRectF(b.x-sz/2.f, b.y-sz/2.f, sz, sz), base);
            QColor red(255, 60+int(pulse*80), 0);
            int isz = 4 + int(pulse*2);
            p.fillRect(QRectF(b.x-isz/2.f, b.y-isz/2.f, isz, isz), red);
        } else if (b.enemy) {
            p.fillRect(QRectF(b.x-5, b.y-5, 10, 10), C_BEG);
            p.fillRect(QRectF(b.x-3, b.y-3, 6, 6), C_BE);
            p.fillRect(QRectF(b.x-1, b.y-1, 2, 2), Qt::white);
        } else {
            p.save();
            p.translate(b.x, b.y);
            p.rotate(b.angle*180.f/M_PI - 90);
            float w = m_assets.sprArrow.width(), h = m_assets.sprArrow.height();
            if (b.icy) p.setOpacity(0.8);
            p.drawImage(QRectF(-w/2, -h/2, w, h), m_assets.sprArrow);
            p.setOpacity(1);
            p.restore();
            if (b.poison || b.burn || b.icy) {
                QColor tc = b.poison ? PAL_POISON[0] : (b.burn ? PAL_FIRE[0] : PAL_ICE[0]);
                tc.setAlphaF(0.6f);
                p.fillRect(QRectF(b.x-2, b.y-2, 4, 4), tc);
            }
        }
    }

    if (!(m.player.invincibility > 0 && (int)(m.player.invincibility*28) % 2 == 0)) {
        const QImage &sheet = m_assets.soldierSheet(m.player.anim, m.player.skinIndex, m.player.atkVariant);
        drawSpriteAt(p, sheet,
                     m_assets.soldierFrameCount(m.player.anim, m.player.atkVariant),
                     m_assets.soldierFps(m.player.anim, m.player.atkVariant),
                     m.player.x, m.player.y, 1.3f, m.player.facingLeft, m.player.animTimer);
        if (m.player.dashActive > 0) {
            p.setOpacity(0.4f);
            drawSpriteAt(p, sheet,
                         m_assets.soldierFrameCount(m.player.anim, m.player.atkVariant),
                         m_assets.soldierFps(m.player.anim, m.player.atkVariant),
                         m.player.x - std::cos(m.player.facing)*8,
                         m.player.y - std::sin(m.player.facing)*8,
                         1.3f, m.player.facingLeft, m.player.animTimer);
            p.setOpacity(1);
        }
    }

    drawHUD(p, m);
    drawDialogBubble(p, m);
    drawTutorialOverlay(p, m);
}

}
