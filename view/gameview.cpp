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
    int frame = (fc > 0) ? ((int)(boss.animTimer * wi.bossFps) % fc) : 0;
    float scale = (boss.size / 60.f) * 0.85f * scaleMul;
    // Lord Malificus : sprite 64x96 unique, plus gros
    if (boss.type == ET_TrueFinalBoss) scale = (boss.size / 64.f) * 1.05f * scaleMul;
    float dw = fw*scale, dh = fh*scale;
    float dx = boss.x - dw/2.f, dy = boss.y - dh*0.6f;
    // Aura violette pour Malificus
    if (boss.type == ET_TrueFinalBoss && !boss.dead) {
        QColor aura(180, 60, 220, 70 + (int)(std::sin(m.globalTime*3)*30));
        p.fillRect(QRectF(dx-4, dy-4, dw+8, dh+8), aura);
    }
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
    // Couleurs de sol selon biome
    int wi = std::min(6, ((m.currentRoom-1)/5)+1) - 1;
    QColor fl1, fl2, wt, wf;
    switch (wi) {
        case 0: fl1=QColor("#3a2008"); fl2=QColor("#2a1606"); wt=QColor("#4d1808"); wf=QColor("#260a04"); break; // Forge
        case 1: fl1=QColor("#2a2640"); fl2=QColor("#1f1d30"); wt=QColor("#3a3858"); wf=QColor("#1e1c30"); break; // Catacombes
        case 2: fl1=QColor("#1a3a22"); fl2=QColor("#143018"); wt=QColor("#23502c"); wf=QColor("#0e2614"); break; // Marais
        case 3: fl1=QColor("#4a3a1a"); fl2=QColor("#3a2c10"); wt=QColor("#553e1a"); wf=QColor("#2a1d08"); break; // Arene
        case 4: fl1=QColor("#1c3a4a"); fl2=QColor("#142a38"); wt=QColor("#2c4a5c"); wf=QColor("#0f2030"); break; // Citadelle
        default:fl1=QColor("#2a1230"); fl2=QColor("#1a081c"); wt=QColor("#3a1a40"); wf=QColor("#1a0a22"); break; // Abysses
    }
    // Sol damier + petits motifs aléatoires (semi-procédural stable par room)
    quint32 seed = (quint32)(m.currentRoom * 1009);
    for (int r=1; r<ROWS-1; ++r) for (int c=1; c<COLS-1; ++c) {
        int x = c*TILE, y = r*TILE;
        p.fillRect(x, y, TILE, TILE, ((r+c)%2==0)?fl1:fl2);
        // Motif rare : crack/fissure
        if (((c*7919 + r*1031 + seed) % 31) == 5) {
            QColor accent = fl1.lighter(115);
            p.fillRect(QRectF(x+8, y+12, 4, 1), accent);
            p.fillRect(QRectF(x+12, y+12, 1, 6), accent);
            p.fillRect(QRectF(x+12, y+18, 5, 1), accent);
        }
    }
    // Murs périphériques
    for (int c=0; c<COLS; ++c) {
        p.fillRect(c*TILE, 0, TILE, TILE, wt);
        p.fillRect(c*TILE, TILE-8, TILE, 8, wf);
        p.fillRect(c*TILE, (ROWS-1)*TILE, TILE, TILE, wt);
        p.fillRect(c*TILE, (ROWS-1)*TILE, TILE, 8, wf);
    }
    for (int r=0; r<ROWS; ++r) {
        p.fillRect(0, r*TILE, TILE, TILE, wt);
        p.fillRect(TILE-8, r*TILE, 8, TILE, wf);
        p.fillRect((COLS-1)*TILE, r*TILE, TILE, TILE, wt);
        p.fillRect((COLS-1)*TILE, r*TILE, 8, TILE, wf);
    }
    // Obstacles intérieurs
    for (auto &o : m.obstacles) {
        int x = o.col*TILE, y = o.row*TILE;
        if (o.kind == 0) { // pilier
            p.fillRect(x+4, y+4, TILE-8, TILE-8, wt);
            p.fillRect(x+6, y+6, TILE-12, TILE-12, wt.darker(130));
            p.fillRect(x+8, y+8, 4, TILE-16, wt.lighter(125));
        } else if (o.kind == 1) { // caisse
            p.fillRect(x+5, y+6, TILE-10, TILE-10, QColor("#6a4a2a"));
            p.fillRect(x+5, y+6, TILE-10, 2, QColor("#8a6a3a"));
            p.fillRect(x+5, y+6, 2, TILE-10, QColor("#8a6a3a"));
            p.setPen(QPen(QColor("#3a2010"), 1)); p.setBrush(Qt::NoBrush);
            p.drawRect(x+5, y+6, TILE-10, TILE-10); p.setPen(Qt::NoPen);
        } else { // brasero animé
            p.fillRect(x+10, y+12, TILE-20, TILE-16, QColor("#3a2010"));
            float ph = std::sin(m.globalTime*6 + o.col*0.7f + o.row*0.5f);
            int sz = 6 + int(ph*3);
            p.fillRect(QRectF(x+TILE/2-sz/2.f, y+6, sz, sz),
                       (wi==4) ? QColor("#88ccff") : QColor("#ff6622"));
            p.fillRect(QRectF(x+TILE/2-sz/4.f, y+8, sz/2, sz/2),
                       (wi==4) ? QColor("#ddeeff") : QColor("#ffee88"));
        }
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

    // Indicateur de salle spéciale (label discret en haut)
    if (m.roomType != RT_Normal && m.roomType != RT_Boss) {
        QFont rf("monospace", 7, QFont::Bold);
        p.setFont(rf); QFontMetrics rfm(rf);
        QString label;
        QColor lcol = QColor("#ffd544");
        switch (m.roomType) {
            case RT_Elite:     label="* SALLE D'ELITE *"; lcol=QColor("#ff6644"); break;
            case RT_Shop:      label="MARCHAND"; lcol=QColor("#44ddaa"); break;
            case RT_Forge:     label="FORGE"; lcol=QColor("#ffaa44"); break;
            case RT_Challenge: label="DEFI - SANS DEGAT"; lcol=QColor("#bb88ff"); break;
            default: break;
        }
        if (!label.isEmpty()) {
            int lw = rfm.horizontalAdvance(label);
            QColor bg(0,0,0,180); p.fillRect(QRectF(GW/2-lw/2-4, TILE-4, lw+8, 12), bg);
            p.setPen(lcol); p.drawText(GW/2-lw/2, TILE+5, label); p.setPen(Qt::NoPen);
        }
    }
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
    // Bouclier (barre bleue sous les PV)
    if (m.player.shieldMax > 0) {
        int shW = std::min(8, m.player.shieldMax) * 15;
        p.setFont(QFont("monospace", 9, QFont::Bold));
        p.setPen(QColor("#44aaff")); p.drawText(8, GH+34, "BL");
        for (int i = 0; i < std::min(m.player.shieldMax, 8); ++i) {
            bool full = i < m.player.shieldHp;
            QColor sc = full ? QColor("#44aaff") : QColor("#1a3355");
            p.fillRect(QRectF(28+i*15, GH+24, 12, 8), sc);
            if (full) p.fillRect(QRectF(30+i*15, GH+25, 4, 2), QColor("#aaddff"));
        }
        (void)shW;
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
    // Gold
    if (m.player.gold > 0) {
        QFont gf("monospace", 7, QFont::Bold);
        p.setFont(gf); p.setPen(QColor("#ffcc44"));
        p.drawText(GW - 80, GH+18, QString("$ %1").arg(m.player.gold));
    }
    // Sorts (slots 1-4, affichés à droite du HUD)
    {
        QFont sf("monospace", 6, QFont::Bold); p.setFont(sf);
        int slotW = 32, slotH = 22, slotStartX = GW - 4 * (slotW + 2) - 4;
        for (int i = 0; i < 4; ++i) {
            int spId = m.player.spellSlots[i];
            float cd = m.player.spellCds[i];
            int sx = slotStartX + i * (slotW + 2);
            int sy = GH + 30;
            const SpellInfo *sp = nullptr;
            for (auto &s : m.allSpells) if (s.id == spId) { sp = &s; break; }
            QColor bgc = sp ? sp->color : QColor("#222233");
            bgc.setAlpha(sp ? 120 : 60);
            p.fillRect(sx, sy, slotW, slotH, bgc);
            // Barre de cooldown
            if (sp && cd > 0) {
                float cdBase = sp->baseCd;
                float fill = 1.f - std::min(1.f, cd / std::max(0.01f, cdBase));
                QColor cdC(20, 15, 35, 200);
                p.fillRect(QRectF(sx, sy, slotW * (1.f - fill), slotH), cdC);
            }
            QColor borderC = sp ? sp->color : QColor("#444455");
            p.setPen(QPen(borderC, 1)); p.setBrush(Qt::NoBrush);
            p.drawRect(sx, sy, slotW, slotH); p.setPen(Qt::NoPen);
            // Numéro de slot
            p.setPen(QColor("#ffd544"));
            p.drawText(sx + 2, sy + 8, QString::number(i+1));
            // Nom du sort (court)
            if (sp) {
                p.setPen(cd > 0 ? QColor("#886699") : sp->color);
                p.drawText(sx + 2, sy + 18, sp->name.left(6));
            } else {
                p.setPen(QColor("#444455"));
                p.drawText(sx + 2, sy + 18, "---");
            }
            p.setPen(Qt::NoPen);
        }
    }
    // Boss bar
    const Enemy *boss = nullptr;
    for (auto &e : m.enemies) if (e.type==ET_MiniBoss || e.type==ET_FinalBoss || e.type==ET_TrueFinalBoss) { boss = &e; break; }
    if (boss && !boss->dead) {
        float bw = 240, bh = 11;
        float bx = GW/2.f - bw/2.f, by = GH+10;
        p.fillRect(QRectF(bx, by, bw, bh), C_HPB);
        QColor bc;
        if (boss->type == ET_TrueFinalBoss) {
            switch (boss->phase) {
                case 5: bc = QColor("#ff00ff"); break;
                case 4: bc = QColor("#cc00ff"); break;
                case 3: bc = QColor("#9900ff"); break;
                case 2: bc = QColor("#6633ff"); break;
                default: bc = QColor("#bb44ff"); break;
            }
        } else if (boss->type == ET_FinalBoss) {
            bc = (boss->phase==4) ? QColor("#ff00ff") :
                 (boss->phase==3) ? QColor("#ff00aa") :
                 (boss->phase==2) ? QColor("#ff5500") : C_BE;
        } else {
            bc = (boss->phase == 2) ? QColor("#ff5500") : QColor("#cc3344");
        }
        p.fillRect(QRectF(bx, by, bw * boss->hp/boss->maxHp, bh), bc);
        p.setPen(QPen(C_GOLD, 1)); p.drawRect(QRectF(bx, by, bw, bh)); p.setPen(Qt::NoPen);
        QFont sf("monospace", 7, QFont::Bold); p.setFont(sf); p.setPen(C_GOLD);
        int wIdx = boss->subType;
        QString name = (wIdx>=0 && wIdx<m.worlds.size()) ? m.worlds[wIdx].bossNameFr : "BOSS";
        QString label = (boss->type == ET_TrueFinalBoss) ?
            QString("%1 - PHASE %2 / 5").arg(name).arg(boss->phase) :
            (boss->type == ET_FinalBoss) ?
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

    // ===== Biome particles (sous le reste) =====
    for (auto &bp : m.biomeFx) {
        float a = std::max(0.f, bp.life/bp.maxLife);
        QColor c = bp.color; c.setAlphaF(std::min(1.f, a) * (c.alphaF() > 0 ? c.alphaF() : 1.f));
        p.fillRect(QRectF(bp.x - bp.size/2.f, bp.y - bp.size/2.f, bp.size, bp.size), c);
    }

    for (auto &pa : m.particles) {
        float a = std::max(0.f, pa.life/pa.maxLife);
        QColor c = pa.color; c.setAlphaF(a);
        p.fillRect(QRectF(pa.x - pa.size/2.f, pa.y - pa.size/2.f, pa.size, pa.size), c);
    }

    // ===== Pickups =====
    for (auto &pu : m.pickups) {
        float bob = std::sin(pu.bobPhase) * 2.5f;
        float yy = pu.y + bob;
        QColor outline("#000000"), fillC, glow;
        switch (pu.type) {
        case PU_Gold:    fillC=QColor("#ffcc44"); glow=QColor("#ffee88"); break;
        case PU_Heart:   fillC=QColor("#ff4477"); glow=QColor("#ff88aa"); break;
        case PU_Potion:  fillC=QColor("#ff2255"); glow=QColor("#ff5577"); break;
        case PU_Scroll:  fillC=QColor("#cc88ff"); glow=QColor("#eeaaff"); break;
        case PU_Chest:   fillC=QColor("#aa6633"); glow=QColor("#ddaa44"); break;
        }
        // Halo
        QColor hl = glow; hl.setAlphaF(0.4f + 0.2f*std::sin(m.globalTime*4+pu.bobPhase));
        p.fillRect(QRectF(pu.x-10, yy-10, 20, 20), hl);
        // Forme
        if (pu.type == PU_Heart) {
            p.fillRect(QRectF(pu.x-5, yy-3, 3, 3), fillC);
            p.fillRect(QRectF(pu.x+2, yy-3, 3, 3), fillC);
            p.fillRect(QRectF(pu.x-5, yy, 10, 4), fillC);
            p.fillRect(QRectF(pu.x-3, yy+4, 6, 2), fillC);
            p.fillRect(QRectF(pu.x-1, yy+6, 2, 1), fillC);
        } else if (pu.type == PU_Gold) {
            p.fillRect(QRectF(pu.x-4, yy-3, 8, 6), fillC);
            p.fillRect(QRectF(pu.x-3, yy-2, 6, 4), glow);
        } else if (pu.type == PU_Scroll) {
            p.fillRect(QRectF(pu.x-5, yy-5, 10, 10), fillC);
            p.fillRect(QRectF(pu.x-3, yy-3, 6, 6), QColor("#ffeebb"));
            p.fillRect(QRectF(pu.x-2, yy-1, 4, 1), QColor("#000"));
            p.fillRect(QRectF(pu.x-2, yy+1, 4, 1), QColor("#000"));
        } else if (pu.type == PU_Chest) {
            p.fillRect(QRectF(pu.x-6, yy-4, 12, 10), fillC);
            p.fillRect(QRectF(pu.x-6, yy-4, 12, 3), glow);
            p.fillRect(QRectF(pu.x-1, yy, 2, 2), QColor("#ffd544"));
        } else { // Potion
            p.fillRect(QRectF(pu.x-2, yy-6, 4, 2), QColor("#666"));
            p.fillRect(QRectF(pu.x-3, yy-4, 6, 8), fillC);
            p.fillRect(QRectF(pu.x-2, yy-2, 4, 4), glow);
        }
    }

    for (auto &e : m.enemies) {
        if (e.type == ET_MiniBoss || e.type == ET_FinalBoss || e.type == ET_TrueFinalBoss) {
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
                QColor hpCol = (e.type == ET_TrueFinalBoss) ? QColor("#bb44ff")
                             : (e.type == ET_FinalBoss) ? C_BE : QColor("#cc4422");
                p.fillRect(QRectF(bx, by, bw * e.hp/e.maxHp, bh), hpCol);
            }
            continue;
        }
        if (e.type == ET_Elite) {
            int v = std::max(0, std::min(3, e.eliteVariant));
            const QImage &sheet = m_assets.sprElite[v];
            if (!sheet.isNull()) {
                // Aura pulsée
                int pulse = (int)(std::sin(e.auraPhase)*40);
                QColor aura(255, 90+pulse, 50, 110);
                p.fillRect(QRectF(e.x-26, e.y-30, 52, 52), aura);
                // Sprite direct (pas spritesheet animé, 64x96)
                int fw = sheet.width(), fh = sheet.height();
                float sc = (e.size/40.f);
                float dw = fw*sc, dh = fh*sc;
                p.save();
                if (e.facingLeft) {
                    p.translate(int(e.x), 0); p.scale(-1, 1);
                    p.drawImage(QRectF(-dw/2, e.y - dh*0.65f, dw, dh), sheet);
                } else {
                    p.drawImage(QRectF(e.x-dw/2, e.y-dh*0.65f, dw, dh), sheet);
                }
                p.restore();
                if (e.hitFlash > 0) {
                    p.save();
                    p.setCompositionMode(QPainter::CompositionMode_Plus);
                    p.setOpacity(std::min(0.6f, e.hitFlash*3));
                    p.drawImage(QRectF(e.x-dw/2, e.y-dh*0.65f, dw, dh), sheet);
                    p.restore();
                }
                if (e.hp < e.maxHp) {
                    float bw = e.size*1.4f, bh = 5;
                    p.fillRect(QRectF(e.x-bw/2, e.y-dh*0.65f-10, bw, bh), C_HPB);
                    p.fillRect(QRectF(e.x-bw/2, e.y-dh*0.65f-10, bw*e.hp/e.maxHp, bh), QColor("#ff5544"));
                }
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

    // ===== FX sprites (explosions, portails, etc) =====
    for (auto &fx : m.fxEffects) {
        QImage sheet = m_assets.fxSheets[fx.sheetKey];
        if (sheet.isNull()) continue;
        int frame = std::min(fx.frameCount-1, (int)(fx.timer * fx.fps));
        float dw = fx.frameW * fx.scale, dh = fx.frameH * fx.scale;
        QRectF dst(fx.x - dw/2, fx.y - dh/2, dw, dh);
        QRectF src(frame * fx.frameW, 0, fx.frameW, fx.frameH);
        p.save();
        p.setCompositionMode(QPainter::CompositionMode_Plus);
        p.drawImage(dst, sheet, src);
        p.restore();
    }

    // ===== Score popups =====
    for (auto &pop : m.popups) {
        float a = std::max(0.f, std::min(1.f, pop.life / pop.maxLife));
        QFont pf("monospace", (int)(11 * pop.scale), QFont::Bold);
        p.setFont(pf); QFontMetrics pfm(pf);
        int w = pfm.horizontalAdvance(pop.text);
        QColor shadow(0,0,0); shadow.setAlphaF(a*0.7f);
        p.setPen(shadow);
        p.drawText(int(pop.x - w/2 + 1), int(pop.y + 1), pop.text);
        QColor col = pop.color; col.setAlphaF(a);
        p.setPen(col);
        p.drawText(int(pop.x - w/2), int(pop.y), pop.text);
        p.setPen(Qt::NoPen);
    }

    drawHUD(p, m);
    drawDialogBubble(p, m);
    drawTutorialOverlay(p, m);
}

}
