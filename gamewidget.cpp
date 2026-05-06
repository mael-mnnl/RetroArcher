#include "gamewidget.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QFont>
#include <QFontMetrics>
#include <QtMath>
#include <QtGlobal>
#include <QRandomGenerator>
#include <QSettings>
#include <algorithm>
#include <cmath>

// ============================================================
//  CONSTANTS
// ============================================================
namespace {
const int TILE = 32;
const int COLS = 17;
const int ROWS = 13;
const int GW   = COLS * TILE;   // 544
const int GH   = ROWS * TILE;   // 416
const int HUD  = 64;
const int CW   = GW;
const int CH   = GH + HUD;      // 480
const int DISPLAY_SCALE = 2;
const int ROOMS_TOTAL = 20;
const int DOOR_COL = COLS / 2;  // 8
const int DOOR_ROW = ROWS / 2;  // 6
}

const int GameWidget::s_walkFrameCount  = 8;
const int GameWidget::s_idleFrameCount  = 6;
const int GameWidget::s_hurtFrameCount  = 4;
const int GameWidget::s_deathFrameCount = 4;
const int GameWidget::s_atk1Frames      = 6;
const int GameWidget::s_atk2Frames      = 6;
const int GameWidget::s_atk3Frames      = 9;

// ============================================================
//  ROOM LAYOUTS  (15 cols x 11 rows inner area)
//  Col 7 et Row 5 toujours libres (couloirs portes)
// ============================================================
static const char* g_layouts[10][11] = {
    { "...............", "...............", "...............", "...............",
      "...............", "...............", "...............", "...............",
      "...............", "...............", "..............." },
    { "...............", ".###.......###.", ".###.......###.", "...............",
      "...............", "...............", "...............", "...............",
      ".###.......###.", ".###.......###.", "..............." },
    { "...............", "...............", "...#.......#...", "...............",
      "...............", "...............", "...............", "...............",
      "...#.......#...", "...............", "..............." },
    { "...............", "...............", ".......#.......", "......#.#......",
      ".....#...#.....", "...............", ".....#...#.....", "......#.#......",
      ".......#.......", "...............", "..............." },
    { "...............", ".#.#.......#.#.", "...............", "...............",
      "#...#.....#...#", "...............", "#...#.....#...#", "...............",
      "...............", ".#.#.......#.#.", "..............." },
    { "...............", "##...........##", "##...........##", "...............",
      "...............", "...............", "...............", "...............",
      "##...........##", "##...........##", "..............." },
    { "...............", "....#.....#....", "....#.....#....", "....#.....#....",
      "...............", "...............", "...............", "....#.....#....",
      "....#.....#....", "....#.....#....", "..............." },
    { "...............", "...####.####...", "...............", "...#.......#...",
      "...............", "...............", "...............", "...#.......#...",
      "...............", "...####.####...", "..............." },
    { "...............", ".#.............", "...#...........", ".....#...#.....",
      ".......#.......", "...............", ".......#.......", ".....#...#.....",
      "...........#...", ".............#.", "..............." },
    { "...............", ".###.....#####.", "...............", "...............",
      "....#.....#....", "...............", "....#.....#....", "...............",
      "...............", ".#####.....###.", "..............." },
};

// ============================================================
//  PALETTE
// ============================================================
static const QColor C_FL1   ("#3a2a1a");
static const QColor C_FL2   ("#302215");
static const QColor C_WT    ("#3d1e08");
static const QColor C_WF    ("#261408");
static const QColor C_DC    ("#7a5c0a");
static const QColor C_DO    ("#4a3408");
static const QColor C_PILL  ("#5a3818");
static const QColor C_PILL2 ("#3a2008");
static const QColor C_HUD   ("#0a0a18");
static const QColor C_HB    ("#c8a800");
static const QColor C_HP    ("#dd1a33");
static const QColor C_HPB   ("#3a0010");
static const QColor C_GOLD  ("#c8a800");
static const QColor C_WH    ("#e8e8e8");
static const QColor C_BE    ("#ff4400");
static const QColor C_BEG   ("#881100");

static const QColor PAL_SLIME[2] = { QColor("#1ab83a"), QColor("#0d7a22") };
static const QColor PAL_BONE[2]  = { QColor("#c8c8c8"), QColor("#7a7a7a") };
static const QColor PAL_FIRE[2]  = { QColor("#ffaa00"), QColor("#ff4400") };

// ============================================================
//  SKINS / ATTACK NAMES
// ============================================================
struct SkinDef { const char* name; int hue; double sat; double val; QColor accent; };
static const SkinDef g_skins[4] = {
    { "Bleu",            0, 1.0, 1.0,  QColor("#1a55bb") },
    { "Vert lierre",    90, 1.0, 0.95, QColor("#1a8833") },
    { "Rouge cramoisi",-110, 1.2, 0.9, QColor("#bb1a33") },
    { "Or imperial",  -150, 1.4, 1.05, QColor("#c8a800") },
};
static const char* g_atkNames[3] = { "Coup d'epee", "Estoc rapide", "Frappe lourde" };
static const char* g_atkDescs[3] = {
    "6 frames - cadence equilibree",
    "6 frames - rapide et precis",
    "9 frames - impact puissant"
};

// ============================================================
//  SKILLS  (15 total)
// ============================================================
enum SkillId {
    SK_DBL = 0, SK_TRI, SK_FST, SK_PRC, SK_SPD, SK_POW,
    SK_BNC, SK_DIA, SK_GRN,
    // Nouveaux v4
    SK_VAMP, SK_MAXHP, SK_CRIT, SK_SPLT, SK_FRZ, SK_QUAD
};

// ============================================================
//  HELPERS
// ============================================================
static float rndF(float a, float b) {
    return a + QRandomGenerator::global()->generateDouble() * (b - a);
}
static int rndI(int a, int b) {
    return QRandomGenerator::global()->bounded(a, b + 1);
}
static float clampF(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}
static float distF(float ax, float ay, float bx, float by) {
    return std::hypot(bx - ax, by - ay);
}

// ============================================================
//  CONSTRUCTOR
// ============================================================
GameWidget::GameWidget(QWidget *parent) : QWidget(parent)
{
    setFixedSize(CW * DISPLAY_SCALE, CH * DISPLAY_SCALE);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    loadAssets();
    buildSkinSprites();
    loadHighScore();

    for (auto &skill : std::vector<Skill>{
        { SK_DBL,  "Double Fleche",   "2 fleches paralleles",      "x2",  QColor("#44aaff") },
        { SK_TRI,  "Triple Fleche",   "3 fleches en eventail",     "x3",  QColor("#44ffbb") },
        { SK_FST,  "Tir Rapide",      "Cadence +35%",              ">>",  QColor("#ffff44") },
        { SK_PRC,  "Perforant",       "Fleches traversent",        "->|", QColor("#ff44aa") },
        { SK_SPD,  "Sprint",          "Vitesse +25%",              "~~",  QColor("#88ff88") },
        { SK_POW,  "Force",           "Degats +60%",               "!!",  QColor("#ff8844") },
        { SK_BNC,  "Ricochet",        "Fleches rebondissent",      "<>",  QColor("#ffaaff") },
        { SK_DIA,  "Oblique",         "Tir diagonal aussi",        "X",   QColor("#cc44ff") },
        { SK_GRN,  "Lance-grenade",   "Bombe G - manche 4+",       "*",   QColor("#ff6622") },
        { SK_VAMP, "Vampirisme",      "20% soin par kill",         "+V",  QColor("#aa1144") },
        { SK_MAXHP,"Vitalite",        "+2 PV max + soin total",    "++",  QColor("#ff3366") },
        { SK_CRIT, "Critique",        "25% chance x2 degats",      "x!",  QColor("#ffdd00") },
        { SK_SPLT, "Eclats",          "Fleches eclatent en 2",     "<*",  QColor("#66ddff") },
        { SK_FRZ,  "Glacant",         "Ralentit ennemis 2s",       "*~",  QColor("#88ccff") },
        { SK_QUAD, "Quadruple",       "4 fleches paralleles",      "x4",  QColor("#ff77ff") },
    }) m_allSkills.push_back(skill);

    m_elapsed.start();
    m_lastTickNs = m_elapsed.nsecsElapsed();
    connect(&m_timer, &QTimer::timeout, this, &GameWidget::tick);
    m_timer.start(16);
}

GameWidget::~GameWidget() {}

// ============================================================
//  ASSETS
// ============================================================
QImage GameWidget::hueShift(const QImage &src, int degrees, double sat, double val)
{
    QImage img = src.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < img.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            QColor c = QColor::fromRgba(line[x]);
            if (c.alpha() == 0) continue;
            int h, s, v, a;
            c.getHsv(&h, &s, &v, &a);
            if (h < 0) {
                v = std::min(255, std::max(0, int(v * val)));
                c.setHsv(0, 0, v, a);
            } else {
                h = (h + degrees + 360) % 360;
                s = std::min(255, std::max(0, int(s * sat)));
                v = std::min(255, std::max(0, int(v * val)));
                c.setHsv(h, s, v, a);
            }
            line[x] = c.rgba();
        }
    }
    return img;
}

void GameWidget::loadAssets()
{
    m_sprSoldierBase[AN_Idle]  = QImage(":/sprites/soldier_idle.png");
    m_sprSoldierBase[AN_Walk]  = QImage(":/sprites/soldier_walk.png");
    m_sprSoldierBase[AN_Atk]   = QImage(":/sprites/soldier_atk1.png");
    m_sprSoldierBase[AN_Hurt]  = QImage(":/sprites/soldier_hurt.png");
    m_sprSoldierBase[AN_Death] = QImage(":/sprites/soldier_death.png");
    m_sprSoldierAtkVariants[0] = QImage(":/sprites/soldier_atk1.png");
    m_sprSoldierAtkVariants[1] = QImage(":/sprites/soldier_atk2.png");
    m_sprSoldierAtkVariants[2] = QImage(":/sprites/soldier_atk3.png");
    m_sprArrow = QImage(":/sprites/arrow.png");

    QImage rawOrc[5];
    rawOrc[AN_Idle]  = QImage(":/sprites/orc_idle.png");
    rawOrc[AN_Walk]  = QImage(":/sprites/orc_walk.png");
    rawOrc[AN_Atk]   = QImage(":/sprites/orc_atk.png");
    rawOrc[AN_Hurt]  = QImage(":/sprites/orc_hurt.png");
    rawOrc[AN_Death] = QImage(":/sprites/orc_death.png");
    for (int a = 0; a < 5; ++a) {
        m_sprOrc[0][a] = hueShift(rawOrc[a], 120, 1.8, 1.0);
        m_sprOrc[1][a] = hueShift(rawOrc[a],  40, 0.4, 1.3);
        m_sprOrc[2][a] = hueShift(rawOrc[a], 260, 2.0, 1.0);
        m_sprOrc[3][a] = hueShift(rawOrc[a],   0, 0.6, 0.7);
        m_sprOrc[4][a] = hueShift(rawOrc[a], 200, 2.0, 1.0);
        m_sprOrc[5][a] = hueShift(rawOrc[a], 350, 2.0, 0.9);
    }
    loadBossGallery();
}

