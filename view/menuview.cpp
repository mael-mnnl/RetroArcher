#include "menuview.h"
#include "assetmanager.h"
#include "../model/gamemodel.h"
#include "../core/constants.h"
#include "../core/palette.h"
#include "../controller/skillsystem.h"
#include <QPainter>
#include <QFontMetrics>
#include <QRandomGenerator>
#include <QImage>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace View {

using namespace Model;
using namespace Core;
using namespace Palette;

namespace {
struct SkinDefMenu { const char *name; QColor accent; };
const SkinDefMenu g_skinsMenu[4] = {
    { "Bleu",             QColor("#1a55bb") },
    { "Vert lierre",      QColor("#1a8833") },
    { "Rouge cramoisi",   QColor("#bb1a33") },
    { "Or imperial",      QColor("#c8a800") },
};
const char *g_atkNames[3] = { "Coup d'epee", "Estoc rapide", "Frappe lourde" };
const char *g_atkDescs[3] = { "Cadence equilibree", "Rapide et precis", "Impact puissant" };
}

MenuView::MenuView(const AssetManager &assets) : m_assets(assets) {}

void MenuView::drawSpriteAt(QPainter &p, const QImage &sheet, int frameCount, float fps,
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

// =============== MENU PRINCIPAL ===============
void MenuView::drawMenu(QPainter &p, const GameModel &m)
{
    p.fillRect(0, 0, CW, CH, QColor(15, 8, 25));
    QRandomGenerator starRng(12345);
    for (int i = 0; i < 80; ++i) {
        float sx = starRng.bounded(CW), sy = starRng.bounded(CH);
        int alpha = 80 + (int)(std::sin(m.globalTime*1.5f + i)*60);
        QColor sc("#ffeebb"); sc.setAlpha(std::max(0, std::min(255, alpha)));
        p.fillRect(QRectF(sx, sy, 2, 2), sc);
    }

    QFont titleFont("monospace", 26, QFont::Bold);
    p.setFont(titleFont); QFontMetrics tfm(titleFont);
    QString title = "ARCADEHERO";
    int tw = tfm.horizontalAdvance(title);
    p.setPen(QColor(0,0,0)); p.drawText(CW/2 - tw/2 + 3, 50+3, title);
    p.setPen(C_GOLD); p.drawText(CW/2 - tw/2, 50, title);

    QFont subFont("monospace", 8); p.setFont(subFont); QFontMetrics sfm(subFont);
    QString sub = "Reforge la Pierre d'Arcane - Heros Inconnu";
    p.setPen(QColor("#bbaaff"));
    p.drawText(CW/2 - sfm.horizontalAdvance(sub)/2, 68, sub);

    QFont ff("monospace", 9, QFont::Bold); p.setFont(ff); QFontMetrics ffm(ff);
    QString frag = QString("Fragments  %1 / 5").arg(m.fragments);
    p.setPen(QColor("#ffd544"));
    int fragX = CW/2 - ffm.horizontalAdvance(frag)/2 - 14;
    int fragY = 90;
    p.drawText(fragX + 14, fragY, frag);
    float pulse = 0.5f + 0.5f*std::sin(m.globalTime*3);
    QColor gem(180+(int)(pulse*60), 130+(int)(pulse*80), 255);
    QPolygonF gemP;
    gemP << QPointF(fragX, fragY-6) << QPointF(fragX+5, fragY-12)
         << QPointF(fragX+10, fragY-6) << QPointF(fragX+5, fragY+0);
    p.setBrush(gem); p.setPen(QPen(QColor("#ffeeaa"), 1)); p.drawPolygon(gemP);
    p.setPen(Qt::NoPen); p.setBrush(Qt::NoBrush);

    drawSpriteAt(p, m_assets.sprSoldierSkin[m.menuSelectedSkin][AN_Idle],
                 AssetManager::s_idleFrameCount, 8.f,
                 CW/2.f, 138, 1.4f, false, m.globalTime);

    auto drawBtn = [&](int y, const QString &key, const QString &label, QColor accent) {
        QFont bf("monospace", 10, QFont::Bold); p.setFont(bf); QFontMetrics bfm(bf);
        QString text = QString("[%1] %2").arg(key, label);
        int w = bfm.horizontalAdvance(text) + 24;
        int x = CW/2 - w/2;
        QColor bg(20, 20, 40); p.fillRect(QRectF(x, y, w, 22), bg);
        p.setPen(QPen(accent, 2)); p.setBrush(Qt::NoBrush);
        p.drawRect(x, y, w, 22); p.setPen(Qt::NoPen);
        p.setPen(accent); p.drawText(x+12, y+15, text); p.setPen(Qt::NoPen);
    };
    drawBtn(186, "ESPACE", "Aventure",       QColor("#88ff88"));
    drawBtn(212, "T",      "Tutoriel",       QColor("#ffaa44"));
    drawBtn(238, "C",      "Personnaliser",  QColor("#aaccff"));
    drawBtn(264, "D",      "Decouvertes",    QColor("#bb88ff"));
    drawBtn(290, "L",      "Lore",           QColor("#ddccaa"));

    // Mini-boutons B & K à droite
    auto drawSmall = [&](int x, int y, const QString &key, const QString &label, QColor accent) {
        QFont bf("monospace", 7, QFont::Bold); p.setFont(bf); QFontMetrics bfm(bf);
        QString text = QString("[%1] %2").arg(key, label);
        int w = bfm.horizontalAdvance(text) + 12;
        QColor bg(20, 20, 40); p.fillRect(QRectF(x, y, w, 16), bg);
        p.setPen(QPen(accent, 1)); p.setBrush(Qt::NoBrush);
        p.drawRect(x, y, w, 16); p.setPen(Qt::NoPen);
        p.setPen(accent); p.drawText(x+6, y+11, text);
    };
    drawSmall(CW-110, 186, "B", "Benedictions", QColor("#ffd544"));
    drawSmall(CW-110, 208, "K", "Classement",   QColor("#88eeaa"));
    if (m.ascensionUnlocked)
        drawSmall(CW-110, 230, "A", "Ascension",  QColor("#ff5544"));

    int cy = 326;
    QFont cf("monospace", 7, QFont::Bold); p.setFont(cf); QFontMetrics cfm(cf);
    QColor cBorder = m.cheatFocused ? QColor("#ff66aa") : QColor("#5a4566");
    p.fillRect(QRectF(CW/2-100, cy, 200, 22), QColor(15, 10, 22));
    p.setPen(QPen(cBorder, m.cheatFocused?2:1)); p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(CW/2-100, cy, 200, 22)); p.setPen(Qt::NoPen);
    p.setPen(QColor("#ff88cc"));
    p.drawText(CW/2-94, cy+9, "[X] Code de triche");
    QString prompt = "> " + m.cheatInput;
    if (m.cheatFocused && (int)(m.globalTime*2)%2==0) prompt += "_";
    p.setPen(QColor("#ffeeaa"));
    p.drawText(CW/2-94, cy+19, prompt);
    if (m.cheatFocused) {
        QFont hf("monospace", 6); p.setFont(hf); QFontMetrics hfm(hf);
        QString hint = "ENTREE pour valider, ESC pour annuler";
        p.setPen(QColor("#aa88aa"));
        p.drawText(CW/2 - hfm.horizontalAdvance(hint)/2, cy+30, hint);
    }
    if (m.cheatInvincible) {
        QFont sf("monospace", 7, QFont::Bold); p.setFont(sf); QFontMetrics sfmx(sf);
        QString s = "* INVINCIBLE ACTIF *";
        p.setPen(QColor("#ff44aa"));
        p.drawText(CW/2 - sfmx.horizontalAdvance(s)/2, cy+38, s);
    }
    if (m.cheatFlashTimer > 0) {
        QFont sf("monospace", 8, QFont::Bold); p.setFont(sf); QFontMetrics sfmx(sf);
        QString s = "Code accepte !";
        QColor cc("#ffd544"); cc.setAlphaF(std::min(1.f, m.cheatFlashTimer));
        p.setPen(cc);
        p.drawText(CW/2 - sfmx.horizontalAdvance(s)/2, cy-4, s);
    }

    QFont mf("monospace", 7); p.setFont(mf); QFontMetrics mfm(mf);
    p.setPen(QColor("#ffd544"));
    QString hsTxt = QString("Record salle %1/%2").arg(m.highScore).arg(ROOMS_TOTAL);
    p.drawText(CW/2 - mfm.horizontalAdvance(hsTxt)/2, CH-20, hsTxt);
    QString controls = "ZQSD bouger - tirs auto immobile - SHIFT/T/G actions - ESC retour";
    p.setPen(QColor("#7766aa"));
    p.drawText(CW/2 - mfm.horizontalAdvance(controls)/2, CH-8, controls);
}

// =============== SKIN SELECT ===============
void MenuView::drawSkinSelect(QPainter &p, const GameModel &m)
{
    p.fillRect(0, 0, CW, CH, QColor(15, 8, 25));
    QFont titleFont("monospace", 16, QFont::Bold);
    p.setFont(titleFont); QFontMetrics tfm(titleFont);
    QString title = "PERSONNALISATION";
    int tw = tfm.horizontalAdvance(title);
    p.setPen(C_GOLD); p.drawText(CW/2 - tw/2, 30, title);

    QFont sub("monospace", 9); p.setFont(sub); QFontMetrics sfm(sub);
    p.setPen(QColor("#bbaaff"));
    QString s1 = "Couleur (Q/D) :";
    p.drawText(CW/2 - sfm.horizontalAdvance(s1)/2, 60, s1);

    int cardW = 90, gap = 10;
    int totalW = cardW*4 + gap*3;
    int sx = CW/2 - totalW/2, sy = 75;
    for (int i = 0; i < 4; ++i) {
        int x = sx + i*(cardW+gap);
        bool sel = (i == m.menuSelectedSkin);
        QColor bg = sel ? QColor(60, 50, 100) : QColor(25, 18, 40);
        p.fillRect(x, sy, cardW, 110, bg);
        if (sel) { p.setPen(QPen(g_skinsMenu[i].accent, 3)); p.setBrush(Qt::NoBrush);
                   p.drawRect(x, sy, cardW, 110); p.setPen(Qt::NoPen); }
        drawSpriteAt(p, m_assets.sprSoldierSkin[i][AN_Idle], AssetManager::s_idleFrameCount, 8.f,
                     x + cardW/2.f, sy + 78, 1.4f, false, m.globalTime);
        QFont nf("monospace", 7, QFont::Bold); p.setFont(nf); QFontMetrics nfm(nf);
        p.setPen(g_skinsMenu[i].accent);
        p.drawText(x + cardW/2 - nfm.horizontalAdvance(g_skinsMenu[i].name)/2, sy+102, g_skinsMenu[i].name);
    }

    p.setFont(sub); p.setPen(QColor("#bbaaff"));
    QString s2 = "Style d'attaque (Z/S) :";
    p.drawText(CW/2 - sfm.horizontalAdvance(s2)/2, 215, s2);
    int aSY = 230;
    for (int i = 0; i < 3; ++i) {
        int aY = aSY + i*30;
        bool sel = (i == m.menuSelectedAtk);
        QColor bg = sel ? QColor(60, 50, 100) : QColor(25, 18, 40);
        p.fillRect(40, aY, CW-80, 26, bg);
        if (sel) { p.setPen(QPen(QColor("#ffd544"), 2)); p.setBrush(Qt::NoBrush);
                   p.drawRect(40, aY, CW-80, 26); p.setPen(Qt::NoPen); }
        QFont nf("monospace", 8, QFont::Bold); p.setFont(nf);
        p.setPen(sel ? QColor("#ffd544") : QColor("#aaaacc"));
        p.drawText(50, aY+11, g_atkNames[i]);
        QFont df("monospace", 7); p.setFont(df);
        p.setPen(QColor("#aabbcc")); p.drawText(50, aY+22, g_atkDescs[i]);
    }

    QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
    QString footer = "ESPACE/ENTREE : valider  -  ESC : retour";
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - ffm.horizontalAdvance(footer)/2, CH-12, footer);
}

// =============== DECOUVERTES ===============
void MenuView::drawDiscoveries(QPainter &p, const GameModel &m)
{
    p.fillRect(0, 0, CW, CH, QColor(8, 5, 20));
    for (int y = 0; y < CH; y += 16)
        for (int x = 0; x < CW; x += 16) {
            QColor d = ((x/16+y/16)%2==0) ? QColor(14,10,28) : QColor(10,7,22);
            p.fillRect(x, y, 16, 16, d);
        }

    QFont titleFont("monospace", 16, QFont::Bold);
    p.setFont(titleFont); QFontMetrics tfm(titleFont);
    QString title = "DECOUVERTES";
    int tw = tfm.horizontalAdvance(title);
    p.setPen(QColor(0,0,0)); p.drawText(CW/2 - tw/2 + 2, 24+2, title);
    p.setPen(C_GOLD); p.drawText(CW/2 - tw/2, 24, title);

    QFont sub("monospace", 8); p.setFont(sub); QFontMetrics sfm(sub);
    QString s = QString("Fragments d'Arcane : %1 / 5  -  Mondes debloques : %2 / 5")
                .arg(m.fragments).arg(m.worldsUnlocked);
    p.setPen(QColor("#ffd544"));
    p.drawText(CW/2 - sfm.horizontalAdvance(s)/2, 38, s);

    int cardW = 95, cardH = 200, gap = 8;
    int totalW = cardW*5 + gap*4;
    int startX = CW/2 - totalW/2;
    int cardY = 60;

    for (int i = 0; i < 5 && i < m.worlds.size(); ++i) {
        int cx = startX + i*(cardW+gap);
        bool unlocked = (i+1) <= m.worldsUnlocked;
        bool hover = (i == m.discoveryHover);
        const WorldInfo &wi = m.worlds[i];
        QColor bg = unlocked ? QColor(28, 22, 50) : QColor(18, 14, 30);
        if (hover) bg = bg.lighter(150);
        p.fillRect(QRectF(cx, cardY, cardW, cardH), bg);
        QColor border = hover ? wi.accent : (unlocked ? wi.accent.darker(150) : QColor(60, 50, 80));
        p.setPen(QPen(border, hover ? 3 : 2)); p.setBrush(Qt::NoBrush);
        p.drawRect(cx, cardY, cardW, cardH); p.setPen(Qt::NoPen);

        QFont nf("monospace", 8, QFont::Bold); p.setFont(nf); QFontMetrics nfm(nf);
        QString num = QString("MONDE %1").arg(i+1);
        p.setPen(unlocked ? wi.accent : QColor("#5a5070"));
        p.drawText(cx + cardW/2 - nfm.horizontalAdvance(num)/2, cardY + 14, num);

        QImage sheet = m_assets.bossSheets[wi.bossKey];
        if (!sheet.isNull()) {
            int frame = unlocked ? ((int)(m.globalTime * wi.bossFps) % wi.bossFrameCount) : 0;
            int fw = wi.bossFrameW, fh = wi.bossFrameH;
            float zone = 75.f;
            float scale = std::min(zone/fw, zone/fh) * 0.95f;
            float dw = fw*scale, dh = fh*scale;
            float dx = cx + cardW/2.f - dw/2.f;
            float dy = cardY + 22 + 80 - dh/2.f;
            QRectF dst(dx, dy, dw, dh), src(frame*fw, 0, fw, fh);
            if (unlocked) p.drawImage(dst, sheet, src);
            else {
                QImage tmp = sheet.copy(frame*fw, 0, fw, fh).convertToFormat(QImage::Format_ARGB32);
                for (int y2=0; y2<tmp.height(); ++y2) {
                    QRgb *line = reinterpret_cast<QRgb*>(tmp.scanLine(y2));
                    for (int x2=0; x2<tmp.width(); ++x2) {
                        QColor c = QColor::fromRgba(line[x2]);
                        if (c.alpha() == 0) continue;
                        int g = 30; line[x2] = qRgba(g, g, g+10, std::max(80, c.alpha()-50));
                    }
                }
                p.drawImage(dst, tmp);
                QFont lf("monospace", 14, QFont::Bold); p.setFont(lf);
                QFontMetrics lfm(lf);
                QString lock = "X";
                p.setPen(QColor("#ff4466"));
                p.drawText(cx + cardW/2 - lfm.horizontalAdvance(lock)/2,
                           cardY + 22 + 80 + 6, lock);
                p.setPen(Qt::NoPen);
            }
        }

        QFont bnf("monospace", 7, QFont::Bold); p.setFont(bnf); QFontMetrics bfm(bnf);
        QString bname = unlocked ? wi.bossNameFr : "???";
        p.setPen(unlocked ? QColor("#ffffff") : QColor("#666666"));
        QString line1 = bname; QString line2;
        if (bfm.horizontalAdvance(bname) > cardW-6) {
            int sp = bname.lastIndexOf(' ', bname.size()/2 + 3);
            if (sp > 0) { line1 = bname.left(sp); line2 = bname.mid(sp+1); }
        }
        p.drawText(cx + cardW/2 - bfm.horizontalAdvance(line1)/2, cardY + 130, line1);
        if (!line2.isEmpty()) p.drawText(cx + cardW/2 - bfm.horizontalAdvance(line2)/2, cardY + 140, line2);

        int total = 0, available = 0;
        for (auto &sk : m.allSkills) {
            if (sk.worldTier == i+1) {
                total++;
                if ((i+1) <= m.worldsUnlocked) available++;
            }
        }
        QFont skf("monospace", 6); p.setFont(skf); QFontMetrics skfm(skf);
        QString sktxt = QString("%1/%2 skills").arg(available).arg(total);
        p.setPen(QColor("#88ccff"));
        p.drawText(cx + cardW/2 - skfm.horizontalAdvance(sktxt)/2, cardY + 158, sktxt);

        p.setFont(skf);
        QString status = unlocked ? (i+1 < m.worldsUnlocked ? "VAINCU" : "ACTIF") : "VERROUILLE";
        QColor stc = unlocked ? (i+1 < m.worldsUnlocked ? QColor("#88ff88") : QColor("#ffcc44"))
                              : QColor("#ff5566");
        p.setPen(stc);
        p.drawText(cx + cardW/2 - skfm.horizontalAdvance(status)/2, cardY + 175, status);
    }

    if (m.discoveryHover >= 0 && m.discoveryHover < 5 && m.discoveryHover < m.worlds.size()) {
        const WorldInfo &wi = m.worlds[m.discoveryHover];
        bool unlocked = (m.discoveryHover+1) <= m.worldsUnlocked;
        QFont nf("monospace", 9, QFont::Bold); p.setFont(nf); QFontMetrics nfm(nf);
        QString name = wi.name; int nw = nfm.horizontalAdvance(name);
        p.setPen(unlocked ? wi.accent : QColor("#7066aa"));
        p.drawText(CW/2 - nw/2, cardY + cardH + 22, name);
        QFont ff2("monospace", 8); p.setFont(ff2); QFontMetrics ffm2(ff2);
        QString fla = unlocked ? wi.flavor : "Survis aux mondes precedents pour le decouvrir...";
        p.setPen(QColor("#ddccff"));
        p.drawText(CW/2 - ffm2.horizontalAdvance(fla)/2, cardY + cardH + 38, fla);
    }

    QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
    QString footer = "<-/-> selection  -  ESC : retour";
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - ffm.horizontalAdvance(footer)/2, CH-12, footer);
}