void GameWidget::loadBossGallery()
{
    // Boss 1 – salle  5 – Ver de Feu        (810×90,  9 frames 90×90)
    { BossInfo b; b.idleSheet=QImage(":/sprites/boss_fireworm.png");
      b.frameCount=9; b.frameW=90; b.frameH=90; b.fps=8.f;
      b.nameFr="Ver de Feu"; b.roomTxt="Salle 5"; b.accent=QColor("#ff6600");
      m_bossGallery.append(b); }
    // Boss 2 – salle  9 – Executeur des Morts (500×100, 5 frames 100×100)
    { BossInfo b; b.idleSheet=QImage(":/sprites/boss_undead.png");
      b.frameCount=5; b.frameW=100; b.frameH=100; b.fps=7.f;
      b.nameFr="Executeur des Morts"; b.roomTxt="Salle 9"; b.accent=QColor("#88aacc");
      m_bossGallery.append(b); }
    // Boss 3 – salle 13 – Demon Vase        (1728×160, 6 frames 288×160)
    { BossInfo b; b.idleSheet=QImage(":/sprites/boss_demonslime.png");
      b.frameCount=6; b.frameW=288; b.frameH=160; b.fps=8.f;
      b.nameFr="Demon Vase"; b.roomTxt="Salle 13"; b.accent=QColor("#aa44ff");
      m_bossGallery.append(b); }
    // Boss 4 – salle 17 – Minotaure         (4608×160, 16 frames 288×160)
    { BossInfo b; b.idleSheet=QImage(":/sprites/boss_mino.png");
      b.frameCount=16; b.frameW=288; b.frameH=160; b.fps=10.f;
      b.nameFr="Minotaure"; b.roomTxt="Salle 17"; b.accent=QColor("#cc8822");
      m_bossGallery.append(b); }
    // Boss 5 – salle 20 – Gardien du Givre  (1152×128, 6 frames 192×128)
    { BossInfo b; b.idleSheet=QImage(":/sprites/boss_frostguardian.png");
      b.frameCount=6; b.frameW=192; b.frameH=128; b.fps=8.f;
      b.nameFr="Gardien du Givre"; b.roomTxt="Salle 20 - FINAL"; b.accent=QColor("#44ccff");
      m_bossGallery.append(b); }
}

void GameWidget::buildSkinSprites()
{
    for (int s = 0; s < 4; ++s) {
        const SkinDef &sd = g_skins[s];
        for (int a = 0; a < 5; ++a)
            m_sprSoldierSkin[s][a] = hueShift(m_sprSoldierBase[a], sd.hue, sd.sat, sd.val);
        for (int v = 0; v < 3; ++v)
            m_sprSoldierAtkSkin[s][v] = hueShift(m_sprSoldierAtkVariants[v], sd.hue, sd.sat, sd.val);
    }
}

QImage GameWidget::soldierSheet(AnimType anim) const {
    int s = m_player.skinIndex;
    if (anim == AN_Atk) return m_sprSoldierAtkSkin[s][m_player.atkVariant];
    return m_sprSoldierSkin[s][anim];
}
int GameWidget::soldierFrameCount(AnimType anim) const {
    if (anim == AN_Idle) return s_idleFrameCount;
    if (anim == AN_Walk) return s_walkFrameCount;
    if (anim == AN_Hurt) return s_hurtFrameCount;
    if (anim == AN_Death) return s_deathFrameCount;
    if (m_player.atkVariant == 0) return s_atk1Frames;
    if (m_player.atkVariant == 1) return s_atk2Frames;
    return s_atk3Frames;
}
float GameWidget::soldierFps(AnimType anim) const {
    if (anim == AN_Idle) return 8.f;
    if (anim == AN_Walk) return 12.f;
    if (anim == AN_Hurt) return 10.f;
    if (anim == AN_Death) return 7.f;
    if (m_player.atkVariant == 0) return 14.f;
    if (m_player.atkVariant == 1) return 18.f;
    return 13.f;
}

// ============================================================
//  PERSISTENCE
// ============================================================
void GameWidget::loadHighScore() {
    QSettings s("RetroArcher", "RetroArcher");
    m_highScore = s.value("highScore", 0).toInt();
    m_menuSelectedSkin = s.value("skin", 0).toInt() % 4;
    m_menuSelectedAtk  = s.value("atk",  0).toInt() % 3;
}
void GameWidget::saveHighScore(int roomReached) {
    if (roomReached > m_highScore) {
        m_highScore = roomReached;
        QSettings s("RetroArcher", "RetroArcher");
        s.setValue("highScore", m_highScore);
    }
}

// ============================================================
//  ROOM HELPERS
// ============================================================
bool GameWidget::isMiniBossRoom(int r) const { return r==5||r==9||r==13||r==17; }
bool GameWidget::isFinalBossRoom(int r) const { return r==ROOMS_TOTAL; }
bool GameWidget::isAnyBossRoom(int r) const { return isMiniBossRoom(r)||isFinalBossRoom(r); }

// ============================================================
//  GAME START / BUILD ROOM
// ============================================================
void GameWidget::startGame()
{
    m_player = Player();
    m_player.skinIndex  = m_menuSelectedSkin;
    m_player.atkVariant = m_menuSelectedAtk;
    m_player.x = GW / 2.f;
    m_player.y = GH / 2.f;
    m_enemies.clear(); m_bullets.clear(); m_particles.clear();
    m_currentRoom = 1;
    m_bossIndex   = -1;
    m_state       = GS_Playing;
    m_fadeAmount  = 0;
    buildRoom(1, -1);
}

void GameWidget::buildRoom(int roomIndex, int entryDoor)
{
    m_currentRoom = roomIndex;
    m_enemies.clear(); m_bullets.clear();
    m_bossIndex = -1;
    for (int i = 0; i < 4; ++i) m_doorOpen[i] = false;

    // Place player at entry door
    if      (entryDoor==0) { m_player.x=DOOR_COL*TILE+TILE/2.f; m_player.y=TILE*2.5f; }
    else if (entryDoor==1) { m_player.x=GW-TILE*2.5f; m_player.y=DOOR_ROW*TILE+TILE/2.f; }
    else if (entryDoor==2) { m_player.x=DOOR_COL*TILE+TILE/2.f; m_player.y=GH-TILE*2.5f; }
    else if (entryDoor==3) { m_player.x=TILE*2.5f; m_player.y=DOOR_ROW*TILE+TILE/2.f; }
    else                   { m_player.x=GW/2.f; m_player.y=GH/2.f; }

    // Layouts : vide pour l'instant (enleve les murs invisibles)
    // Pour reactiver les layouts aleatoires, remplacer 0 par : rndI(0,9)
    m_layoutIdx = 0;
    resolveCollision(m_player.x, m_player.y, 8.f);  // rayon reel = 8px

    if (isFinalBossRoom(roomIndex)) {
        Enemy boss; boss.id=m_nextEnemyId++;
        boss.x=GW/2.f; boss.y=GH/2.f-40;
        boss.hp=boss.maxHp=1200; boss.speed=70; boss.damage=2; boss.size=70;
        boss.type=ET_FinalBoss; boss.phase=1;
        boss.shootCd=0.8f; boss.burstCd=4; boss.specialCd=7;
        boss.moveCd=rndF(0.6f,1.2f); boss.moveTargetX=boss.x; boss.moveTargetY=boss.y;
        m_enemies.push_back(boss);
        m_bossIndex=(int)m_enemies.size()-1;
        return;
    }
    if (isMiniBossRoom(roomIndex)) {
        Enemy boss; boss.id=m_nextEnemyId++;
        boss.x=GW/2.f; boss.y=GH/2.f-30;
        int mbIdx=0;
        if (roomIndex==9) mbIdx=1; else if (roomIndex==13) mbIdx=2; else if (roomIndex==17) mbIdx=3;
        boss.subType=mbIdx;
        float hpV[4]={250,380,520,700}; float spdV[4]={50,60,65,70};
        float scdV[4]={1.4f,1.1f,0.95f,0.8f}; float bcdV[4]={7,6,5,4};
        boss.hp=boss.maxHp=hpV[mbIdx]; boss.speed=spdV[mbIdx];
        boss.shootCd=scdV[mbIdx]; boss.burstCd=bcdV[mbIdx];
        boss.damage=1+mbIdx; boss.size=56+mbIdx*4;
        boss.type=ET_MiniBoss; boss.phase=1;
        boss.moveCd=rndF(0.8f,1.6f); boss.moveTargetX=boss.x; boss.moveTargetY=boss.y;
        m_enemies.push_back(boss);
        m_bossIndex=(int)m_enemies.size()-1;
        return;
    }

    int count = 3 + std::min(8, roomIndex/2+1);
    for (int i = 0; i < count; ++i) {
        float ex,ey; int safety=50;
        do {
            ex=rndF(TILE*2.5f,GW-TILE*2.5f); ey=rndF(TILE*2.5f,GH-TILE*2.5f); safety--;
        } while ((distF(ex,ey,m_player.x,m_player.y)<130.f||collidesObstacle(ex,ey,18))&&safety>0);

        Enemy e; e.id=m_nextEnemyId++; e.x=ex; e.y=ey;
        float r=QRandomGenerator::global()->generateDouble();

        if (roomIndex<=2) {
            e.type=ET_Slime; e.hp=e.maxHp=30; e.speed=rndF(45,65); e.size=32;
        } else if (roomIndex<=4) {
            if (r<0.55) { e.type=ET_Slime; e.hp=e.maxHp=30; e.speed=rndF(45,65); e.size=32; }
            else { e.type=ET_Bat; e.hp=e.maxHp=20; e.speed=rndF(50,70); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
        } else if (roomIndex<=8) {
            if (r<0.35) { e.type=ET_Slime; e.hp=e.maxHp=35; e.speed=rndF(50,70); e.size=32; }
            else if (r<0.65) { e.type=ET_Skel; e.hp=e.maxHp=50; e.speed=rndF(40,55); e.size=32; e.shootCd=rndF(1.5f,2.5f); }
            else { e.type=ET_Bat; e.hp=e.maxHp=25; e.speed=rndF(55,75); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
        } else if (roomIndex<=12) {
            if (r<0.25) { e.type=ET_Slime; e.hp=e.maxHp=45; e.speed=rndF(55,75); e.size=32; }
            else if (r<0.45) { e.type=ET_Skel; e.hp=e.maxHp=60; e.speed=rndF(45,60); e.size=32; e.shootCd=rndF(1.3f,2.2f); }
            else if (r<0.65) { e.type=ET_Bat; e.hp=e.maxHp=30; e.speed=rndF(60,80); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
            else if (r<0.85) { e.type=ET_Brute; e.hp=e.maxHp=90; e.speed=rndF(35,50); e.size=36; e.damage=2; }
            else { e.type=ET_Mage; e.hp=e.maxHp=55; e.speed=rndF(35,50); e.size=32; e.shootCd=rndF(1.8f,2.8f); }
        } else {
            if (r<0.15) { e.type=ET_Slime; e.hp=e.maxHp=60; e.speed=rndF(60,80); e.size=32; }
            else if (r<0.30) { e.type=ET_Skel; e.hp=e.maxHp=75; e.speed=rndF(50,65); e.size=32; e.shootCd=rndF(1.1f,2.0f); }
            else if (r<0.50) { e.type=ET_Bat; e.hp=e.maxHp=40; e.speed=rndF(65,90); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
            else if (r<0.75) { e.type=ET_Brute; e.hp=e.maxHp=130; e.speed=rndF(40,55); e.size=36; e.damage=2; }
            else { e.type=ET_Mage; e.hp=e.maxHp=80; e.speed=rndF(40,55); e.size=32; e.shootCd=rndF(1.4f,2.4f); }
        }
        e.anim=AN_Walk;
        m_enemies.push_back(e);
    }
}

// ============================================================
//  COLLISION
// ============================================================
bool GameWidget::isObstacleTile(int col, int row) const
{
    if (col<0||col>=COLS||row<0||row>=ROWS) return true;
    bool isPerim=(col==0||col==COLS-1||row==0||row==ROWS-1);
    if (isPerim) {
        if (row==0      && col==DOOR_COL  && m_doorOpen[0]) return false;
        if (col==COLS-1 && row==DOOR_ROW  && m_doorOpen[1]) return false;
        if (row==ROWS-1 && col==DOOR_COL  && m_doorOpen[2]) return false;
        if (col==0      && row==DOOR_ROW  && m_doorOpen[3]) return false;
        return true;
    }
    int ic=col-1, ir=row-1;
    if (ic<0||ic>14||ir<0||ir>10) return false;
    return g_layouts[m_layoutIdx][ir][ic]=='#';
}

bool GameWidget::collidesObstacle(float x, float y, float r) const
{
    int c0=std::max(0,int((x-r)/TILE)), c1=std::min(COLS-1,int((x+r)/TILE));
    int r0=std::max(0,int((y-r)/TILE)), r1=std::min(ROWS-1,int((y+r)/TILE));
    for (int row=r0;row<=r1;++row)
        for (int col=c0;col<=c1;++col) {
            if (!isObstacleTile(col,row)) continue;
            float tx0=col*TILE,ty0=row*TILE,tx1=tx0+TILE,ty1=ty0+TILE;
            float cx=clampF(x,tx0,tx1), cy=clampF(y,ty0,ty1);
            float dx=x-cx, dy=y-cy;
            if (dx*dx+dy*dy<r*r) return true;
        }
    return false;
}

void GameWidget::resolveCollision(float &x, float &y, float r)
{
    for (int iter=0;iter<8;++iter) {
        bool collided=false;
        int c0=std::max(0,int((x-r)/TILE)), c1=std::min(COLS-1,int((x+r)/TILE));
        int r0=std::max(0,int((y-r)/TILE)), r1=std::min(ROWS-1,int((y+r)/TILE));
        for (int row=r0;row<=r1;++row)
            for (int col=c0;col<=c1;++col) {
                if (!isObstacleTile(col,row)) continue;
                float tx0=col*TILE,ty0=row*TILE,tx1=tx0+TILE,ty1=ty0+TILE;
                float cx=clampF(x,tx0,tx1),cy=clampF(y,ty0,ty1);
                float dx=x-cx,dy=y-cy,d2=dx*dx+dy*dy;
                if (d2<r*r&&d2>0.0001f) {
                    float d=std::sqrt(d2),push=r-d+0.5f;
                    x+=(dx/d)*push; y+=(dy/d)*push; collided=true;
                } else if (d2<0.0001f) {
                    float dL=std::abs(x-tx0),dR=std::abs(tx1-x),dT=std::abs(y-ty0),dB=std::abs(ty1-y);
                    float m=std::min({dL,dR,dT,dB});
                    if (m==dL) x=tx0-r-0.5f; else if (m==dR) x=tx1+r+0.5f;
                    else if (m==dT) y=ty0-r-0.5f; else y=ty1+r+0.5f;
                    collided=true;
                }
            }
        if (!collided) break;
    }
}

void GameWidget::tryMove(float &x, float &y, float dx, float dy, float r)
{
    float dist=std::hypot(dx,dy);
    int steps=std::max(1,int(std::ceil(dist/(TILE*0.4f))));
    float sx=dx/steps, sy=dy/steps;
    for (int i=0;i<steps;++i) {
        float nx=x+sx;
        if (!collidesObstacle(nx,y,r)) x=nx;
        float ny=y+sy;
        if (!collidesObstacle(x,ny,r)) y=ny;
    }
}


// ============================================================
//  TICK / UPDATE LOOP
// ============================================================
void GameWidget::tick()
{
    qint64 now=m_elapsed.nsecsElapsed();
    float dt=std::min(0.05f,float(now-m_lastTickNs)/1e9f);
    m_lastTickNs=now;
    m_globalTime+=dt;

    if (m_state==GS_Playing||m_state==GS_RoomCleared) {
        updateGame(dt);
    } else if (m_state==GS_FadeOut) {
        m_fadeAmount+=dt/0.4f;
        if (m_fadeAmount>=1.f) {
            m_fadeAmount=1.f;
            if (m_currentRoom+1>ROOMS_TOTAL) { m_state=GS_Victory; saveHighScore(ROOMS_TOTAL); }
            else { generateSkills(); m_state=GS_SkillSelect; }
        }
    } else if (m_state==GS_FadeIn) {
        m_fadeAmount-=dt/0.4f;
        if (m_fadeAmount<=0.f) { m_fadeAmount=0.f; m_state=GS_Playing; }
        for (auto &p:m_particles) { p.x+=p.vx*dt; p.y+=p.vy*dt; p.life-=dt; }
    }
    update();
}

void GameWidget::updateGame(float dt)
{
    updatePlayer(dt);
    updateEnemies(dt);
    updateBullets(dt);
    updateParticles(dt);

    for (auto &e:m_enemies)
        if (e.dead&&e.anim==AN_Death) { e.corpseFadeTimer+=dt; if(e.corpseFadeTimer>0.6f) e.corpse=true; }
    m_enemies.erase(std::remove_if(m_enemies.begin(),m_enemies.end(),[](const Enemy &e){return e.corpse;}),m_enemies.end());

    if (m_player.hp<=0&&m_state!=GS_GameOver) { m_state=GS_GameOver; saveHighScore(m_currentRoom); return; }

    if (m_state==GS_Playing) {
        bool anyAlive=false;
        for (auto &e:m_enemies) if(!e.dead){anyAlive=true;break;}
        if (!anyAlive) {
            if (isFinalBossRoom(m_currentRoom)) { m_state=GS_Victory; saveHighScore(ROOMS_TOTAL); return; }
            m_state=GS_RoomCleared;
            for (int i=0;i<4;++i) m_doorOpen[i]=true;
        }
    }
    if (m_state==GS_RoomCleared) checkDoorTransition();
}

// ============================================================
//  PLAYER UPDATE
// ============================================================
void GameWidget::updatePlayer(float dt)
{
    Player &pl=m_player;
    pl.animTimer+=dt;
    if (pl.atkTimer>0) pl.atkTimer-=dt;
    if (pl.invincibility>0) pl.invincibility-=dt;
    if (pl.grenadeCd>0) pl.grenadeCd-=dt;

    float vx=0,vy=0;
    if (m_keys.contains(Qt::Key_Left) ||m_keys.contains(Qt::Key_A)||m_keys.contains(Qt::Key_Q)) vx-=1;
    if (m_keys.contains(Qt::Key_Right)||m_keys.contains(Qt::Key_D)) vx+=1;
    if (m_keys.contains(Qt::Key_Up)   ||m_keys.contains(Qt::Key_W)||m_keys.contains(Qt::Key_Z)) vy-=1;
    if (m_keys.contains(Qt::Key_Down) ||m_keys.contains(Qt::Key_S)) vy+=1;

    float len=std::hypot(vx,vy);
    pl.moving=(len>0);
    if (pl.moving) {
        vx/=len; vy/=len;
        // CORRECTION BUG : rayon 8px (pas pl.size=22) pour eviter les murs invisibles
        tryMove(pl.x,pl.y,vx*pl.speed*dt,vy*pl.speed*dt,8.f);
        pl.facing=std::atan2(vy,vx); pl.facingLeft=vx<0;
        if (pl.anim!=AN_Walk){pl.anim=AN_Walk;pl.animTimer=0;}
    } else {
        if (pl.atkTimer<=0&&pl.anim!=AN_Hurt) pl.anim=AN_Idle;
    }
    pl.x=clampF(pl.x,-TILE,GW+TILE); pl.y=clampF(pl.y,-TILE,GH+TILE);

    pl.fireCd-=dt;
    bool anyEnemy=false;
    for (auto &e:m_enemies) if(!e.dead){anyEnemy=true;break;}
    if (!pl.moving&&pl.fireCd<=0&&anyEnemy) {
        pl.fireCd=pl.fireRate; shootPlayer();
        pl.anim=AN_Atk; pl.animTimer=0; pl.atkTimer=0.4f;
    }
    if (pl.moving&&pl.fireCd<pl.fireRate*0.5f) pl.fireCd=pl.fireRate*0.5f;

    if (m_keys.contains(Qt::Key_G)&&pl.grenadeAmmo>0&&pl.grenadeCd<=0) {
        throwGrenade(); pl.grenadeAmmo--; pl.grenadeCd=0.3f;
    }

    // Dommages au contact – hitboxes reduites (rayon 10 + fraction taille enemi)
    if (pl.invincibility<=0) {
        for (auto &e:m_enemies) {
            if (e.dead) continue;
            if (distF(pl.x,pl.y,e.x,e.y)<10+e.size*0.35f) { hurtPlayer(e.damage); break; }
        }
        for (auto &b:m_bullets) {
            if (!b.enemy||b.dead) continue;
            if (distF(pl.x,pl.y,b.x,b.y)<12) { b.dead=true; hurtPlayer(b.damage); }
        }
    }
}

// ============================================================
//  SHOOT / GRENADE
// ============================================================
void GameWidget::shootPlayer()
{
    Player &pl=m_player;
    Enemy *near=nullptr; float nd=1e9f;
    for (auto &e:m_enemies) { if(e.dead) continue; float d=distF(pl.x,pl.y,e.x,e.y); if(d<nd){nd=d;near=&e;} }
    if (!near) return;

    auto hasSkill=[&](int id){ return std::find(pl.skills.begin(),pl.skills.end(),id)!=pl.skills.end(); };
    bool prc=hasSkill(SK_PRC), bnc=hasSkill(SK_BNC), tri=hasSkill(SK_TRI);
    bool dbl=hasSkill(SK_DBL), dia=hasSkill(SK_DIA), pow=hasSkill(SK_POW);
    bool crit=hasSkill(SK_CRIT), quad=hasSkill(SK_QUAD);

    float spd=320.f;
    float dmg=pl.damage*(pow?1.6f:1.f);
    // SK_CRIT : 25% chance x2
    if (crit&&QRandomGenerator::global()->generateDouble()<0.25) dmg*=2.f;

    float a=std::atan2(near->y-pl.y,near->x-pl.x);
    pl.facing=a; pl.facingLeft=std::cos(a)<0;

    auto fire=[&](float ang,float ox=0,float oy=0){
        Bullet b; b.x=pl.x+ox; b.y=pl.y+oy;
        b.vx=std::cos(ang)*spd; b.vy=std::sin(ang)*spd;
        b.angle=ang; b.damage=dmg; b.pierce=prc; b.bounce=bnc?1:0; b.enemy=false;
        m_bullets.push_back(b);
    };

    // SK_QUAD : 4 fleches paralleles (prioritaire sur tri/dbl)
    if (quad) {
        float perp=a+M_PI/2;
        for (float off:{-18.f,-6.f,6.f,18.f})
            fire(a,std::cos(perp)*off,std::sin(perp)*off);
    } else if (tri) {
        fire(a-0.28f); fire(a); fire(a+0.28f);
    } else if (dbl) {
        float perp=a+M_PI/2; float off=8.f;
        fire(a,std::cos(perp)*off,std::sin(perp)*off);
        fire(a,-std::cos(perp)*off,-std::sin(perp)*off);
    } else {
        fire(a);
    }
    if (dia) { fire(a+M_PI/4); fire(a-M_PI/4); }
}

void GameWidget::throwGrenade()
{
    float a=m_player.facing;
    Bullet b; b.x=m_player.x; b.y=m_player.y;
    b.vx=std::cos(a)*220; b.vy=std::sin(a)*220;
    b.angle=a; b.damage=0; b.enemy=false;
    b.grenade=true; b.grenadeFuse=1.0f; b.grenadeTotal=1.0f;
    m_bullets.push_back(b);
}

// ============================================================
//  HURT
// ============================================================
void GameWidget::hurtPlayer(float d)
{
    if (m_player.invincibility>0) return;
    m_player.hp=std::max(0.f,m_player.hp-d);
    m_player.invincibility=0.8f;
    m_player.anim=AN_Hurt; m_player.animTimer=0; m_player.atkTimer=0.4f;
    for (int i=0;i<8;++i) {
        Particle p; p.x=m_player.x+rndF(-10,10); p.y=m_player.y+rndF(-10,10);
        p.vx=rndF(-100,100); p.vy=rndF(-140,-40);
        p.color=C_HP; p.life=p.maxLife=rndF(0.3f,0.6f); p.size=(int)rndF(3,6);
        m_particles.push_back(p);
    }
}

void GameWidget::hurtEnemy(Enemy &e, float dmg)
{
    e.hp-=dmg; e.hitFlash=0.18f;

    // SK_FRZ : ralentit l'ennemi
    bool hasFrz=std::find(m_player.skills.begin(),m_player.skills.end(),SK_FRZ)!=m_player.skills.end();
    if (hasFrz) e.slowTimer=std::min(e.slowTimer+2.f,(e.type==ET_FinalBoss)?1.f:2.f);

    const QColor *pal;
    if (e.type==ET_Slime) pal=PAL_SLIME;
    else if (e.type==ET_Mage||e.type==ET_FinalBoss) pal=PAL_FIRE;
    else pal=PAL_BONE;

    for (int i=0;i<5;++i) {
        Particle p; p.x=e.x+rndF(-8,8); p.y=e.y+rndF(-8,8);
        p.vx=rndF(-70,70); p.vy=rndF(-110,-20);
        p.color=hasFrz?QColor("#aaddff"):pal[rndI(0,1)];
        p.life=p.maxLife=rndF(0.3f,0.6f); p.size=(int)rndF(2,5);
        m_particles.push_back(p);
    }
    if (e.hp<=0) {
        e.dead=true; e.anim=AN_Death; e.animTimer=0;
        // SK_VAMP : 20% soin par kill
        bool hasVamp=std::find(m_player.skills.begin(),m_player.skills.end(),SK_VAMP)!=m_player.skills.end();
        if (hasVamp&&QRandomGenerator::global()->generateDouble()<0.20) {
            m_player.hp=std::min(m_player.maxHp,m_player.hp+1.f);
            for (int i=0;i<6;++i) {
                Particle pp; pp.x=m_player.x+rndF(-6,6); pp.y=m_player.y+rndF(-12,0);
                pp.vx=rndF(-30,30); pp.vy=rndF(-80,-30);
                pp.color=QColor("#ff66aa"); pp.life=pp.maxLife=rndF(0.4f,0.7f); pp.size=(int)rndF(3,5);
                m_particles.push_back(pp);
            }
        }
        for (int i=0;i<12;++i) {
            float ang=(i/12.f)*2*M_PI;
            Particle p; p.x=e.x; p.y=e.y;
            p.vx=std::cos(ang)*rndF(50,130); p.vy=std::sin(ang)*rndF(50,130);
            p.color=pal[rndI(0,1)]; p.life=p.maxLife=rndF(0.4f,0.8f); p.size=(int)rndF(3,6);
            m_particles.push_back(p);
        }
    } else {
        e.anim=AN_Hurt; e.animTimer=0;
    }
}

// ============================================================
//  ENEMY AI
// ============================================================
void GameWidget::updateEnemies(float dt)
{
    for (auto &e:m_enemies) {
        if (e.hitFlash>0) e.hitFlash-=dt;
        if (e.slowTimer>0) e.slowTimer-=dt;
        if (e.dead) { e.animTimer+=dt; continue; }
        // Temps ralenti pour ennemis gelés
        float adt=dt*(e.slowTimer>0?0.5f:1.0f);
        e.animTimer+=adt;
        float dx=m_player.x-e.x, dy=m_player.y-e.y, d=std::hypot(dx,dy);
        float collR=e.size*0.4f;

        if (e.type==ET_Slime) {
            if (d>0) { tryMove(e.x,e.y,dx/d*e.speed*adt,dy/d*e.speed*adt,collR); e.facingLeft=dx<0; }
            if (e.anim==AN_Hurt&&e.animTimer>0.4f){e.anim=AN_Walk;e.animTimer=0;} else if(e.anim!=AN_Hurt) e.anim=AN_Walk;
        } else if (e.type==ET_Skel) {
            e.aimAngle=std::atan2(dy,dx); e.facingLeft=dx<0;
            if (d>0) {
                float ds=0; if(d<150) ds=-0.4f; else if(d>220) ds=1.f;
                if (ds!=0) tryMove(e.x,e.y,dx/d*e.speed*ds*adt,dy/d*e.speed*ds*adt,collR);
            }
            e.shootCd-=adt;
            if (e.shootCd<=0&&d<320) {
                e.shootCd=rndF(1.8f,2.8f);
                Bullet b; b.x=e.x; b.y=e.y; b.vx=std::cos(e.aimAngle)*175; b.vy=std::sin(e.aimAngle)*175;
                b.angle=e.aimAngle; b.damage=1; b.enemy=true; m_bullets.push_back(b);
            }
            if (e.anim==AN_Hurt&&e.animTimer>0.4f){e.anim=AN_Walk;e.animTimer=0;} else if(e.anim!=AN_Hurt) e.anim=(d>5)?AN_Walk:AN_Idle;
        } else if (e.type==ET_Bat) {
            e.zigTimer-=adt;
            if (e.zigTimer<=0){e.zigDir*=-1;e.zigTimer=rndF(0.3f,0.7f);}
            if (d>0) {
                float px=-dy/d,py=dx/d;
                tryMove(e.x,e.y,(dx/d*e.speed+px*35*e.zigDir)*adt,(dy/d*e.speed+py*35*e.zigDir)*adt,collR);
                e.facingLeft=dx<0;
            }
            if (e.anim==AN_Hurt&&e.animTimer>0.4f){e.anim=AN_Walk;e.animTimer=0;} else if(e.anim!=AN_Hurt) e.anim=AN_Walk;
        } else if (e.type==ET_Brute) {
            if (d>0){tryMove(e.x,e.y,dx/d*e.speed*adt,dy/d*e.speed*adt,collR);e.facingLeft=dx<0;}
            if (e.anim==AN_Hurt&&e.animTimer>0.4f){e.anim=AN_Walk;e.animTimer=0;} else if(e.anim!=AN_Hurt) e.anim=AN_Walk;
        } else if (e.type==ET_Mage) {
            e.aimAngle=std::atan2(dy,dx); e.facingLeft=dx<0;
            if (d>0) {
                float ds=0; if(d<200) ds=-0.5f; else if(d>280) ds=0.7f;
                if (ds!=0) tryMove(e.x,e.y,dx/d*e.speed*ds*adt,dy/d*e.speed*ds*adt,collR);
            }
            e.shootCd-=adt;
            if (e.shootCd<=0&&d<380) {
                e.shootCd=rndF(2.0f,3.0f);
                for (float off:{-0.3f,0.f,0.3f}) {
                    Bullet b; b.x=e.x; b.y=e.y; float a=e.aimAngle+off;
                    b.vx=std::cos(a)*160; b.vy=std::sin(a)*160; b.angle=a; b.damage=1; b.enemy=true;
                    m_bullets.push_back(b);
                }
            }
            if (e.anim==AN_Hurt&&e.animTimer>0.4f){e.anim=AN_Walk;e.animTimer=0;} else if(e.anim!=AN_Hurt) e.anim=(d>5)?AN_Walk:AN_Idle;
        } else if (e.type==ET_MiniBoss) {
            if (e.hp<e.maxHp*0.5f&&e.phase==1) e.phase=2;
            e.moveCd-=adt;
            if (e.moveCd<=0){e.moveCd=rndF(0.8f,1.6f);float pad=TILE*2.5f;e.moveTargetX=rndF(pad,GW-pad);e.moveTargetY=rndF(pad,GH-pad);}
            float mdx=e.moveTargetX-e.x,mdy=e.moveTargetY-e.y,md=std::hypot(mdx,mdy);
            if (md>5){tryMove(e.x,e.y,mdx/md*e.speed*adt,mdy/md*e.speed*adt,collR);e.facingLeft=mdx<0;}
            float a=std::atan2(m_player.y-e.y,m_player.x-e.x);
            float bspd=(e.phase==2)?195:160;
            e.shootCd-=adt;
            if (e.shootCd<=0){
                float baseCd=0.7f+(3-e.subType)*0.15f;
                e.shootCd=(e.phase==2)?baseCd*0.7f:baseCd;
                Bullet b; b.x=e.x; b.y=e.y; b.vx=std::cos(a)*bspd; b.vy=std::sin(a)*bspd;
                b.angle=a; b.damage=e.damage; b.enemy=true; m_bullets.push_back(b);
                if (e.phase==2) for (float off:{-0.35f,0.35f}) {
                    Bullet b2=b; b2.vx=std::cos(a+off)*bspd; b2.vy=std::sin(a+off)*bspd;
                    b2.angle=a+off; b2.damage=1; m_bullets.push_back(b2);
                }
            }
            e.burstCd-=adt;
            if (e.burstCd<=0){
                e.burstCd=(e.phase==2)?3.5f:5.5f;
                int rays=8+e.subType*2;
                for (int i=0;i<rays;++i){
                    float ang=(i/float(rays))*2*M_PI;
                    Bullet b; b.x=e.x; b.y=e.y; b.vx=std::cos(ang)*130; b.vy=std::sin(ang)*130;
                    b.angle=ang; b.damage=1; b.enemy=true; m_bullets.push_back(b);
                }
            }
            if (e.anim==AN_Hurt&&e.animTimer>0.4f){e.anim=AN_Walk;e.animTimer=0;} else if(e.anim!=AN_Hurt) e.anim=(md>5)?AN_Walk:AN_Idle;
        } else if (e.type==ET_FinalBoss) {
            if (e.hp<e.maxHp*0.66f&&e.phase==1) e.phase=2;
            if (e.hp<e.maxHp*0.33f&&e.phase==2) e.phase=3;
            e.moveCd-=adt;
            if (e.moveCd<=0){e.moveCd=rndF(0.5f,1.0f);float pad=TILE*2.5f;e.moveTargetX=rndF(pad,GW-pad);e.moveTargetY=rndF(pad,GH-pad);}
            float mdx=e.moveTargetX-e.x,mdy=e.moveTargetY-e.y,md=std::hypot(mdx,mdy);
            float fSpd=e.speed*(e.phase==1?1.f:e.phase==2?1.3f:1.7f);
            if (md>5){tryMove(e.x,e.y,mdx/md*fSpd*adt,mdy/md*fSpd*adt,collR);e.facingLeft=mdx<0;}
            float a=std::atan2(m_player.y-e.y,m_player.x-e.x);
            float bspd=200+e.phase*30;
            e.shootCd-=adt;
            if (e.shootCd<=0){
                e.shootCd=(e.phase==3)?0.45f:(e.phase==2?0.6f:0.8f);
                int n=1+e.phase;
                for (int i=0;i<n;++i){
                    float off=(n==1)?0:(i-(n-1)/2.f)*0.22f;
                    Bullet b; b.x=e.x; b.y=e.y;
                    b.vx=std::cos(a+off)*bspd; b.vy=std::sin(a+off)*bspd;
                    b.angle=a+off; b.damage=e.damage; b.enemy=true; m_bullets.push_back(b);
                }
            }
            e.burstCd-=adt;
            if (e.burstCd<=0){
                e.burstCd=(e.phase==3)?2.5f:(e.phase==2?3.5f:4.5f);
                int rays=12+e.phase*2; float baseAng=m_globalTime*0.5f;
                for (int i=0;i<rays;++i){
                    float ang=baseAng+(i/float(rays))*2*M_PI;
                    Bullet b; b.x=e.x; b.y=e.y; b.vx=std::cos(ang)*140; b.vy=std::sin(ang)*140;
                    b.angle=ang; b.damage=1; b.enemy=true; m_bullets.push_back(b);
                }
            }
            if (e.phase>=2){
                e.specialCd-=adt;
                if (e.specialCd<=0){
                    e.specialCd=(e.phase==3)?4.5f:7.0f;
                    int n=(e.phase==3)?16:10;
                    float baseAng=std::atan2(m_player.y-e.y,m_player.x-e.x);
                    for (int i=0;i<n;++i){
                        float ang=baseAng-0.6f+(i/float(n-1))*1.2f;
                        Bullet b; b.x=e.x; b.y=e.y; b.vx=std::cos(ang)*240; b.vy=std::sin(ang)*240;
                        b.angle=ang; b.damage=1; b.enemy=true; m_bullets.push_back(b);
                    }
                }
            }
            if (e.anim==AN_Hurt&&e.animTimer>0.4f){e.anim=AN_Walk;e.animTimer=0;} else if(e.anim!=AN_Hurt) e.anim=(md>5)?AN_Walk:AN_Idle;
        }
    }
}

// ============================================================
//  BULLETS
// ============================================================
void GameWidget::updateBullets(float dt)
{
    for (size_t bi=0;bi<m_bullets.size();++bi) {
        Bullet &b=m_bullets[bi];
        if (b.dead) continue;
        if (b.grenade) {
            float drag=0.985f; b.vx*=drag; b.vy*=drag;
            b.x+=b.vx*dt; b.y+=b.vy*dt; b.grenadeFuse-=dt;
            if (b.x<TILE*1.2f||b.x>GW-TILE*1.2f) b.vx*=-0.6f;
            if (b.y<TILE*1.2f||b.y>GH-TILE*1.2f) b.vy*=-0.6f;
            b.x=clampF(b.x,TILE*1.2f,GW-TILE*1.2f); b.y=clampF(b.y,TILE*1.2f,GH-TILE*1.2f);
            if (b.grenadeFuse<=0) {
                float exX=b.x,exY=b.y,rad=90.f;
                for (auto &e:m_enemies) if(!e.dead&&distF(e.x,e.y,exX,exY)<rad+e.size*0.3f) hurtEnemy(e,80);
                for (int i=0;i<32;++i) {
                    float ang=(i/32.f)*2*M_PI+rndF(-0.1f,0.1f),spd=rndF(80,250);
                    Particle p; p.x=exX; p.y=exY; p.vx=std::cos(ang)*spd; p.vy=std::sin(ang)*spd-60;
                    p.color=(i%2==0)?PAL_FIRE[0]:PAL_FIRE[1]; p.life=p.maxLife=rndF(0.4f,0.9f); p.size=(int)rndF(4,8);
                    m_particles.push_back(p);
                }
                b.dead=true;
            }
            continue;
        }
        b.x+=b.vx*dt; b.y+=b.vy*dt;
        float m=TILE*1.0f;
        if (b.x<m||b.x>GW-m) { if(b.bounce>0){b.vx*=-1;b.bounce--;b.angle=std::atan2(b.vy,b.vx);}else b.dead=true; }
        if (b.y<m||b.y>GH-m) { if(b.bounce>0){b.vy*=-1;b.bounce--;b.angle=std::atan2(b.vy,b.vx);}else b.dead=true; }
        if (!b.dead&&collidesObstacle(b.x,b.y,4)) b.dead=true;
        if (!b.dead&&!b.enemy) {
            for (auto &e:m_enemies) {
                if (e.dead||b.hitIds.contains(e.id)) continue;
                if (distF(b.x,b.y,e.x,e.y)<8+e.size*0.4f) {
                    hurtEnemy(e,b.damage);
                    // SK_SPLT : eclat en 2 fleches perpendiculaires (pas de recursion)
                    bool hasSplt=std::find(m_player.skills.begin(),m_player.skills.end(),SK_SPLT)!=m_player.skills.end();
                    if (hasSplt&&!b.splitChild) {
                        float perp=b.angle+M_PI/2;
                        for (int sgn=-1;sgn<=1;sgn+=2) {
                            Bullet sb; sb.x=b.x; sb.y=b.y;
                            sb.vx=std::cos(perp)*sgn*240; sb.vy=std::sin(perp)*sgn*240;
                            sb.angle=perp+(sgn<0?M_PI:0); sb.damage=b.damage*0.5f;
                            sb.splitChild=true; sb.enemy=false; sb.hitIds.insert(e.id);
                            m_bullets.push_back(sb);
                        }
                    }
                    if (!b.pierce){b.dead=true;} else b.hitIds.insert(e.id);
                    break;
                }
            }
        }
    }
    m_bullets.erase(std::remove_if(m_bullets.begin(),m_bullets.end(),[](const Bullet &b){return b.dead;}),m_bullets.end());
}

void GameWidget::updateParticles(float dt)
{
    for (auto &p:m_particles) { p.x+=p.vx*dt; p.y+=p.vy*dt; p.vy+=180*dt; p.life-=dt; }
    m_particles.erase(std::remove_if(m_particles.begin(),m_particles.end(),[](const Particle &p){return p.life<=0;}),m_particles.end());
}

// ============================================================
//  DOOR TRANSITION
// ============================================================
void GameWidget::checkDoorTransition()
{
    int door=-1;
    // Zone elargie : TILE*1.0 en profondeur, TILE*0.7 en largeur autour de la porte
    if (m_doorOpen[0]&&m_player.y<TILE*1.0f&&std::abs(m_player.x-DOOR_COL*TILE-TILE/2.f)<TILE*0.7f) door=0;
    else if (m_doorOpen[1]&&m_player.x>GW-TILE*1.0f&&std::abs(m_player.y-DOOR_ROW*TILE-TILE/2.f)<TILE*0.7f) door=1;
    else if (m_doorOpen[2]&&m_player.y>GH-TILE*1.0f&&std::abs(m_player.x-DOOR_COL*TILE-TILE/2.f)<TILE*0.7f) door=2;
    else if (m_doorOpen[3]&&m_player.x<TILE*1.0f&&std::abs(m_player.y-DOOR_ROW*TILE-TILE/2.f)<TILE*0.7f) door=3;
    if (door>=0) { m_exitDoor=door; m_state=GS_FadeOut; m_fadeAmount=0; }
}

// ============================================================
//  SKILLS
// ============================================================
void GameWidget::generateSkills()
{
    std::vector<int> avail;
    for (auto &s:m_allSkills) {
        if (s.id==SK_GRN&&m_currentRoom<3) continue;
        bool has=std::find(m_player.skills.begin(),m_player.skills.end(),s.id)!=m_player.skills.end();
        // Repeatable : grenade (+munitions) et vitalite (+PV)
        if (s.id==SK_GRN||s.id==SK_MAXHP) { avail.push_back(s.id); continue; }
        if (!has) avail.push_back(s.id);
    }
    for (int i=(int)avail.size()-1;i>0;--i) { int j=rndI(0,i); std::swap(avail[i],avail[j]); }
    m_skillChoices.clear();
    for (int i=0;i<std::min((int)avail.size(),3);++i) m_skillChoices.push_back(avail[i]);
}

void GameWidget::applySkill(int skillId)
{
    bool has=std::find(m_player.skills.begin(),m_player.skills.end(),skillId)!=m_player.skills.end();
    if (!has) m_player.skills.push_back(skillId);
    if (skillId==SK_FST)   m_player.fireRate*=0.65f;
    if (skillId==SK_SPD)   m_player.speed*=1.25f;
    if (skillId==SK_POW)   m_player.damage*=1.6f;
    if (skillId==SK_GRN)   m_player.grenadeAmmo+=3;
    if (skillId==SK_MAXHP) { m_player.maxHp+=2; m_player.hp=m_player.maxHp; }
    // +1 PV regen automatique entre chaque salle
    m_player.hp=std::min(m_player.maxHp,m_player.hp+1.f);
    int entryDoor=(m_exitDoor+2)%4;
    buildRoom(m_currentRoom+1,entryDoor);
    m_state=GS_FadeIn; m_fadeAmount=1.f;
}


// ============================================================
//  INPUT
// ============================================================
void GameWidget::keyPressEvent(QKeyEvent *event)
{
    int k=event->key();
    m_keys.insert(k);
    if (m_state==GS_Menu) {
        if (k==Qt::Key_Space||k==Qt::Key_Return) startGame();
        else if (k==Qt::Key_C) m_state=GS_SkinSelect;
    } else if (m_state==GS_SkinSelect) {
        if (k==Qt::Key_Left||k==Qt::Key_A) m_menuSelectedSkin=(m_menuSelectedSkin+3)%4;
        else if (k==Qt::Key_Right||k==Qt::Key_D) m_menuSelectedSkin=(m_menuSelectedSkin+1)%4;
        else if (k==Qt::Key_Up||k==Qt::Key_W) m_menuSelectedAtk=(m_menuSelectedAtk+2)%3;
        else if (k==Qt::Key_Down||k==Qt::Key_S) m_menuSelectedAtk=(m_menuSelectedAtk+1)%3;
        else if (k==Qt::Key_Escape||k==Qt::Key_Return||k==Qt::Key_Space) {
            QSettings s("RetroArcher","RetroArcher");
            s.setValue("skin",m_menuSelectedSkin); s.setValue("atk",m_menuSelectedAtk);
            m_state=GS_Menu;
        }
    } else if (m_state==GS_GameOver||m_state==GS_Victory) {
        if (k==Qt::Key_Space) startGame();
        else if (k==Qt::Key_Escape) m_state=GS_Menu;
    } else if (m_state==GS_SkillSelect) {
        int idx=-1;
        if (k==Qt::Key_1) idx=0; else if (k==Qt::Key_2) idx=1; else if (k==Qt::Key_3) idx=2;
        if (idx>=0&&idx<(int)m_skillChoices.size()) applySkill(m_skillChoices[idx]);
    }
}
void GameWidget::keyReleaseEvent(QKeyEvent *event) { m_keys.remove(event->key()); }
void GameWidget::mousePressEvent(QMouseEvent *) {}

// ============================================================
//  PAINT
// ============================================================
void GameWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.scale(DISPLAY_SCALE,DISPLAY_SCALE);
    p.setRenderHint(QPainter::SmoothPixmapTransform,false);
    p.setRenderHint(QPainter::Antialiasing,false);
    renderGame(p);
}

void GameWidget::drawSpriteAt(QPainter &p, const QImage &sheet, int frameCount, float fps,
                               float x, float y, float scale, bool flipX, float timer)
{
    if (sheet.isNull()) return;
    int frame=std::max(0,std::min(frameCount-1,(int)(timer*fps)%frameCount));
    int fw=sheet.width()/frameCount, fh=sheet.height();
    float dw=fw*scale, dh=fh*scale;
    float dx=x-dw/2.f, dy=y-dh*0.85f;
    p.save();
    if (flipX) {
        p.translate(int(x),0); p.scale(-1,1);
        p.drawImage(QRectF(-dw/2.f,dy,dw,dh),sheet,QRectF(frame*fw,0,fw,fh));
    } else {
        p.drawImage(QRectF(dx,dy,dw,dh),sheet,QRectF(frame*fw,0,fw,fh));
    }
    p.restore();
}

// ============================================================
//  RENDER GAME
// ============================================================
void GameWidget::renderGame(QPainter &p)
{
    p.fillRect(0,0,CW,CH,Qt::black);
    if (m_state==GS_Menu){drawMenu(p);return;}
    if (m_state==GS_SkinSelect){drawSkinSelect(p);return;}
    drawRoom(p);

    for (auto &pa:m_particles) {
        float a=std::max(0.f,pa.life/pa.maxLife);
        QColor c=pa.color; c.setAlphaF(a);
        p.fillRect(QRectF(pa.x-pa.size/2.f,pa.y-pa.size/2.f,pa.size,pa.size),c);
    }

    for (auto &e:m_enemies) {
        int variant=-1;
        if (e.type==ET_Slime) variant=0; else if (e.type==ET_Skel) variant=1;
        else if (e.type==ET_Bat) variant=2; else if (e.type==ET_Brute) variant=3;
        else if (e.type==ET_Mage) variant=4;
        else if (e.type==ET_MiniBoss||e.type==ET_FinalBoss) variant=5;
        if (variant<0) continue;
        AnimType anim=e.anim;
        const QImage &sheet=m_sprOrc[variant][anim];
        float scale=1.f;
        if (e.type==ET_Slime) scale=1.05f; else if (e.type==ET_Skel) scale=1.10f;
        else if (e.type==ET_Bat) scale=0.95f; else if (e.type==ET_Brute) scale=1.30f;
        else if (e.type==ET_Mage) scale=1.10f;
        else if (e.type==ET_MiniBoss) scale=1.6f+e.subType*0.1f;
        else if (e.type==ET_FinalBoss) scale=2.2f;
        int fc=6; float fps=8.f;
        switch(anim){case AN_Idle:fc=s_idleFrameCount;fps=8.f;break;case AN_Walk:fc=s_walkFrameCount;fps=12.f;break;
                     case AN_Atk:fc=6;fps=14.f;break;case AN_Hurt:fc=s_hurtFrameCount;fps=10.f;break;
                     case AN_Death:fc=s_deathFrameCount;fps=7.f;break;}
        float fadeAlpha=1.f;
        if (e.dead&&e.corpseFadeTimer>0) fadeAlpha=std::max(0.f,1.f-e.corpseFadeTimer/0.6f);
        if (fadeAlpha<1.f) p.setOpacity(fadeAlpha);
        drawSpriteAt(p,sheet,fc,fps,e.x,e.y,scale,e.facingLeft,e.animTimer);
        if (fadeAlpha<1.f) p.setOpacity(1.f);

        // Flocons geles (SK_FRZ)
        if (e.slowTimer>0&&!e.dead) {
            for (int i=0;i<3;++i) {
                float phase=m_globalTime*2+i*2.f;
                float fx=e.x+std::cos(phase)*e.size*0.5f;
                float fy=e.y-e.size*0.5f+std::sin(phase*1.3f)*6;
                p.fillRect(QRectF(fx-1.5f,fy-1.5f,3,3),QColor("#aaddff"));
                p.fillRect(QRectF(fx-0.5f,fy-0.5f,1,1),QColor("#ffffff"));
            }
        }
        if (e.hitFlash>0&&!e.dead) {
            p.save(); p.setCompositionMode(QPainter::CompositionMode_Plus);
            p.setOpacity(std::min(0.7f,e.hitFlash*4));
            drawSpriteAt(p,sheet,fc,fps,e.x,e.y,scale,e.facingLeft,e.animTimer);
            p.restore();
        }
        if (!e.dead&&e.hp<e.maxHp) {
            float bw=e.size*1.2f,bh=4,bx=e.x-bw/2.f,by=e.y-e.size/2.f-14;
            p.fillRect(QRectF(bx,by,bw,bh),C_HPB);
            QColor hpCol=(e.type==ET_MiniBoss||e.type==ET_FinalBoss)?C_BE:C_HP;
            p.fillRect(QRectF(bx,by,bw*e.hp/e.maxHp,bh),hpCol);
        }
    }

    for (auto &b:m_bullets) {
        if (b.dead) continue;
        if (b.grenade) {
            float t=(b.grenadeTotal-b.grenadeFuse)/b.grenadeTotal;
            float pulse=0.5f+0.5f*std::sin(t*30); int sz=7+int(pulse*3);
            p.fillRect(QRectF(b.x-sz/2.f,b.y-sz/2.f,sz,sz),QColor(60,50,40));
            QColor red(255,60+int(pulse*80),0); int isz=4+int(pulse*2);
            p.fillRect(QRectF(b.x-isz/2.f,b.y-isz/2.f,isz,isz),red);
            p.fillRect(QRectF(b.x-1,b.y-sz/2.f-4,2,2),QColor("#ffff44"));
        } else if (b.enemy) {
            p.fillRect(QRectF(b.x-5,b.y-5,10,10),C_BEG);
            p.fillRect(QRectF(b.x-3,b.y-3,6,6),C_BE);
            p.fillRect(QRectF(b.x-1,b.y-1,2,2),Qt::white);
        } else {
            p.save(); p.translate(b.x,b.y); p.rotate(b.angle*180.f/M_PI-90);
            float w=m_sprArrow.width(),h=m_sprArrow.height();
            p.drawImage(QRectF(-w/2,-h/2,w,h),m_sprArrow);
            p.restore();
        }
    }

    if (!(m_player.invincibility>0&&(int)(m_player.invincibility*28)%2==0)) {
        const QImage &sheet=soldierSheet(m_player.anim);
        drawSpriteAt(p,sheet,soldierFrameCount(m_player.anim),soldierFps(m_player.anim),
                     m_player.x,m_player.y,1.3f,m_player.facingLeft,m_player.animTimer);
    }

    drawHUD(p);
    if (m_fadeAmount>0) { QColor c(0,0,0); c.setAlphaF(std::min(1.f,m_fadeAmount)); p.fillRect(0,0,CW,CH,c); }
    if (m_state==GS_SkillSelect) drawSkillSelect(p);
    if (m_state==GS_GameOver)    drawEndScreen(p,false);
    if (m_state==GS_Victory)     drawEndScreen(p,true);
}

// ============================================================
//  DRAW ROOM
// ============================================================
void GameWidget::drawRoom(QPainter &p)
{
    for (int r=1;r<ROWS-1;++r) for (int c=1;c<COLS-1;++c) {
        int x=c*TILE,y=r*TILE;
        p.fillRect(x,y,TILE,TILE,((r+c)%2==0)?C_FL1:C_FL2);
        if ((r*7+c*13)%11==0) p.fillRect(x+4,y+9,9,2,QColor(0,0,0,50));
    }
    for (int r=1;r<ROWS-1;++r) for (int c=1;c<COLS-1;++c) {
        if (!isObstacleTile(c,r)) continue;
        int x=c*TILE,y=r*TILE;
        p.fillRect(x+2,y+2,TILE-2,TILE-2,QColor(0,0,0,90));
        p.fillRect(x+2,y,TILE-4,TILE-4,C_PILL2);
        p.fillRect(x+4,y+2,TILE-8,TILE-8,C_PILL);
        p.fillRect(x+6,y+4,4,4,QColor("#7a4828"));
    }
    for (int c=0;c<COLS;++c) {
        p.fillRect(c*TILE,0,TILE,TILE,C_WT); p.fillRect(c*TILE,TILE-8,TILE,8,C_WF);
        p.fillRect(c*TILE,(ROWS-1)*TILE,TILE,TILE,C_WT); p.fillRect(c*TILE,(ROWS-1)*TILE,TILE,8,C_WF);
    }
    for (int r=0;r<ROWS;++r) {
        p.fillRect(0,r*TILE,TILE,TILE,C_WT); p.fillRect(TILE-8,r*TILE,8,TILE,C_WF);
        p.fillRect((COLS-1)*TILE,r*TILE,TILE,TILE,C_WT); p.fillRect((COLS-1)*TILE,r*TILE,8,TILE,C_WF);
    }
    auto drawDoor=[&](int col,int row,bool open){
        int x=col*TILE,y=row*TILE;
        p.fillRect(x+4,y+4,TILE-8,TILE-8,open?C_DO:C_DC);
        if (open) {
            p.fillRect(x+6,y+6,TILE-12,TILE-12,QColor(20,10,5));
            QColor glow("#ffd544"); glow.setAlpha(120+(int)(std::sin(m_globalTime*4)*40));
            p.setPen(QPen(glow,1)); p.setBrush(Qt::NoBrush);
            p.drawRect(x+4,y+4,TILE-9,TILE-9); p.setPen(Qt::NoPen);
        } else {
            p.fillRect(x+TILE/2-2,y+TILE/2-2,4,4,QColor(40,30,5));
        }
    };
    drawDoor(DOOR_COL,0,m_doorOpen[0]); drawDoor(COLS-1,DOOR_ROW,m_doorOpen[1]);
    drawDoor(DOOR_COL,ROWS-1,m_doorOpen[2]); drawDoor(0,DOOR_ROW,m_doorOpen[3]);
}

// ============================================================
//  PROGRESSION BAR
// ============================================================
void GameWidget::drawProgressionBar(QPainter &p, int x, int y, int w)
{
    int h=8;
    p.fillRect(QRectF(x,y,w,h),QColor(20,15,10));
    p.setPen(QPen(C_GOLD,1)); p.setBrush(Qt::NoBrush); p.drawRect(x,y,w,h); p.setPen(Qt::NoPen);
    float per=float(w)/ROOMS_TOTAL;
    for (int i=1;i<=ROOMS_TOTAL;++i) {
        float rx=x+(i-1)*per, rw=per-1;
        QColor col=(i<m_currentRoom)?QColor(40,110,40):(i==m_currentRoom)?QColor(220,180,40):QColor(60,50,40);
        p.fillRect(QRectF(rx+0.5f,y+1,rw,h-2),col);
    }
    auto drawMarker=[&](int room,bool isFinal){
        float cx=x+(room-0.5f)*per, cy=y+h/2.f;
        if (isFinal) {
            p.setBrush(QColor("#ff2222")); p.setPen(QPen(QColor("#ffaa44"),1));
            float s=7; QPolygonF star;
            for (int i=0;i<8;++i) { float ang=i*M_PI/4.f-M_PI/2.f; float rr=(i%2==0)?s:s*0.5f;
                star<<QPointF(cx+std::cos(ang)*rr,cy+std::sin(ang)*rr); }
            p.drawPolygon(star);
        } else {
            p.setBrush(QColor("#cc2244")); p.setPen(QPen(QColor("#ffcc44"),1));
            p.drawEllipse(QPointF(cx,cy),5,5);
        }
        p.setPen(Qt::NoPen); p.setBrush(Qt::NoBrush);
    };
    drawMarker(5,false); drawMarker(9,false); drawMarker(13,false); drawMarker(17,false); drawMarker(20,true);
}

// ============================================================
//  HUD
// ============================================================
void GameWidget::drawHUD(QPainter &p)
{
    p.fillRect(0,GH,GW,HUD,C_HUD);
    p.setPen(QPen(C_HB,2)); p.drawRect(0,GH,GW,HUD-1); p.setPen(Qt::NoPen);
    QFont font("monospace",9,QFont::Bold); p.setFont(font); p.setPen(C_GOLD); p.drawText(8,GH+18,"PV");
    for (int i=0;i<(int)m_player.maxHp;++i) {
        bool full=i<m_player.hp; QColor fill=full?C_HP:C_HPB;
        p.fillRect(QRectF(30+i*17,GH+8,14,14),fill);
        if (full) p.fillRect(QRectF(32+i*17,GH+10,5,4),QColor("#ff6688"));
    }
    if (m_player.grenadeAmmo>0||std::find(m_player.skills.begin(),m_player.skills.end(),SK_GRN)!=m_player.skills.end()) {
        QFont gf("monospace",8,QFont::Bold); p.setFont(gf); p.setPen(QColor("#ff8844"));
        p.drawText(8+30+(int)m_player.maxHp*17+12,GH+19,QString("[G] x%1").arg(m_player.grenadeAmmo));
    }
    if (isAnyBossRoom(m_currentRoom)&&m_bossIndex>=0&&m_bossIndex<(int)m_enemies.size()) {
        Enemy &boss=m_enemies[m_bossIndex];
        if (!boss.dead) {
            float bw=220,bh=11,bx=GW/2.f-bw/2.f,by=GH+10;
            p.fillRect(QRectF(bx,by,bw,bh),C_HPB);
            QColor bc=(boss.type==ET_FinalBoss)?QColor("#ff1100"):(boss.phase==2)?QColor("#ff5500"):QColor("#cc3344");
            p.fillRect(QRectF(bx,by,bw*boss.hp/boss.maxHp,bh),bc);
            p.setPen(QPen(C_GOLD,1)); p.drawRect(QRectF(bx,by,bw,bh)); p.setPen(Qt::NoPen);
            QFont sf("monospace",7,QFont::Bold); p.setFont(sf); p.setPen(C_GOLD);
            QString label;
            if (boss.type==ET_FinalBoss) {
                if (boss.phase==1) label="BOSS FINAL";
                else if (boss.phase==2) label="BOSS FINAL - PHASE 2";
                else label="BOSS FINAL - PHASE 3 - FUREUR";
            } else {
                label=(boss.phase==2)?QString("MINI-BOSS %1 - PHASE 2").arg(boss.subType+1):QString("MINI-BOSS %1").arg(boss.subType+1);
            }
            QFontMetrics fm(sf); p.drawText(GW/2-fm.horizontalAdvance(label)/2,by-2,label);
        }
    }
    drawProgressionBar(p,8,GH+32,GW-16);
    QFont sf("monospace",8,QFont::Bold); p.setFont(sf); p.setPen(C_GOLD);
    QString roomTxt=QString("MANCHE %1 / %2").arg(m_currentRoom).arg(ROOMS_TOTAL);
    if (m_state==GS_RoomCleared) roomTxt+="  -  CHOISIS UNE PORTE";
    p.drawText(8,GH+53,roomTxt);
    QFont skf("monospace",7,QFont::Bold); p.setFont(skf);
    QFontMetrics skfm(skf); int sx=GW-8;
    for (int i=(int)m_player.skills.size()-1;i>=0;--i) {
        const Skill *sk=nullptr;
        for (auto &s:m_allSkills) if(s.id==m_player.skills[i]){sk=&s;break;}
        if (!sk) continue;
        int tw=skfm.horizontalAdvance(sk->icon)+8; sx-=tw+4;
        QColor bgc=sk->color; bgc.setAlpha(60);
        p.fillRect(QRectF(sx,GH+8,tw,14),bgc); p.setPen(sk->color); p.drawText(sx+4,GH+19,sk->icon);
    }
    QFont hf("monospace",7); p.setFont(hf); p.setPen(QColor("#aaaaaa"));
    QString hs=QString("Record: %1").arg(m_highScore);
    QFontMetrics hfm(hf); p.drawText(GW-hfm.horizontalAdvance(hs)-8,GH+53,hs);
}


// ============================================================
//  MENU PRINCIPAL avec galerie des boss animée
// ============================================================
void GameWidget::drawMenu(QPainter &p)
{
    p.fillRect(0,0,CW,CH,QColor("#050510"));
    // Etoiles
    for (int i=0;i<60;++i) {
        int sx=(i*137+42)%CW, sy=(i*89+11)%CH;
        float b=0.4f+std::sin(m_globalTime*2+i)*0.3f;
        QColor c(255,255,255); c.setAlphaF(std::max(0.f,std::min(1.f,b)));
        p.fillRect(sx,sy,1,1,c);
    }

    // Titre
    QFont big("monospace",30,QFont::Bold); p.setFont(big); QFontMetrics fmb(big);
    p.setPen(C_GOLD); p.drawText(CW/2-fmb.horizontalAdvance("RETRO")/2,38,"RETRO");
    p.setPen(QColor("#dd2222")); p.drawText(CW/2-fmb.horizontalAdvance("ARCHER")/2,70,"ARCHER");
    QFont mid("monospace",7); p.setFont(mid); p.setPen(QColor("#666666")); QFontMetrics fmm(mid);
    QString sv="20 salles - 5 boss"; p.drawText(CW/2-fmm.horizontalAdvance(sv)/2,82,sv);

    // Record + barre
    QFont hf("monospace",8,QFont::Bold); p.setFont(hf); p.setPen(C_GOLD); QFontMetrics hfm(hf);
    QString hs=(m_highScore>0)?QString("RECORD : %1 / %2").arg(m_highScore).arg(ROOMS_TOTAL):"RECORD : aucun";
    p.drawText(CW/2-hfm.horizontalAdvance(hs)/2,97,hs);
    { int old=m_currentRoom; m_currentRoom=std::min(m_highScore+1,ROOMS_TOTAL);
      drawProgressionBar(p,CW/2-200,102,400); m_currentRoom=old; }
    p.setFont(mid); p.setPen(QColor("#666666")); QString leg="O = mini-boss     * = boss final";
    p.drawText(CW/2-fmm.horizontalAdvance(leg)/2,119,leg);

    // ─────────────────────────────────────────────────────
    //  GALERIE DES BOSS
    // ─────────────────────────────────────────────────────
    const int gY=127, gH=118, panelW=CW/5, sprH=72, sprMaxW=panelW-8;
    QFont galF("monospace",7,QFont::Bold); p.setFont(galF); p.setPen(C_GOLD);
    QFontMetrics gfm(galF); QString gt="— LES BOSS —";
    p.drawText(CW/2-gfm.horizontalAdvance(gt)/2,gY-2,gt);

    for (int i=0;i<m_bossGallery.size();++i) {
        const BossInfo &boss=m_bossGallery[i];
        int px=i*panelW;
        bool isFinal=(i==m_bossGallery.size()-1);
        p.fillRect(px+2,gY+2,panelW-4,gH-4,isFinal?QColor(20,5,40):QColor(10,10,25));

        float pulse=0.7f+0.3f*std::sin(m_globalTime*3+i);
        QColor border=boss.accent; border.setAlphaF(isFinal?pulse:0.55f);
        p.setPen(QPen(border,isFinal?2:1)); p.setBrush(Qt::NoBrush);
        p.drawRect(px+2,gY+2,panelW-5,gH-5); p.setPen(Qt::NoPen);

        if (!boss.idleSheet.isNull()) {
            int frame=(int)(m_globalTime*boss.fps)%boss.frameCount;
            float scX=float(sprMaxW)/boss.frameW, scY=float(sprH)/boss.frameH;
            float sc=std::min(scX,scY);
            float dw=boss.frameW*sc, dh=boss.frameH*sc;
            float dx=px+(panelW-dw)/2.f, dy=gY+4+(sprH-dh)/2.f;
            p.drawImage(QRectF(dx,dy,dw,dh),boss.idleSheet,QRectF(frame*boss.frameW,0,boss.frameW,boss.frameH));
        }

        QFont nF("monospace",5,QFont::Bold); p.setFont(nF); QFontMetrics nfm(nF);
        p.setPen(boss.accent);
        // Nom sur 1 ou 2 lignes
        int lineY=gY+sprH+13;
        int nw=nfm.horizontalAdvance(boss.nameFr);
        if (nw>panelW-4) {
            int sp=boss.nameFr.indexOf(' ',boss.nameFr.length()/2);
            if (sp>0) {
                QString l1=boss.nameFr.left(sp),l2=boss.nameFr.mid(sp+1);
                p.drawText(px+(panelW-nfm.horizontalAdvance(l1))/2,lineY,l1);
                p.drawText(px+(panelW-nfm.horizontalAdvance(l2))/2,lineY+7,l2); lineY+=7;
            } else p.drawText(px+(panelW-nw)/2,lineY,boss.nameFr);
        } else p.drawText(px+(panelW-nw)/2,lineY,boss.nameFr);

        QFont rF("monospace",5); p.setFont(rF); p.setPen(QColor("#888888")); QFontMetrics rfm(rF);
        p.drawText(px+(panelW-rfm.horizontalAdvance(boss.roomTxt))/2,lineY+9,boss.roomTxt);
    }

    // Controles
    QFont lf("monospace",6); p.setFont(lf); QFontMetrics lfm(lf); p.setPen(QColor("#777777"));
    int ky=gY+gH+12;
    for (auto &h:{QString("WASD/Fleches: deplacer     Arret: tir auto"),
                  QString("G: grenade    1/2/3: competence"),
                  QString("Porte ouverte: marcher dedans pour avancer")}) {
        p.drawText(CW/2-lfm.horizontalAdvance(h)/2,ky,h); ky+=11;
    }

    // Boutons
    QFont btn("monospace",10,QFont::Bold); p.setFont(btn); QFontMetrics btnFm(btn);
    if (std::sin(m_globalTime*3)>0) {
        p.setPen(C_GOLD); QString s="[ESPACE]  JOUER";
        p.drawText(CW/2-btnFm.horizontalAdvance(s)/2,CH-50,s);
    }
    p.setPen(QColor("#7799ff")); QString cs="[C]  Choisir skin / attaque";
    p.drawText(CW/2-btnFm.horizontalAdvance(cs)/2,CH-32,cs);
    p.setFont(mid); p.setPen(QColor("#555555"));
    QString sel=QString("(%1  -  %2)").arg(g_skins[m_menuSelectedSkin].name).arg(g_atkNames[m_menuSelectedAtk]);
    p.drawText(CW/2-fmm.horizontalAdvance(sel)/2,CH-14,sel);
}

// ============================================================
//  SKIN SELECT
// ============================================================
void GameWidget::drawSkinSelect(QPainter &p)
{
    p.fillRect(0,0,CW,CH,QColor("#0a0518"));
    QFont big("monospace",16,QFont::Bold); p.setFont(big); p.setPen(C_GOLD);
    QFontMetrics fb(big); QString t="PERSONNALISATION";
    p.drawText(CW/2-fb.horizontalAdvance(t)/2,40,t);
    QFont sub("monospace",8); p.setFont(sub); p.setPen(QColor("#aaaaaa")); QFontMetrics fs(sub);
    QString s="Fleches gauche/droite : Skin       haut/bas : Attaque";
    p.drawText(CW/2-fs.horizontalAdvance(s)/2,60,s);

    QFont lbl("monospace",9,QFont::Bold); p.setFont(lbl); p.setPen(C_WH); p.drawText(40,95,"SKIN");
    int boxW=(CW-80)/4, boxH=130, boxY=100;
    for (int i=0;i<4;++i) {
        int bx=40+i*boxW;
        p.fillRect(QRectF(bx+4,boxY,boxW-8,boxH),QColor(20,15,35));
        p.setPen(QPen((i==m_menuSelectedSkin)?g_skins[i].accent:QColor("#333355"),(i==m_menuSelectedSkin)?3:1));
        p.setBrush(Qt::NoBrush); p.drawRect(QRectF(bx+4,boxY,boxW-8,boxH)); p.setPen(Qt::NoPen);
        QImage sheet=m_sprSoldierSkin[i][AN_Idle];
        if (!sheet.isNull()) {
            int frame=((int)(m_globalTime*8))%s_idleFrameCount;
            int fw=sheet.width()/s_idleFrameCount,fh=sheet.height();
            float dw=fw,dh=fh;
            p.drawImage(QRectF(bx+boxW/2.f-dw/2.f,boxY+20,dw,dh),sheet,QRectF(frame*fw,0,fw,fh));
        }
        QFont nf("monospace",7,QFont::Bold); p.setFont(nf); QFontMetrics nfm(nf);
        p.setPen(g_skins[i].accent); QString name=g_skins[i].name;
        p.drawText(bx+boxW/2-nfm.horizontalAdvance(name)/2,boxY+boxH-8,name);
    }

    p.setFont(lbl); p.setPen(C_WH); p.drawText(40,255,"ANIMATION D'ATTAQUE");
    int aboxW=(CW-80)/3, aboxH=110, aboxY=265;
    for (int i=0;i<3;++i) {
        int bx=40+i*aboxW;
        p.fillRect(QRectF(bx+4,aboxY,aboxW-8,aboxH),QColor(20,15,35));
        p.setPen(QPen((i==m_menuSelectedAtk)?QColor("#ffcc44"):QColor("#333355"),(i==m_menuSelectedAtk)?3:1));
        p.setBrush(Qt::NoBrush); p.drawRect(QRectF(bx+4,aboxY,aboxW-8,aboxH)); p.setPen(Qt::NoPen);
        QImage sheet=m_sprSoldierAtkSkin[m_menuSelectedSkin][i];
        int frames=(i==2)?s_atk3Frames:s_atk1Frames;
        float fps=(i==1)?18.f:(i==2?13.f:14.f);
        if (!sheet.isNull()) {
            int frame=((int)(m_globalTime*fps))%frames;
            int fw=sheet.width()/frames,fh=sheet.height(); float scale=0.85f;
            float dw=fw*scale,dh=fh*scale;
            p.drawImage(QRectF(bx+aboxW/2.f-dw/2.f,aboxY+10,dw,dh),sheet,QRectF(frame*fw,0,fw,fh));
        }
        QFont nf("monospace",8,QFont::Bold); p.setFont(nf); QFontMetrics nfm(nf);
        p.setPen(QColor("#ffcc44")); p.drawText(bx+aboxW/2-nfm.horizontalAdvance(g_atkNames[i])/2,aboxY+aboxH-26,g_atkNames[i]);
        QFont df("monospace",6); p.setFont(df); QFontMetrics dfm(df); p.setPen(QColor("#888888"));
        p.drawText(bx+aboxW/2-dfm.horizontalAdvance(g_atkDescs[i])/2,aboxY+aboxH-12,g_atkDescs[i]);
    }
    QFont btn("monospace",9,QFont::Bold); p.setFont(btn); QFontMetrics bfm(btn); p.setPen(C_GOLD);
    QString back="[ESPACE] Confirmer et revenir au menu";
    p.drawText(CW/2-bfm.horizontalAdvance(back)/2,CH-20,back);
}

// ============================================================
//  SKILL SELECT
// ============================================================
void GameWidget::drawSkillSelect(QPainter &p)
{
    p.fillRect(0,0,CW,CH,QColor(0,0,0,215));
    QFont titleFont("monospace",12,QFont::Bold); p.setFont(titleFont); p.setPen(C_GOLD);
    QFontMetrics tfm(titleFont); QString title="MANCHE DEGAGEE";
    p.drawText(CW/2-tfm.horizontalAdvance(title)/2,50,title);
    QFont subFont("monospace",8); p.setFont(subFont); p.setPen(C_WH); QFontMetrics sfm(subFont);
    QString sub="+1 PV regen - Choisis une competence";
    p.drawText(CW/2-sfm.horizontalAdvance(sub)/2,78,sub);
    for (int i=0;i<(int)m_skillChoices.size();++i) {
        const Skill *sk=nullptr;
        for (auto &s:m_allSkills) if(s.id==m_skillChoices[i]){sk=&s;break;}
        if (!sk) continue;
        float x=CW/2-160,y=110+i*90,w=320,h=78;
        p.fillRect(QRectF(x,y,w,h),QColor(15,10,35));
        p.setPen(QPen(sk->color,2)); p.setBrush(Qt::NoBrush); p.drawRect(QRectF(x,y,w,h)); p.setPen(Qt::NoPen);
        QFont icf("monospace",12,QFont::Bold); p.setFont(icf); p.setPen(sk->color); p.drawText(x+16,y+46,sk->icon);
        QFont nf("monospace",9,QFont::Bold); p.setFont(nf); p.setPen(C_WH); p.drawText(x+70,y+30,sk->name);
        QFont df("monospace",7); p.setFont(df); p.setPen(QColor("#aaaaaa")); p.drawText(x+70,y+54,sk->desc);
        QFont kf("monospace",9,QFont::Bold); p.setFont(kf); p.setPen(sk->color);
        QString k=QString("[%1]").arg(i+1); QFontMetrics kfm(kf);
        p.drawText(x+w-kfm.horizontalAdvance(k)-10,y+h-10,k);
    }
}

// ============================================================
//  END SCREEN
// ============================================================
void GameWidget::drawEndScreen(QPainter &p, bool win)
{
    p.fillRect(0,0,CW,CH,QColor(0,0,0,230));
    QFont big("monospace",20,QFont::Bold); p.setFont(big); p.setPen(win?C_GOLD:C_HP);
    QFontMetrics fm(big); QString t=win?"VICTOIRE !":"GAME OVER";
    p.drawText(CW/2-fm.horizontalAdvance(t)/2,CH/2-40,t);
    QFont mid("monospace",9); p.setFont(mid); p.setPen(C_WH); QFontMetrics mfm(mid);
    QString sub=win?"Tu as conquis les 20 manches !":QString("Tu as atteint la manche %1 / %2").arg(m_currentRoom).arg(ROOMS_TOTAL);
    p.drawText(CW/2-mfm.horizontalAdvance(sub)/2,CH/2-5,sub);
    p.setPen(C_GOLD); QString rec=QString("Record: %1 manches").arg(m_highScore);
    p.drawText(CW/2-mfm.horizontalAdvance(rec)/2,CH/2+20,rec);
    if (std::sin(m_globalTime*3)>0) {
        p.setPen(QColor("#888888")); QString s="ESPACE pour rejouer    ECHAP pour le menu";
        p.drawText(CW/2-mfm.horizontalAdvance(s)/2,CH/2+60,s);
    }
}