// =============== LORE ===============
void MenuView::drawLore(QPainter &p, const GameModel &m)
{
    p.fillRect(0, 0, CW, CH, QColor(8, 5, 20));
    QRandomGenerator starRng(54321);
    for (int i = 0; i < 100; ++i) {
        float sx = starRng.bounded(CW), sy = starRng.bounded(CH);
        int alpha = 60 + (int)(std::sin(m.globalTime*1.2f + i*0.3f)*50);
        QColor sc((i%3==0)?"#bbaaff":"#ffeebb"); sc.setAlpha(std::max(0, std::min(255, alpha)));
        p.fillRect(QRectF(sx, sy, 2, 2), sc);
    }

    QFont titleFont("monospace", 14, QFont::Bold);
    p.setFont(titleFont); QFontMetrics tfm(titleFont);
    QString title = "LE LORE D'ARCADEHERO";
    int tw = tfm.horizontalAdvance(title);
    p.setPen(C_GOLD); p.drawText(CW/2 - tw/2, 26, title);

    QFont bf("monospace", 8); p.setFont(bf); QFontMetrics bfm(bf);
    p.setPen(QColor("#ddccff"));
    QStringList paragraphs = {
        QString::fromUtf8("Le monde n'est plus qu'un tissu dechire. Lord Malificus, tyran des"),
        QString::fromUtf8("abysses, a derobe la Pierre d'Arcane qui scellait l'equilibre cosmique."),
        QString::fromUtf8("L'artefact a explose en fragments et l'univers s'est brise."),
        "",
        QString::fromUtf8("Tu es le Heros Inconnu. La volonte residuelle de la Pierre s'est"),
        QString::fromUtf8("refugiee dans ton ame, te conferant un Fragment initial - une cle qui"),
        QString::fromUtf8("absorbe la puissance des autres fragments quand tu les recuperes."),
        "",
        QString::fromUtf8("Cinq entites corrompues gardent jalousement les fragments majeurs"),
        QString::fromUtf8("dans des biomes extremes : forge ardente, catacombes, marais maudit,"),
        QString::fromUtf8("arene de l'anneau et citadelle gelee."),
        "",
        QString::fromUtf8("Chaque fragment recupere debloque ta memoire magique : de nouveaux"),
        QString::fromUtf8("schemas d'energie se revelent dans tes armes. Reunis-les tous pour"),
        QString::fromUtf8("reforger la Pierre et sceller les failles dimensionnelles.")
    };
    int y = 56;
    for (auto &line : paragraphs) {
        if (line.isEmpty()) { y += 6; continue; }
        p.drawText(CW/2 - bfm.horizontalAdvance(line)/2, y, line);
        y += 14;
    }

    QFont qf("monospace", 10, QFont::Bold); p.setFont(qf); QFontMetrics qfm(qf);
    QString quete = QString("Fragments en ta possession : %1 / 5").arg(m.fragments);
    p.setPen(C_GOLD);
    p.drawText(CW/2 - qfm.horizontalAdvance(quete)/2, y+12, quete);

    QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
    QString footer = "ESC : retour";
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - ffm.horizontalAdvance(footer)/2, CH-12, footer);
}

// =============== SKILL SELECT ===============
void MenuView::drawSkillSelect(QPainter &p, const GameModel &m)
{
    QColor overlay(0,0,0); overlay.setAlphaF(0.78f);
    p.fillRect(0, 0, CW, CH, overlay);
    QFont titleFont("monospace", 14, QFont::Bold);
    p.setFont(titleFont); QFontMetrics tfm(titleFont);
    QString title = "CHOISIS UNE AMELIORATION";
    int tw = tfm.horizontalAdvance(title);
    p.setPen(C_GOLD); p.drawText(CW/2 - tw/2, 40, title);

    int n = (int)m.skillChoices.size();
    if (n <= 0) return;
    int cardW = 145, gap = 12;
    int totalW = cardW*n + gap*(n-1);
    int sx = CW/2 - totalW/2, sy = 80;
    for (int i = 0; i < n; ++i) {
        int x = sx + i*(cardW+gap);
        const Skill *sk = nullptr;
        for (auto &s : m.allSkills) if (s.id == m.skillChoices[i]) { sk = &s; break; }
        if (!sk) continue;
        QColor bg = sk->color; bg.setAlpha(60);
        p.fillRect(x, sy, cardW, 200, bg);
        p.setPen(QPen(sk->color, 2)); p.setBrush(Qt::NoBrush);
        p.drawRect(x, sy, cardW, 200); p.setPen(Qt::NoPen);

        QFont keyf("monospace", 22, QFont::Bold); p.setFont(keyf); QFontMetrics kfm(keyf);
        QString keyTxt = QString::number(i+1);
        p.setPen(sk->color);
        p.drawText(x + cardW/2 - kfm.horizontalAdvance(keyTxt)/2, sy+38, keyTxt);

        QFont icf("monospace", 18, QFont::Bold); p.setFont(icf); QFontMetrics ifm(icf);
        p.drawText(x + cardW/2 - ifm.horizontalAdvance(sk->icon)/2, sy+78, sk->icon);

        QFont nf("monospace", 9, QFont::Bold); p.setFont(nf); QFontMetrics nfm(nf);
        p.drawText(x + cardW/2 - nfm.horizontalAdvance(sk->name)/2, sy+108, sk->name);

        QFont df("monospace", 7); p.setFont(df); QFontMetrics dfm(df);
        p.setPen(QColor("#ddddff"));
        QString desc = sk->desc;
        QStringList words = desc.split(' ');
        int lineY = sy+128; QString line;
        for (auto &w : words) {
            int aw = dfm.horizontalAdvance(line + (line.isEmpty()?"":" ") + w);
            if (aw > cardW-12 && !line.isEmpty()) {
                p.drawText(x + cardW/2 - dfm.horizontalAdvance(line)/2, lineY, line);
                lineY += 11; line = w;
            } else line = line.isEmpty() ? w : line + " " + w;
        }
        if (!line.isEmpty())
            p.drawText(x + cardW/2 - dfm.horizontalAdvance(line)/2, lineY, line);

        QFont tf2("monospace", 6, QFont::Bold); p.setFont(tf2); QFontMetrics tfm2(tf2);
        QString tier = QString("MONDE %1").arg(sk->worldTier);
        p.setPen(sk->color.lighter(170));
        p.drawText(x + cardW/2 - tfm2.horizontalAdvance(tier)/2, sy+193, tier);
    }
    QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
    QString tip = "Appuie sur 1, 2 ou 3 pour choisir";
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - ffm.horizontalAdvance(tip)/2, CH-30, tip);
}

// =============== END SCREEN ===============
void MenuView::drawEndScreen(QPainter &p, const GameModel &m, bool win)
{
    QColor overlay(0, 0, 0); overlay.setAlphaF(0.8f);
    p.fillRect(0, 0, CW, CH, overlay);
    QFont titleFont("monospace", 24, QFont::Bold);
    p.setFont(titleFont); QFontMetrics tfm(titleFont);
    QString title = win ? "VICTOIRE !" : "GAME OVER";
    int tw = tfm.horizontalAdvance(title);
    p.setPen(win ? C_GOLD : C_HP);
    p.drawText(CW/2 - tw/2, CH/2 - 20, title);

    QFont sf("monospace", 9); p.setFont(sf); QFontMetrics sfm(sf);
    int worldNum = std::min(5, ((m.currentRoom-1)/5)+1);
    QString s = win ? QString("Tu as terrasse le Gardien du Givre !")
                    : QString("Tu es tombe dans la salle %1 (M%2)").arg(m.currentRoom).arg(worldNum);
    p.setPen(QColor("#ddccff"));
    p.drawText(CW/2 - sfm.horizontalAdvance(s)/2, CH/2 + 8, s);

    QString h = QString("Record : Salle %1/%2  -  Mondes debloques : %3/5")
                .arg(m.highScore).arg(ROOMS_TOTAL).arg(m.worldsUnlocked);
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - sfm.horizontalAdvance(h)/2, CH/2 + 26, h);

    QString r = "ESPACE : rejouer    -    ESC : menu";
    p.setPen(QColor("#88ff88"));
    p.drawText(CW/2 - sfm.horizontalAdvance(r)/2, CH/2 + 50, r);
}

// =============== CLASS SELECT ===============
void MenuView::drawClassSelect(QPainter &p, const GameModel &m, int hover)
{
    p.fillRect(0, 0, CW, CH, QColor(12, 8, 25));
    QFont tf("monospace", 16, QFont::Bold); p.setFont(tf); QFontMetrics tfm(tf);
    QString t = "CHOISIS TON ARCHETYPE";
    p.setPen(C_GOLD); p.drawText(CW/2 - tfm.horizontalAdvance(t)/2, 30, t);

    int n = (int)m.allClasses.size();
    int cardW = 115, gap = 10;
    int totalW = cardW*n + gap*(n-1);
    int sx = CW/2 - totalW/2, sy = 70;
    for (int i = 0; i < n; ++i) {
        int x = sx + i*(cardW+gap);
        const ClassInfo &c = m.allClasses[i];
        bool sel = (i == hover);
        QColor bg = sel ? QColor(45, 35, 80) : QColor(22, 16, 38);
        p.fillRect(x, sy, cardW, 210, bg);
        p.setPen(QPen(c.accent, sel?3:2)); p.setBrush(Qt::NoBrush);
        p.drawRect(x, sy, cardW, 210); p.setPen(Qt::NoPen);
        QFont nf("monospace", 9, QFont::Bold); p.setFont(nf); QFontMetrics nfm(nf);
        p.setPen(c.accent);
        p.drawText(x + cardW/2 - nfm.horizontalAdvance(c.name)/2, sy+18, c.name);
        // Stats
        QFont sf2("monospace", 7); p.setFont(sf2);
        p.setPen(QColor("#ccddff"));
        QStringList stats = {
            QString("PV : %1").arg(int(c.baseHp)),
            QString("Vitesse : %1").arg(int(c.baseSpeed)),
            QString("Cadence : %1").arg(QString::number(c.baseFireRate, 'f', 2)),
            QString("Degats : %1").arg(int(c.baseDamage)),
            QString("Grenades : %1").arg(c.startGrenade)
        };
        int yy = sy+40;
        for (auto &s : stats) { p.drawText(x+10, yy, s); yy += 11; }
        // Desc
        p.setPen(QColor("#bbaaff"));
        QStringList words = c.desc.split(' ');
        QString line; int dy = yy + 8;
        QFontMetrics dfm(sf2);
        for (auto &w : words) {
            QString test = line.isEmpty() ? w : line + " " + w;
            if (dfm.horizontalAdvance(test) > cardW-12) {
                p.drawText(x+6, dy, line); dy += 10; line = w;
            } else line = test;
        }
        if (!line.isEmpty()) p.drawText(x+6, dy, line);
        if (sel) {
            QFont kf("monospace", 16, QFont::Bold); p.setFont(kf);
            p.setPen(c.accent);
            QString k = QString::number(i+1);
            QFontMetrics kfm(kf);
            p.drawText(x + cardW/2 - kfm.horizontalAdvance(k)/2, sy+205, k);
        }
    }
    QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
    QString foot = "<-/-> selection - ESPACE/ENTREE confirmer - ESC retour";
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - ffm.horizontalAdvance(foot)/2, CH-12, foot);
}

// =============== RELIC SELECT ===============
void MenuView::drawRelicSelect(QPainter &p, const GameModel &m, int hover)
{
    p.fillRect(0, 0, CW, CH, QColor(10, 6, 20));
    QFont tf("monospace", 16, QFont::Bold); p.setFont(tf); QFontMetrics tfm(tf);
    QString t = "RELIQUE DE DEPART";
    p.setPen(C_GOLD); p.drawText(CW/2 - tfm.horizontalAdvance(t)/2, 32, t);
    QFont sf2("monospace", 8); p.setFont(sf2); QFontMetrics sfm(sf2);
    QString s = "Choisis une relique - effets permanents pour ce run";
    p.setPen(QColor("#bbaaff"));
    p.drawText(CW/2 - sfm.horizontalAdvance(s)/2, 50, s);

    int n = std::min((int)m.relicChoices.size(), 3);
    int cardW = 155, gap = 12, sy = 80;
    int totalW = cardW*n + gap*(n-1);
    int sx = CW/2 - totalW/2;
    for (int i = 0; i < n; ++i) {
        int x = sx + i*(cardW+gap);
        int rid = m.relicChoices[i];
        if (rid < 0 || rid >= (int)m.allRelics.size()) continue;
        const RelicInfo &r = m.allRelics[rid];
        bool sel = (i == hover);
        QColor bg = r.color; bg.setAlpha(sel?100:50);
        p.fillRect(x, sy, cardW, 220, bg);
        p.setPen(QPen(r.color, sel?3:2)); p.setBrush(Qt::NoBrush);
        p.drawRect(x, sy, cardW, 220); p.setPen(Qt::NoPen);
        QFont nf("monospace", 10, QFont::Bold); p.setFont(nf);
        p.setPen(r.color); QFontMetrics nfm(nf);
        p.drawText(x + cardW/2 - nfm.horizontalAdvance(r.name)/2, sy+22, r.name);
        QFont kf("monospace", 28, QFont::Bold); p.setFont(kf);
        QString k = QString::number(i+1);
        QFontMetrics kfm(kf);
        p.drawText(x + cardW/2 - kfm.horizontalAdvance(k)/2, sy+90, k);
        QFont df("monospace", 7); p.setFont(df); QFontMetrics dfm(df);
        p.setPen(QColor("#ddccff"));
        QStringList words = r.desc.split(' ');
        QString line; int dy = sy + 130;
        for (auto &w : words) {
            QString test = line.isEmpty() ? w : line + " " + w;
            if (dfm.horizontalAdvance(test) > cardW-14) {
                p.drawText(x+cardW/2 - dfm.horizontalAdvance(line)/2, dy, line);
                dy += 12; line = w;
            } else line = test;
        }
        if (!line.isEmpty())
            p.drawText(x+cardW/2 - dfm.horizontalAdvance(line)/2, dy, line);
    }
    QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
    QString foot = "1, 2, 3 pour choisir - 0 pour aucune relique";
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - ffm.horizontalAdvance(foot)/2, CH-18, foot);
}

// =============== CURSE SELECT ===============
void MenuView::drawCurseSelect(QPainter &p, const GameModel &m)
{
    QColor overlay(0,0,0); overlay.setAlphaF(0.78f);
    p.fillRect(0, 0, CW, CH, overlay);
    QFont tf("monospace", 14, QFont::Bold); p.setFont(tf); QFontMetrics tfm(tf);
    QString t = "CHOISIS UNE MALEDICTION";
    p.setPen(QColor("#ff66aa")); p.drawText(CW/2 - tfm.horizontalAdvance(t)/2, 50, t);
    QFont sf2("monospace", 8); p.setFont(sf2); QFontMetrics sfm(sf2);
    p.setPen(QColor("#ddbbcc"));
    QString s = "Le pouvoir a un prix - une malediction obligatoire";
    p.drawText(CW/2 - sfm.horizontalAdvance(s)/2, 68, s);
    int n = std::min((int)m.curseChoices.size(), 2);
    int cardW = 200, gap = 18, sy = 100;
    int totalW = cardW*n + gap*(n-1);
    int sx = CW/2 - totalW/2;
    for (int i = 0; i < n; ++i) {
        int x = sx + i*(cardW+gap);
        int cid = m.curseChoices[i];
        if (cid < 0 || cid >= (int)m.allCurses.size()) continue;
        const CurseInfo &c = m.allCurses[cid];
        QColor bg(40,10,20); p.fillRect(x, sy, cardW, 180, bg);
        p.setPen(QPen(c.color, 2)); p.setBrush(Qt::NoBrush);
        p.drawRect(x, sy, cardW, 180); p.setPen(Qt::NoPen);
        QFont nf("monospace", 11, QFont::Bold); p.setFont(nf); QFontMetrics nfm(nf);
        p.setPen(c.color);
        p.drawText(x+cardW/2 - nfm.horizontalAdvance(c.name)/2, sy+30, c.name);
        QFont kf("monospace", 24, QFont::Bold); p.setFont(kf);
        QString k = QString::number(i+1);
        QFontMetrics kfm(kf);
        p.drawText(x+cardW/2 - kfm.horizontalAdvance(k)/2, sy+85, k);
        QFont df("monospace", 8); p.setFont(df); QFontMetrics dfm(df);
        p.setPen(QColor("#ffddee"));
        QStringList words = c.desc.split(' ');
        QString line; int dy = sy + 130;
        for (auto &w : words) {
            QString test = line.isEmpty() ? w : line + " " + w;
            if (dfm.horizontalAdvance(test) > cardW-14) {
                p.drawText(x+cardW/2 - dfm.horizontalAdvance(line)/2, dy, line);
                dy += 12; line = w;
            } else line = test;
        }
        if (!line.isEmpty())
            p.drawText(x+cardW/2 - dfm.horizontalAdvance(line)/2, dy, line);
    }
}

// =============== SHOP ===============
void MenuView::drawShop(QPainter &p, const GameModel &m, int hover)
{
    QColor overlay(0,0,0); overlay.setAlphaF(0.5f);
    p.fillRect(0, 0, CW, CH, overlay);
    QFont tf("monospace", 13, QFont::Bold); p.setFont(tf); QFontMetrics tfm(tf);
    QString t = "MARCHAND ITINERANT";
    p.setPen(QColor("#44ddaa")); p.drawText(CW/2 - tfm.horizontalAdvance(t)/2, 36, t);
    QFont sf2("monospace", 8); p.setFont(sf2); QFontMetrics sfm(sf2);
    p.setPen(QColor("#ffcc44"));
    QString g = QString("Or : $%1").arg(m.player.gold);
    p.drawText(CW/2 - sfm.horizontalAdvance(g)/2, 52, g);

    int n = (int)m.shopItems.size();
    int cardW = 130, gap = 14, sy = 78;
    int totalW = cardW*n + gap*(n-1);
    int sx = CW/2 - totalW/2;
    for (int i = 0; i < n; ++i) {
        int x = sx + i*(cardW+gap);
        int sid = m.shopItems[i];
        const Skill *sk = nullptr;
        for (auto &s2 : m.allSkills) if (s2.id == sid) { sk = &s2; break; }
        if (!sk) continue;
        bool sel = (i == hover);
        QColor bg = sk->color; bg.setAlpha(sel?100:50);
        p.fillRect(x, sy, cardW, 180, bg);
        p.setPen(QPen(sk->color, sel?3:2)); p.setBrush(Qt::NoBrush);
        p.drawRect(x, sy, cardW, 180); p.setPen(Qt::NoPen);
        QFont nf("monospace", 9, QFont::Bold); p.setFont(nf); QFontMetrics nfm(nf);
        p.setPen(sk->color);
        p.drawText(x+cardW/2 - nfm.horizontalAdvance(sk->name)/2, sy+22, sk->name);
        QFont kf("monospace", 22, QFont::Bold); p.setFont(kf);
        QString k = QString::number(i+1);
        QFontMetrics kfm(kf);
        p.drawText(x+cardW/2 - kfm.horizontalAdvance(k)/2, sy+76, k);
        // Prix
        QFont pf("monospace", 10, QFont::Bold); p.setFont(pf); QFontMetrics pfm(pf);
        QString price = QString("$ %1").arg(m.shopPrices[i]);
        bool canAfford = m.player.gold >= m.shopPrices[i];
        p.setPen(canAfford ? QColor("#ffcc44") : QColor("#776655"));
        p.drawText(x+cardW/2 - pfm.horizontalAdvance(price)/2, sy+105, price);
        QFont df("monospace", 7); p.setFont(df); QFontMetrics dfm(df);
        p.setPen(QColor("#ddddff"));
        QStringList words = sk->desc.split(' ');
        QString line; int dy = sy + 130;
        for (auto &w : words) {
            QString test = line.isEmpty() ? w : line + " " + w;
            if (dfm.horizontalAdvance(test) > cardW-12) {
                p.drawText(x+cardW/2 - dfm.horizontalAdvance(line)/2, dy, line);
                dy += 11; line = w;
            } else line = test;
        }
        if (!line.isEmpty())
            p.drawText(x+cardW/2 - dfm.horizontalAdvance(line)/2, dy, line);
    }
    QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
    QString foot = "1,2,3 acheter - ESC partir";
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - ffm.horizontalAdvance(foot)/2, CH-18, foot);
}

// =============== FORGE ===============
void MenuView::drawForge(QPainter &p, const GameModel &m, int hover)
{
    QColor overlay(0,0,0); overlay.setAlphaF(0.6f);
    p.fillRect(0, 0, CW, CH, overlay);
    QFont tf("monospace", 13, QFont::Bold); p.setFont(tf); QFontMetrics tfm(tf);
    QString t = "FORGE";
    p.setPen(QColor("#ffaa44")); p.drawText(CW/2 - tfm.horizontalAdvance(t)/2, 36, t);
    QFont sf2("monospace", 8); p.setFont(sf2); QFontMetrics sfm(sf2);
    QString s = "Sacrifie 1 skill pour en obtenir 1 du tier superieur";
    p.setPen(QColor("#ffddaa"));
    p.drawText(CW/2 - sfm.horizontalAdvance(s)/2, 52, s);

    int n = (int)m.player.skills.size();
    if (n == 0) {
        QFont mf("monospace", 9); p.setFont(mf); QFontMetrics mfm(mf);
        QString msg = "Tu n'as aucun skill a sacrifier !";
        p.setPen(QColor("#cccccc"));
        p.drawText(CW/2 - mfm.horizontalAdvance(msg)/2, CH/2, msg);
    } else {
        QFont kf("monospace", 7, QFont::Bold); p.setFont(kf); QFontMetrics kfm(kf);
        int colW = 75, gap = 6;
        int perRow = std::min(6, (CW - 40) / (colW+gap));
        int startX = CW/2 - (perRow*(colW+gap)-gap)/2;
        int rowY = 80;
        for (int i = 0; i < n; ++i) {
            int rx = startX + (i % perRow)*(colW+gap);
            int ry = rowY + (i / perRow)*40;
            int sid = m.player.skills[i];
            const Skill *sk = nullptr;
            for (auto &s2 : m.allSkills) if (s2.id == sid) { sk = &s2; break; }
            if (!sk) continue;
            bool sel = (i == hover);
            QColor bg = sk->color; bg.setAlpha(sel?120:60);
            p.fillRect(rx, ry, colW, 34, bg);
            p.setPen(QPen(sk->color, sel?2:1)); p.setBrush(Qt::NoBrush);
            p.drawRect(rx, ry, colW, 34); p.setPen(Qt::NoPen);
            p.setPen(sk->color);
            p.drawText(rx+3, ry+10, sk->name.left(11));
            QFont ff2("monospace", 6); p.setFont(ff2);
            p.setPen(QColor("#cccccc"));
            p.drawText(rx+3, ry+22, QString("M%1").arg(sk->worldTier));
            p.setFont(kf);
        }
    }
    QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
    QString foot = "<-/-> nav - ESPACE sacrifier - ESC partir";
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - ffm.horizontalAdvance(foot)/2, CH-18, foot);
}

// =============== BLESSINGS (meta) ===============
void MenuView::drawBlessings(QPainter &p, const GameModel &m, int hover)
{
    p.fillRect(0, 0, CW, CH, QColor(8, 5, 22));
    QFont tf("monospace", 14, QFont::Bold); p.setFont(tf); QFontMetrics tfm(tf);
    QString t = "BENEDICTIONS PERMANENTES";
    p.setPen(C_GOLD); p.drawText(CW/2 - tfm.horizontalAdvance(t)/2, 32, t);
    QFont sf2("monospace", 8); p.setFont(sf2); QFontMetrics sfm(sf2);
    QString s = QString("Fragments disponibles : %1").arg(m.blessingPointsAvail);
    p.setPen(QColor("#ffd544"));
    p.drawText(CW/2 - sfm.horizontalAdvance(s)/2, 52, s);

    struct B { QString name; QString desc; int cost; bool active; };
    B blessings[3] = {
        { QString("PV+ x%1").arg(m.blessingMaxHpBonus),
          "+1 PV maximum au depart (cumulable)", 1, m.blessingMaxHpBonus > 0 },
        { QString("Bourse +%1$").arg(m.blessingStartGold),
          "+25$ au depart de chaque run (cumulable)", 1, m.blessingStartGold > 0 },
        { "Skill bonus",
          "Commence avec 1 skill au choix parmi 3", 2, m.blessingFreeSkill }
    };
    int cardW = CW - 80, sy = 80;
    for (int i = 0; i < 3; ++i) {
        int x = 40, y = sy + i*60;
        bool sel = (i == hover);
        QColor bg = sel ? QColor(35, 28, 60) : QColor(20, 15, 35);
        p.fillRect(x, y, cardW, 54, bg);
        p.setPen(QPen(blessings[i].active ? QColor("#88ff88") : QColor("#666688"), sel?2:1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(x, y, cardW, 54); p.setPen(Qt::NoPen);
        QFont nf("monospace", 9, QFont::Bold); p.setFont(nf);
        p.setPen(blessings[i].active ? QColor("#88ffaa") : QColor("#ddccff"));
        p.drawText(x+10, y+18, blessings[i].name);
        QFont df("monospace", 7); p.setFont(df);
        p.setPen(QColor("#bbaacc"));
        p.drawText(x+10, y+34, blessings[i].desc);
        QFont cf("monospace", 8, QFont::Bold); p.setFont(cf);
        p.setPen(QColor("#ffcc44"));
        QString cost = QString("%1 frag").arg(blessings[i].cost);
        QFontMetrics cfm(cf);
        p.drawText(x+cardW - cfm.horizontalAdvance(cost) - 10, y+30, cost);
    }
    QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
    QString foot = "Haut/Bas selection - ESPACE acheter - ESC retour";
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - ffm.horizontalAdvance(foot)/2, CH-12, foot);
}

// =============== LEADERBOARD ===============
void MenuView::drawLeaderboard(QPainter &p, const GameModel &m)
{
    p.fillRect(0, 0, CW, CH, QColor(8, 5, 22));
    QFont tf("monospace", 14, QFont::Bold); p.setFont(tf); QFontMetrics tfm(tf);
    QString t = "MEILLEURS RUNS";
    p.setPen(C_GOLD); p.drawText(CW/2 - tfm.horizontalAdvance(t)/2, 30, t);
    QFont hf("monospace", 7, QFont::Bold); p.setFont(hf); QFontMetrics hfm(hf);
    p.setPen(QColor("#aaaaff"));
    int colX[5] = { 30, 65, 130, 250, 380 };
    p.drawText(colX[0], 52, "#");
    p.drawText(colX[1], 52, "SALLE");
    p.drawText(colX[2], 52, "CLASSE");
    p.drawText(colX[3], 52, "RELIQUE");
    p.drawText(colX[4], 52, "SKILLS");

    QFont rf("monospace", 8); p.setFont(rf);
    int y = 72;
    for (int i = 0; i < (int)m.leaderboard.size() && i < 10; ++i) {
        const LeaderEntry &e = m.leaderboard[i];
        QString className = (e.classId >= 0 && e.classId < (int)m.allClasses.size())
            ? m.allClasses[e.classId].name : QString("?");
        QString relic = (e.relicId >= 0 && e.relicId < (int)m.allRelics.size())
            ? m.allRelics[e.relicId].name : QString("aucune");
        QColor c = i < 3 ? C_GOLD : QColor("#ddccff");
        p.setPen(c);
        p.drawText(colX[0], y, QString::number(i+1));
        p.drawText(colX[1], y, QString("%1 (M%2)").arg(e.roomReached).arg(e.worldReached));
        p.drawText(colX[2], y, className);
        p.drawText(colX[3], y, relic);
        p.drawText(colX[4], y, QString::number(e.skillsCount));
        y += 14;
    }
    if (m.leaderboard.empty()) {
        QFont mf("monospace", 9); p.setFont(mf); QFontMetrics mfm(mf);
        QString msg = "Aucun run enregistre - lance ta premiere aventure !";
        p.setPen(QColor("#7766aa"));
        p.drawText(CW/2 - mfm.horizontalAdvance(msg)/2, CH/2, msg);
    }
    QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
    QString foot = "ESC : retour";
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - ffm.horizontalAdvance(foot)/2, CH-12, foot);
}

// =============== MENU PAUSE ===============
void MenuView::drawPauseMenu(QPainter &p, const GameModel &m)
{
    // Fond semi-transparent sur le jeu déjà rendu
    QColor bg(0, 0, 0); bg.setAlpha(200);
    p.fillRect(0, 0, CW, CH, bg);

    // Panneau central
    int px = 30, py = 20, pw = CW - 60, ph = CH - 40;
    p.fillRect(px, py, pw, ph, QColor(12, 8, 22));
    p.setPen(QPen(QColor("#5a3a8a"), 2)); p.setBrush(Qt::NoBrush);
    p.drawRect(px, py, pw, ph); p.setPen(Qt::NoPen);

    // Titre
    QFont tf("monospace", 14, QFont::Bold); p.setFont(tf); QFontMetrics tfm(tf);
    QString title = "PAUSE";
    p.setPen(C_GOLD); p.drawText(CW/2 - tfm.horizontalAdvance(title)/2, py + 22, title);

    // Onglets
    const char *tabNames[3] = { "Competences", "Inventaire", "Sorts" };
    QColor tabColors[3] = { QColor("#8844ff"), QColor("#44aaff"), QColor("#ff8844") };
    int tabW = 130, tabH = 24, tabY = py + 30, tabGap = 4;
    int totalTabW = 3*tabW + 2*tabGap;
    int tabStartX = CW/2 - totalTabW/2;
    for (int i = 0; i < 3; ++i) {
        int tx = tabStartX + i*(tabW+tabGap);
        bool sel = (m.pauseTab == i);
        QColor bg2 = tabColors[i]; bg2.setAlpha(sel ? 180 : 60);
        p.fillRect(tx, tabY, tabW, tabH, bg2);
        p.setPen(QPen(sel ? tabColors[i] : tabColors[i].darker(150), sel ? 2 : 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(tx, tabY, tabW, tabH);
        p.setPen(Qt::NoPen);
        QFont bf("monospace", 8, QFont::Bold); p.setFont(bf); QFontMetrics bfm(bf);
        QString lbl = QString("[%1] %2").arg(i+1).arg(tabNames[i]);
        p.setPen(sel ? tabColors[i] : tabColors[i].darker(120));
        p.drawText(tx + tabW/2 - bfm.horizontalAdvance(lbl)/2, tabY + 16, lbl);
        p.setPen(Qt::NoPen);
    }

    int contentY = tabY + tabH + 8;
    int contentH = ph - (contentY - py) - 30;

    // ── Onglet 0 : Compétences ──
    if (m.pauseTab == 0) {
        QFont hf("monospace", 8); p.setFont(hf); QFontMetrics hfm(hf);
        QString sub = QString("Power-ups actifs : %1/10").arg((int)m.player.skills.size());
        p.setPen(QColor("#aaaaff"));
        p.drawText(px + 10, contentY + 12, sub);

        int cols = 2, cellH = 32, cellW = (pw - 20) / cols;
        for (int i = 0; i < (int)m.player.skills.size() && i < 10; ++i) {
            int col = i % cols, row = i / cols;
            int cx = px + 10 + col * cellW;
            int cy = contentY + 22 + row * cellH;
            int sid = m.player.skills[i];
            const Skill *sk = nullptr;
            for (auto &s : m.allSkills) if (s.id == sid) { sk = &s; break; }
            if (!sk) continue;
            bool hover = (m.pauseSkillHover == i);
            QColor bg3 = sk->color; bg3.setAlpha(hover ? 90 : 40);
            p.fillRect(cx, cy, cellW - 4, cellH - 2, bg3);
            p.setPen(QPen(sk->color, hover ? 2 : 1)); p.setBrush(Qt::NoBrush);
            p.drawRect(cx, cy, cellW - 4, cellH - 2);
            p.setPen(Qt::NoPen);
            QFont nf("monospace", 8, QFont::Bold); p.setFont(nf);
            p.setPen(sk->color);
            p.drawText(cx + 6, cy + 12, sk->icon + " " + sk->name);
            QFont df("monospace", 6); p.setFont(df);
            p.setPen(QColor("#bbaaee"));
            // Tronque la desc si trop longue
            QString desc = sk->desc;
            QFontMetrics dfm(df);
            if (dfm.horizontalAdvance(desc) > cellW - 20)
                desc = desc.left(desc.length() * (cellW - 20) / dfm.horizontalAdvance(desc));
            p.drawText(cx + 6, cy + 24, desc);
            p.setPen(Qt::NoPen);
        }
        if (m.player.skills.empty()) {
            QFont mf("monospace", 8); p.setFont(mf); QFontMetrics mfm(mf);
            p.setPen(QColor("#664466"));
            QString msg = "Aucun power-up pour l'instant";
            p.drawText(CW/2 - mfm.horizontalAdvance(msg)/2, contentY + contentH/2, msg);
            p.setPen(Qt::NoPen);
        }
    }

    // ── Onglet 1 : Inventaire ──
    else if (m.pauseTab == 1) {
        int slotY = contentY + 6;
        const char *slotNames[3] = { "Arme", "Armure", "Accessoire" };
        QColor slotColors[3] = { QColor("#ff6644"), QColor("#44aaff"), QColor("#44ff88") };
        int slotW = (pw - 20) / 3, slotH = 56;

        for (int s = 0; s < EQ__COUNT; ++s) {
            int sx2 = px + 10 + s * slotW;
            const EquipItem &eq = m.player.equipped[s];
            QColor sc = slotColors[s]; sc.setAlpha(eq.empty ? 40 : 100);
            p.fillRect(sx2, slotY, slotW - 4, slotH, sc);
            p.setPen(QPen(slotColors[s], 2)); p.setBrush(Qt::NoBrush);
            p.drawRect(sx2, slotY, slotW - 4, slotH);
            p.setPen(Qt::NoPen);
            QFont lf("monospace", 7, QFont::Bold); p.setFont(lf);
            p.setPen(slotColors[s]);
            p.drawText(sx2 + 4, slotY + 12, slotNames[s]);
            if (!eq.empty) {
                QFont nf("monospace", 7, QFont::Bold); p.setFont(nf);
                p.setPen(eq.color);
                p.drawText(sx2 + 4, slotY + 26, eq.name);
                QFont df("monospace", 6); p.setFont(df);
                p.setPen(QColor("#ccbbee"));
                p.drawText(sx2 + 4, slotY + 38, eq.desc.left(20));
            } else {
                QFont ef("monospace", 7); p.setFont(ef);
                p.setPen(QColor("#554466"));
                p.drawText(sx2 + 4, slotY + 32, "(vide)");
            }
            p.setPen(Qt::NoPen);
        }

        // Sac
        int bagY = slotY + slotH + 12;
        QFont bh("monospace", 8, QFont::Bold); p.setFont(bh);
        p.setPen(QColor("#ffd544"));
        p.drawText(px + 10, bagY, QString("Sac (%1/8)").arg((int)m.player.bag.size()));
        bagY += 14;

        int itemW = (pw - 20) / 4, itemH = 48;
        for (int i = 0; i < (int)m.player.bag.size() && i < 8; ++i) {
            int col = i % 4, row = i / 4;
            int ix = px + 10 + col * itemW;
            int iy = bagY + row * (itemH + 4);
            const EquipItem &it = m.player.bag[i];
            bool hover = (m.pauseInvHover == i);
            QColor ic = it.color; ic.setAlpha(hover ? 120 : 60);
            p.fillRect(ix, iy, itemW - 4, itemH, ic);
            p.setPen(QPen(hover ? it.color : it.color.darker(130), hover ? 2 : 1));
            p.setBrush(Qt::NoBrush);
            p.drawRect(ix, iy, itemW - 4, itemH);
            p.setPen(Qt::NoPen);
            QFont nf("monospace", 7, QFont::Bold); p.setFont(nf);
            p.setPen(it.color);
            p.drawText(ix + 4, iy + 14, it.name);
            QFont df("monospace", 6); p.setFont(df);
            p.setPen(QColor("#ccbbee"));
            p.drawText(ix + 4, iy + 26, it.desc.left(18));
            if (hover) {
                p.setPen(QColor("#44ff88"));
                p.drawText(ix + 4, iy + 40, "[ESPACE] equiper");
            }
            p.setPen(Qt::NoPen);
        }
        if (m.player.bag.empty()) {
            QFont mf("monospace", 8); p.setFont(mf); QFontMetrics mfm(mf);
            p.setPen(QColor("#664466"));
            QString msg = "Sac vide - les ennemis droppent de l'equipement";
            p.drawText(px + 10, bagY + 24, msg);
            p.setPen(Qt::NoPen);
        }
    }

    // ── Onglet 2 : Sorts ──
    else if (m.pauseTab == 2) {
        int sy = contentY + 6;
        QFont sh("monospace", 8, QFont::Bold); p.setFont(sh);
        p.setPen(QColor("#ff8844"));
        p.drawText(px + 10, sy + 12, "Sorts equipes (touches 1-4 en jeu)");
        sy += 18;

        int spW = (pw - 20) / 4, spH = 80;
        for (int i = 0; i < 4; ++i) {
            int sx2 = px + 10 + i * spW;
            int spId = m.player.spellSlots[i];
            float cd = m.player.spellCds[i];
            const SpellInfo *sp = nullptr;
            for (auto &s : m.allSpells) if (s.id == spId) { sp = &s; break; }
            bool hover = (m.pauseSpellHover == i);
            QColor bc = sp ? sp->color : QColor("#555566");
            bc.setAlpha(hover ? 120 : 60);
            p.fillRect(sx2, sy, spW - 4, spH, bc);
            QColor borderC = sp ? sp->color : QColor("#444455");
            p.setPen(QPen(hover ? borderC : borderC.darker(130), hover ? 2 : 1));
            p.setBrush(Qt::NoBrush);
            p.drawRect(sx2, sy, spW - 4, spH);
            p.setPen(Qt::NoPen);

            QFont nf("monospace", 8, QFont::Bold); p.setFont(nf);
            p.setPen(QColor("#ffd544"));
            p.drawText(sx2 + 4, sy + 14, QString("[%1]").arg(i+1));

            if (sp) {
                p.setPen(sp->color);
                p.drawText(sx2 + 4, sy + 28, sp->name);
                QFont df("monospace", 6); p.setFont(df);
                p.setPen(QColor("#ccbbee"));
                p.drawText(sx2 + 4, sy + 40, sp->desc.left(22));
                if (cd > 0) {
                    p.setPen(QColor("#ff4444"));
                    p.drawText(sx2 + 4, sy + 54, QString("CD: %1s").arg(cd, 0, 'f', 1));
                } else {
                    p.setPen(QColor("#44ff88"));
                    p.drawText(sx2 + 4, sy + 54, "PRET");
                }
            } else {
                QFont ef("monospace", 7); p.setFont(ef);
                p.setPen(QColor("#554466"));
                p.drawText(sx2 + 4, sy + 40, "(vide)");
                p.setPen(QColor("#886688"));
                p.drawText(sx2 + 4, sy + 56, "Achetez au");
                p.drawText(sx2 + 4, sy + 66, "marche noir");
            }
            p.setPen(Qt::NoPen);
        }

        // Indication bouclier
        if (m.player.shieldMax > 0) {
            int shieldY = sy + spH + 14;
            QFont shf("monospace", 8, QFont::Bold); p.setFont(shf);
            p.setPen(QColor("#44aaff"));
            p.drawText(px + 10, shieldY, QString("Bouclier : %1 / %2").arg(m.player.shieldHp).arg(m.player.shieldMax));
        }
    }

    // Pied de page
    QFont fft("monospace", 7); p.setFont(fft); QFontMetrics fftm(fft);
    QString foot = "ESC : reprendre  -  1/2/3 : onglets  -  Fleches : naviguer  -  ESPACE : equiper";
    p.setPen(QColor("#7766aa"));
    p.drawText(CW/2 - fftm.horizontalAdvance(foot)/2, py + ph - 8, foot);
}

// =============== MARCHE NOIR ===============
void MenuView::drawBlackMarket(QPainter &p, const GameModel &m, int hover)
{
    // Fond opaque (le jeu est visible derrière via gameview::render)
    QColor bg(0, 0, 0); bg.setAlpha(210);
    p.fillRect(0, 0, CW, CH, bg);

    int px = 40, py = 30, pw = CW - 80, ph = CH - 60;
    p.fillRect(px, py, pw, ph, QColor(8, 4, 18));
    p.setPen(QPen(QColor("#aa44cc"), 2)); p.setBrush(Qt::NoBrush);
    p.drawRect(px, py, pw, ph); p.setPen(Qt::NoPen);

    QFont tf("monospace", 16, QFont::Bold); p.setFont(tf); QFontMetrics tfm(tf);
    QString title = "MARCHE NOIR";
    float pulse = 0.5f + 0.5f*std::sin(m.globalTime*2.5f);
    QColor tc = QColor(180 + (int)(pulse*60), 60, 220);
    p.setPen(tc);
    p.drawText(CW/2 - tfm.horizontalAdvance(title)/2, py + 28, title);

    QFont sf2("monospace", 7); p.setFont(sf2); QFontMetrics sfm(sf2);
    p.setPen(QColor("#aa88cc"));
    QString sub = "Achetez des sorts puissants - touches 1-4 pour acheter, 5-8 pour retirer";
    p.drawText(CW/2 - sfm.horizontalAdvance(sub)/2, py + 44, sub);

    // Or disponible
    QFont gf("monospace", 9, QFont::Bold); p.setFont(gf); QFontMetrics gfm(gf);
    p.setPen(QColor("#ffd544"));
    QString gold = QString("Or : %1 $").arg(m.player.gold);
    p.drawText(px + pw - gfm.horizontalAdvance(gold) - 10, py + 28, gold);

    // 4 cartes de sorts à vendre
    int n = std::min((int)m.blackMarketSpells.size(), 4);
    int cardW = (pw - 20) / std::max(1, n), cardH = 150, cardsY = py + 56, gap = 4;
    int totalW = n * cardW + (n-1) * gap;
    int startX = CW/2 - totalW/2;

    for (int i = 0; i < n; ++i) {
        int spId = m.blackMarketSpells[i];
        int price = (i < (int)m.blackMarketPrices.size()) ? m.blackMarketPrices[i] : 30;
        const SpellInfo *sp = nullptr;
        for (auto &s : m.allSpells) if (s.id == spId) { sp = &s; break; }
        if (!sp) continue;

        int cx = startX + i * (cardW + gap);
        bool sel = (hover == i);
        bool canAfford = m.player.gold >= price;

        QColor cardBg = sp->color; cardBg.setAlpha(sel ? 100 : 50);
        p.fillRect(cx, cardsY, cardW, cardH, cardBg);
        p.setPen(QPen(sel ? sp->color : sp->color.darker(140), sel ? 2 : 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(cx, cardsY, cardW, cardH);
        p.setPen(Qt::NoPen);

        // Numéro de touche
        QFont kf("monospace", 22, QFont::Bold); p.setFont(kf);
        QFontMetrics kfm(kf);
        QString k = QString::number(i+1);
        p.setPen(sel ? sp->color : sp->color.darker(130));
        p.drawText(cx + cardW/2 - kfm.horizontalAdvance(k)/2, cardsY + 32, k);

        // Nom du sort
        QFont nf("monospace", 9, QFont::Bold); p.setFont(nf);
        QFontMetrics nfm(nf);
        p.setPen(sp->color);
        p.drawText(cx + cardW/2 - nfm.horizontalAdvance(sp->name)/2, cardsY + 50, sp->name);

        // Description
        QFont df("monospace", 6); p.setFont(df); QFontMetrics dfm(df);
        p.setPen(QColor("#ccbbee"));
        QStringList words = sp->desc.split(' ');
        QString line; int dy = cardsY + 66;
        for (auto &w : words) {
            QString test = line.isEmpty() ? w : line + " " + w;
            if (dfm.horizontalAdvance(test) > cardW - 12) {
                p.drawText(cx + cardW/2 - dfm.horizontalAdvance(line)/2, dy, line);
                dy += 12; line = w;
            } else line = test;
        }
        if (!line.isEmpty())
            p.drawText(cx + cardW/2 - dfm.horizontalAdvance(line)/2, dy, line);

        // Prix
        QFont prf("monospace", 9, QFont::Bold); p.setFont(prf);
        QFontMetrics prfm(prf);
        QString priceStr = QString("%1 $").arg(price);
        QColor priceC = canAfford ? QColor("#ffd544") : QColor("#aa4422");
        p.setPen(priceC);
        p.drawText(cx + cardW/2 - prfm.horizontalAdvance(priceStr)/2, cardsY + cardH - 12, priceStr);
        p.setPen(Qt::NoPen);
    }

    // Sorts actuellement équipés (résumé)
    int slotY = cardsY + cardH + 14;
    QFont slh("monospace", 8, QFont::Bold); p.setFont(slh);
    p.setPen(QColor("#aa88cc"));
    p.drawText(px + 10, slotY, "Sorts equipes :");
    slotY += 14;

    for (int i = 0; i < 4; ++i) {
        int spId = m.player.spellSlots[i];
        const SpellInfo *sp = nullptr;
        for (auto &s : m.allSpells) if (s.id == spId) { sp = &s; break; }
        int slotX = px + 10 + i * (pw / 4);
        QFont sf3("monospace", 7, QFont::Bold); p.setFont(sf3);
        QColor slotC = sp ? sp->color : QColor("#554466");
        p.setPen(slotC);
        QString slotTxt = QString("[%1] %2").arg(i+1).arg(sp ? sp->name : "(vide)");
        p.drawText(slotX, slotY + 12, slotTxt);
        if (sp) {
            QFont rf("monospace", 6); p.setFont(rf);
            p.setPen(QColor("#887799"));
            p.drawText(slotX, slotY + 24, QString("[%1] retirer").arg(i+5));
        }
        p.setPen(Qt::NoPen);
    }

    // Pied de page
    QFont fft("monospace", 7); p.setFont(fft); QFontMetrics fftm(fft);
    QString foot = "ESC : quitter";
    p.setPen(QColor("#7766aa"));
    p.drawText(CW/2 - fftm.horizontalAdvance(foot)/2, py + ph - 8, foot);
}

}
