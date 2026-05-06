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
const int GW   = COLS * TILE;     // 544
const int GH   = ROWS * TILE;     // 416
const int HUD  = 64;
const int CW   = GW;
const int CH   = GH + HUD;        // 480
const int DISPLAY_SCALE = 2;
const int ROOMS_TOTAL = 25;       // 5 mondes x 5 salles
const int DOOR_COL = COLS / 2;
const int DOOR_ROW = ROWS / 2;
}

const int GameWidget::s_walkFrameCount  = 8;
const int GameWidget::s_idleFrameCount  = 6;
const int GameWidget::s_hurtFrameCount  = 4;
const int GameWidget::s_deathFrameCount = 4;
const int GameWidget::s_atk1Frames      = 6;
const int GameWidget::s_atk2Frames      = 6;
const int GameWidget::s_atk3Frames      = 9;

// ============================================================
//  PALETTE
// ============================================================
static const QColor C_FL1 ("#3a2a1a"), C_FL2 ("#302215"), C_WT ("#3d1e08"), C_WF ("#261408");
static const QColor C_DC  ("#7a5c0a"), C_DO  ("#4a3408");
static const QColor C_HUD ("#0a0a18"), C_HB  ("#c8a800");
static const QColor C_HP  ("#dd1a33"), C_HPB ("#3a0010");
static const QColor C_GOLD("#c8a800"), C_WH  ("#e8e8e8");
static const QColor C_BE  ("#ff4400"), C_BEG ("#881100");
static const QColor PAL_SLIME[2] = { QColor("#1ab83a"), QColor("#0d7a22") };
static const QColor PAL_BONE[2]  = { QColor("#c8c8c8"), QColor("#7a7a7a") };
static const QColor PAL_FIRE[2]  = { QColor("#ffaa00"), QColor("#ff4400") };
static const QColor PAL_ICE[2]   = { QColor("#aaddff"), QColor("#5599cc") };
static const QColor PAL_POISON[2]= { QColor("#88ff44"), QColor("#226611") };

// ============================================================
//  SKINS / ATTACKS
// ============================================================
struct SkinDef { const char* name; int hue; double sat; double val; QColor accent; };
static const SkinDef g_skins[4] = {
    { "Bleu",            0, 1.0, 1.0,  QColor("#1a55bb") },
    { "Vert lierre",    90, 1.0, 0.95, QColor("#1a8833") },
    { "Rouge cramoisi",-110, 1.2, 0.9, QColor("#bb1a33") },
    { "Or imperial",  -150, 1.4, 1.05, QColor("#c8a800") },
};
static const char* g_atkNames[3] = { "Coup d'epee", "Estoc rapide", "Frappe lourde" };
static const char* g_atkDescs[3] = { "Cadence equilibree", "Rapide et precis", "Impact puissant" };

// ============================================================
//  SKILL IDs (15 anciens nerfes + 50 nouveaux = 65)
// ============================================================
enum SkillId {
    // World 1 ─ Forge ardente (anciens nerfes + nouveaux)
    SK_DBL=0, SK_TRI, SK_FST, SK_PRC, SK_SPD, SK_POW, SK_BNC, SK_DIA,
    SK_HARDCORE, SK_DODGE, SK_LIGHT_FOOT, SK_RANGE, SK_KNOCKBACK, SK_BIG_ARROW,
    SK_HUNTER, SK_SHARP, SK_FIRST_HIT, SK_FOCUSED,
    // World 2 ─ Catacombes (necro/death)
    SK_GRN,
    SK_LIFESTEAL, SK_NECRO, SK_DEATH_MARK, SK_BONE_SHIELD, SK_BLOOD_PACT,
    SK_REGEN, SK_RESILIENCE, SK_PHOENIX, SK_SOUL_DRAIN, SK_DECAY,
    SK_VAMP_NERFED, SK_FROST_FATE,
    // World 3 ─ Marais maudit (chaos/explosion/poison)
    SK_POISON, SK_BLAZE, SK_EXPLODE_KILL, SK_FIRE_TRAIL, SK_DOUBLE_GRENADE,
    SK_BIG_BOOM, SK_VOLCANO, SK_RECKLESS, SK_BURST_FIRE, SK_INFERNO,
    SK_STICKY, SK_GHOST_ARROW,
    // World 4 ─ Arene (charge/strength)
    SK_DASH, SK_RAM, SK_BERSERKER, SK_HEAVY, SK_THORNS, SK_TANK,
    SK_RAGE_STACK, SK_SHIELD, SK_LAST_HOPE, SK_ADRENALINE, SK_REFLECT, SK_FINISHER,
    // World 5 ─ Citadelle gelee (frost/legendary)
    SK_FROST_AURA, SK_FREEZE_HIT, SK_ICE_SHARDS, SK_TIME_STOP, SK_LEGEND,
    SK_MASTERY, SK_ECHO, SK_INVUL_BURST, SK_FROST_NOVA, SK_HOMING,
    SK_CRIT_NERFED, SK_QUAD_NERFED,
    // Anciens (nerfes mais inclus)
    SK_SPLT_NERFED, SK_FRZ_NERFED, SK_MAXHP_NERFED,
    SK__COUNT
};

// ============================================================
//  HELPERS
// ============================================================
static float rndF(float a, float b) { return a + QRandomGenerator::global()->generateDouble()*(b-a); }
static int   rndI(int a, int b) { return QRandomGenerator::global()->bounded(a, b+1); }
static float clampF(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
static float distF(float ax, float ay, float bx, float by) { return std::hypot(bx-ax, by-ay); }

// ============================================================
//  CONSTRUCTOR + SKILL TABLE
// ============================================================
GameWidget::GameWidget(QWidget *parent) : QWidget(parent)
{
    setFixedSize(CW * DISPLAY_SCALE, CH * DISPLAY_SCALE);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    loadAssets();
    loadWorlds();
    buildSkinSprites();
    loadSettings();

    // 65 skills - tier = monde requis (skill apparait quand m_worldsUnlocked >= tier)
    auto add = [&](int id,int tier,const char*n,const char*d,const char*ic,const char*col){
        m_allSkills.push_back({id,tier,QString::fromUtf8(n),QString::fromUtf8(d),
                               QString::fromUtf8(ic),QColor(col)});
    };
    // ─── World 1 (18 skills) ───
    add(SK_DBL,1,"Double Fleche","2 fleches paralleles","x2","#44aaff");
    add(SK_TRI,1,"Triple Fleche","3 fleches en eventail (-15% dmg)","x3","#44ffbb");
    add(SK_FST,1,"Tir Rapide","Cadence +22%",">>","#ffff44");
    add(SK_PRC,1,"Perforant","Fleches traversent 1 ennemi","->","#ff44aa");
    add(SK_SPD,1,"Sprint","Vitesse +15%","~~","#88ff88");
    add(SK_POW,1,"Force","Degats +30%","!!","#ff8844");
    add(SK_BNC,1,"Ricochet","Fleches rebondissent x1","<>","#ffaaff");
    add(SK_DIA,1,"Oblique","Tir diagonal aussi","X","#cc44ff");
    add(SK_HARDCORE,1,"Hardcore","+1 PV max","+H","#ff6699");
    add(SK_DODGE,1,"Coup d'oeil","6% chance d'esquive","??","#88ccff");
    add(SK_LIGHT_FOOT,1,"Pied leger","Vitesse +8%","..","#bbff99");
    add(SK_RANGE,1,"Portee","Fleches +30% rapides",">>","#ddee77");
    add(SK_KNOCKBACK,1,"Repousse","Fleches projettent",">!","#aabbcc");
    add(SK_BIG_ARROW,1,"Pointes","Fleches +20% degats, -8% cadence","[]","#cc9966");
    add(SK_HUNTER,1,"Chasseur","+25% degats si distance > 200","-O","#aaff77");
    add(SK_SHARP,1,"Aiguisage","+8% degats","/'","#ffddaa");
    add(SK_FIRST_HIT,1,"Premier Coup","1ere fleche d'1 salve +50%","1!","#ffcc88");
    add(SK_FOCUSED,1,"Concentration","+25% degats si immobile 1s","[*","#9999ff");

    // ─── World 2 (13 skills) ───
    add(SK_GRN,2,"Lance-grenade","Touche G - 50 dmg radius 70","*","#ff6622");
    add(SK_LIFESTEAL,2,"Vol de vie","4% des dmg infliges en PV","Vh","#aa1144");
    add(SK_NECRO,2,"Necromancie","12% chance esprit allie sur kill","sP","#7755aa");
    add(SK_DEATH_MARK,2,"Marque","Tuer = prochain ennemi +30%","Mk","#aa4477");
    add(SK_BONE_SHIELD,2,"Bouclier osseux","+1 PV regen / 8s en combat","[]","#ccccaa");
    add(SK_BLOOD_PACT,2,"Pacte de sang","+25% degats mais -1 PV max","BP","#dd0033");
    add(SK_REGEN,2,"Regeneration","+1 PV / 5s","RG","#66dd66");
    add(SK_RESILIENCE,2,"Resilience","Invincibilite +50%","Rs","#ddddaa");
    add(SK_PHOENIX,2,"Phenix","Revive 50% PV (1x/run)","Ph","#ff7733");
    add(SK_SOUL_DRAIN,2,"Drain","+1 PV par kill (cd 5s)","sD","#cc66cc");
    add(SK_DECAY,2,"Pourriture","Ennemis a 60px perdent 1 PV/s","D-","#778822");
    add(SK_VAMP_NERFED,2,"Vampirisme","12% chance soin sur kill","V+","#aa1166");
    add(SK_FROST_FATE,2,"Gel Renforce","Duree de gel +50%","F+","#aaddff");

    // ─── World 3 (12 skills) ───
    add(SK_POISON,3,"Poison","Fleches : 4 dmg/s pendant 3s","Pn","#88ff44");
    add(SK_BLAZE,3,"Embrasement","20% chance brulure 3dmg/s 4s","Bz","#ff8822");
    add(SK_EXPLODE_KILL,3,"Boom!","Kills explosent 12 dmg / 45px","EX","#ff5500");
    add(SK_FIRE_TRAIL,3,"Trainee","Tu laisses du feu 4dmg/s","FT","#ff4400");
    add(SK_DOUBLE_GRENADE,3,"Bi-grenade","Lance 2 grenades a la fois","2*","#ff6622");
    add(SK_BIG_BOOM,3,"Big Boom","Explosions +35% rayon","BB","#ff7711");
    add(SK_VOLCANO,3,"Volcan","8% pluie de feu sur kill","Vc","#dd3300");
    add(SK_RECKLESS,3,"Imprudence","+40% degats mais +25% subis","Rk","#cc0000");
    add(SK_BURST_FIRE,3,"Rafale","25% chance double tir","Bf","#ffcc00");
    add(SK_INFERNO,3,"Inferno","Brules explosent en mourant","In","#ff5522");
    add(SK_STICKY,3,"Glu acide","Fleches ralentissent ennemis 1s","Gl","#88ff44");
    add(SK_GHOST_ARROW,3,"Fleche fantome","Fleches traversent les murs","Gh","#aaaaff");

    // ─── World 4 (12 skills) ───
    add(SK_DASH,4,"Dash","Touche SHIFT - immune 0.4s, cd 3s","->","#00ddff");
    add(SK_RAM,4,"Belier","Le dash inflige 25 dmg","BR","#ddaa44");
    add(SK_BERSERKER,4,"Berserker","+50% degats sous 35% PV","BS","#cc1133");
    add(SK_HEAVY,4,"Lourd","+45% degats, -18% cadence","HV","#996644");
    add(SK_THORNS,4,"Epines","50% des dmg subis renvoyes au contact","Tn","#557722");
    add(SK_TANK,4,"Tank","+3 PV max, -10% vitesse","Tk","#777799");
    add(SK_RAGE_STACK,4,"Rage","+5% degats par dmg subi (max 50%)","Rg","#dd2244");
    add(SK_SHIELD,4,"Bouclier","Bloque 1 coup, cd 12s","Sh","#aabbdd");
    add(SK_LAST_HOPE,4,"Derniere chance","0 PV : 50% chance de survie a 1 PV","Lh","#dd9933");
    add(SK_ADRENALINE,4,"Adrenaline","+30% vitesse 3s apres dmg","Ad","#dd44aa");
    add(SK_REFLECT,4,"Reflet","20% chance de renvoyer un tir","Rf","#aaccee");
    add(SK_FINISHER,4,"Acheveur","+35% sur ennemis < 30% PV","Fn","#aa3322");

    // ─── World 5 (12 skills - LEGENDARY) ───
    add(SK_FROST_AURA,5,"Aura glaciale","Aura 80px ralentit ennemis 30%","FA","#88ccff");
    add(SK_FREEZE_HIT,5,"Givre total","12% chance gel complet 1s","FH","#aaeeff");
    add(SK_ICE_SHARDS,5,"Eclats de glace","20% chance fleche -> 4 shards","IS","#bbeeff");
    add(SK_TIME_STOP,5,"Arret du temps","Touche T : ralenti 4s (1x/run)","TS","#7799ff");
    add(SK_LEGEND,5,"Legende","+1 a toutes les stats par boss tue","Lg","#ffd700");
    add(SK_MASTERY,5,"Maitrise","Tous les autres skills +18%","Ma","#ffcc44");
    add(SK_ECHO,5,"Echo","18% chance fleche tire 2x","Ec","#bbaaff");
    add(SK_INVUL_BURST,5,"Aegis","4s invul a l'entree d'une salle de boss","Ag","#dddd88");
    add(SK_FROST_NOVA,5,"Frost Nova","Tous les 5 kills, gele tous 2s","Fn","#aaddff");
    add(SK_HOMING,5,"Tete-chercheuse","Fleches incurvent vers cible","H>","#bb88ff");
    add(SK_CRIT_NERFED,5,"Critique","18% chance x1.7 degats","C!","#ffdd00");
    add(SK_QUAD_NERFED,5,"Quadruple","4 fleches paralleles -25% dmg","x4","#ff77ff");

    // Anciens nerfes (gardes, world 3 et 5)
    add(SK_SPLT_NERFED,3,"Eclats","Fleches eclatent en 2 (-65% dmg)","<*","#66ddff");
    add(SK_FRZ_NERFED,2,"Glacant","Ralentit 1s sur hit","*~","#88ccff");
    add(SK_MAXHP_NERFED,2,"Vitalite","+1 PV max + soin total","++","#ff3366");

    m_elapsed.start();
    m_lastTickNs = m_elapsed.nsecsElapsed();
    connect(&m_timer, &QTimer::timeout, this, &GameWidget::tick);
    m_timer.start(16);
}

GameWidget::~GameWidget() {}

// ============================================================
//  HELPERS hasSkill / multipliers
// ============================================================
bool GameWidget::hasSkill(int id) const {
    return std::find(m_player.skills.begin(), m_player.skills.end(), id) != m_player.skills.end();
}
int GameWidget::skillStacks(int id) const {
    return (int)std::count(m_player.skills.begin(), m_player.skills.end(), id);
}

float GameWidget::damageMul() const {
    float m = 1.f;
    if (hasSkill(SK_POW))       m *= 1.30f;
    if (hasSkill(SK_SHARP))     m *= 1.08f;
    if (hasSkill(SK_BIG_ARROW)) m *= 1.20f;
    if (hasSkill(SK_BLOOD_PACT))m *= 1.25f;
    if (hasSkill(SK_HEAVY))     m *= 1.45f;
    if (hasSkill(SK_RECKLESS))  m *= 1.40f;
    if (hasSkill(SK_BERSERKER) && m_player.hp < m_player.maxHp * 0.35f) m *= 1.5f;
    if (hasSkill(SK_RAGE_STACK)) m *= (1.f + 0.05f * std::min(10, m_player.rageStacks));
    if (hasSkill(SK_LEGEND))    m *= (1.f + 0.10f * m_player.bossesKilled);
    if (hasSkill(SK_MASTERY))   m *= 1.18f;
    if (hasSkill(SK_FOCUSED) && m_player.stillSec >= 1.f) m *= 1.25f;
    return m;
}
float GameWidget::speedMul() const {
    float m = 1.f;
    if (hasSkill(SK_SPD))        m *= 1.15f;
    if (hasSkill(SK_LIGHT_FOOT)) m *= 1.08f;
    if (hasSkill(SK_TANK))       m *= 0.90f;
    if (m_player.adrenalineTimer > 0) m *= 1.30f;
    if (hasSkill(SK_LEGEND))     m *= (1.f + 0.05f * m_player.bossesKilled);
    if (hasSkill(SK_MASTERY))    m *= 1.18f;
    return m;
}
float GameWidget::fireRateMul() const {
    float m = 1.f;
    if (hasSkill(SK_FST))        m *= 0.78f;
    if (hasSkill(SK_BIG_ARROW))  m *= 1.08f;
    if (hasSkill(SK_HEAVY))      m *= 1.18f;
    if (m_player.hp < m_player.maxHp * 0.25f && hasSkill(SK_LAST_HOPE)) m *= 0.50f;
    if (hasSkill(SK_MASTERY))    m *= 0.85f;
    return m;
}
float GameWidget::incomingDmgMul() const {
    float m = 1.f;
    if (hasSkill(SK_RECKLESS)) m *= 1.25f;
    if (hasSkill(SK_TANK))     m *= 0.92f;
    return m;
}
int GameWidget::pierceLevel() const {
    int p = 0;
    if (hasSkill(SK_PRC)) p = 1;
    return p;
}
int GameWidget::bounceLevel() const {
    int b = 0;
    if (hasSkill(SK_BNC)) b = 1;
    return b;
}


// ============================================================
//  ASSETS / SKINS / WORLDS
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
                v = std::min(255, std::max(0, int(v*val)));
                c.setHsv(0, 0, v, a);
            } else {
                h = (h + degrees + 360) % 360;
                s = std::min(255, std::max(0, int(s*sat)));
                v = std::min(255, std::max(0, int(v*val)));
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
    rawOrc[AN_Idle]=QImage(":/sprites/orc_idle.png");
    rawOrc[AN_Walk]=QImage(":/sprites/orc_walk.png");
    rawOrc[AN_Atk]=QImage(":/sprites/orc_atk.png");
    rawOrc[AN_Hurt]=QImage(":/sprites/orc_hurt.png");
    rawOrc[AN_Death]=QImage(":/sprites/orc_death.png");
    for (int a = 0; a < 5; ++a) {
        m_sprOrc[0][a] = hueShift(rawOrc[a], 120, 1.8, 1.0);
        m_sprOrc[1][a] = hueShift(rawOrc[a],  40, 0.4, 1.3);
        m_sprOrc[2][a] = hueShift(rawOrc[a], 260, 2.0, 1.0);
        m_sprOrc[3][a] = hueShift(rawOrc[a],   0, 0.6, 0.7);
        m_sprOrc[4][a] = hueShift(rawOrc[a], 200, 2.0, 1.0);
        m_sprOrc[5][a] = hueShift(rawOrc[a], 350, 2.0, 0.9);
    }
    // Boss sheets
    m_bossSheets["fireworm"]      = QImage(":/sprites/boss_fireworm.png");
    m_bossSheets["undead"]        = QImage(":/sprites/boss_undead.png");
    m_bossSheets["demonslime"]    = QImage(":/sprites/boss_demonslime.png");
    m_bossSheets["mino"]          = QImage(":/sprites/boss_mino.png");
    m_bossSheets["frostguardian"] = QImage(":/sprites/boss_frostguardian.png");
}

void GameWidget::loadWorlds()
{
    m_worlds.clear();
    m_worlds.append({"Forge ardente",     "Ouverture - le Ver du magma",
                     "fireworm", "Ver de Feu",            9,  90,  90, 8.f,  QColor("#ff6600")});
    m_worlds.append({"Catacombes",        "Le marechal des morts",
                     "undead",   "Executeur des Morts",   5, 100, 100, 7.f,  QColor("#88aacc")});
    m_worlds.append({"Marais maudit",     "Le seigneur visqueux",
                     "demonslime","Demon Vase",           6, 288, 160, 8.f,  QColor("#aa44ff")});
    m_worlds.append({"Arene de l'anneau", "Le titan a cornes",
                     "mino",     "Minotaure",            16, 288, 160, 10.f, QColor("#cc8822")});
    m_worlds.append({"Citadelle gelee",   "Le Gardien du Givre - tu vas souffrir",
                     "frostguardian","Gardien du Givre",  6, 192, 128, 8.f,  QColor("#44ccff")});
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
void GameWidget::loadSettings() {
    QSettings s("RetroArcher", "RetroArcher");
    m_highScore        = s.value("highScore", 0).toInt();
    m_worldsUnlocked   = std::max(1, std::min(5, s.value("worldsUnlocked", 1).toInt()));
    m_menuSelectedSkin = s.value("skin", 0).toInt() % 4;
    m_menuSelectedAtk  = s.value("atk", 0).toInt() % 3;
}
void GameWidget::saveSettings() {
    QSettings s("RetroArcher", "RetroArcher");
    s.setValue("highScore", m_highScore);
    s.setValue("worldsUnlocked", m_worldsUnlocked);
}

// ============================================================
//  ROOM HELPERS
// ============================================================
int GameWidget::worldOf(int room) const {
    return std::min(5, ((room - 1) / 5) + 1);
}
bool GameWidget::isBossRoom(int room) const { return room % 5 == 0; }
bool GameWidget::isFinalBossRoom(int room) const { return room == 25; }

void GameWidget::spawnMinion(float x, float y, int hp, float speed, EnemyType type) {
    Enemy m;
    m.id = m_nextEnemyId++;
    m.x = x; m.y = y;
    m.hp = m.maxHp = hp;
    m.speed = speed;
    m.size = 26;
    m.type = type;
    m.damage = 1;
    m.shootCd = (type == ET_Skel) ? rndF(2.f, 3.5f) : 0;
    m.zigDir = (rndI(0,1)==0) ? 1.f : -1.f;
    m.anim = AN_Walk;
    m_enemies.push_back(m);
}

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

    if      (entryDoor==0) { m_player.x=DOOR_COL*TILE+TILE/2.f; m_player.y=TILE*2.5f; }
    else if (entryDoor==1) { m_player.x=GW-TILE*2.5f; m_player.y=DOOR_ROW*TILE+TILE/2.f; }
    else if (entryDoor==2) { m_player.x=DOOR_COL*TILE+TILE/2.f; m_player.y=GH-TILE*2.5f; }
    else if (entryDoor==3) { m_player.x=TILE*2.5f; m_player.y=DOOR_ROW*TILE+TILE/2.f; }
    else                   { m_player.x=GW/2.f; m_player.y=GH/2.f; }

    m_layoutIdx = 0;
    resolveCollision(m_player.x, m_player.y, 8.f);

    int worldIdx = worldOf(roomIndex) - 1;

    // SK_INVUL_BURST : 4 sec d'invul a l'entree d'une salle de boss
    if (isBossRoom(roomIndex) && hasSkill(SK_INVUL_BURST)) {
        m_player.invulBurstTimer = 4.f;
        m_player.invincibility = std::max(m_player.invincibility, 4.f);
    }

    // ─── BOSS ROOMS ───
    if (isBossRoom(roomIndex)) {
        Enemy boss;
        boss.id = m_nextEnemyId++;
        boss.x = GW / 2.f;
        boss.y = GH / 2.f - 30;
        boss.subType = worldIdx;       // 0..4
        boss.phase = 1;
        boss.moveCd = rndF(0.8f, 1.6f);
        boss.moveTargetX = boss.x;
        boss.moveTargetY = boss.y;

        if (worldIdx == 0) {           // Ver de Feu - facile
            boss.hp = boss.maxHp = 250;
            boss.speed = 55; boss.damage = 1; boss.size = 60;
            boss.shootCd = 1.4f; boss.burstCd = 6;
            boss.chargeCd = 5.f;
            boss.type = ET_MiniBoss;
        } else if (worldIdx == 1) {    // Executeur des Morts
            boss.hp = boss.maxHp = 450;
            boss.speed = 45; boss.damage = 2; boss.size = 64;
            boss.shootCd = 1.2f; boss.burstCd = 5;
            boss.summonCd = 6.f;
            boss.type = ET_MiniBoss;
        } else if (worldIdx == 2) {    // Demon Vase
            boss.hp = boss.maxHp = 650;
            boss.speed = 60; boss.damage = 2; boss.size = 70;
            boss.shootCd = 1.0f; boss.burstCd = 4.5f;
            boss.specialCd = 4.f;       // teleport
            boss.type = ET_MiniBoss;
        } else if (worldIdx == 3) {    // Minotaure
            boss.hp = boss.maxHp = 900;
            boss.speed = 70; boss.damage = 2; boss.size = 78;
            boss.shootCd = 1.6f;
            boss.chargeCd = 3.f;
            boss.type = ET_MiniBoss;
        } else {                        // Gardien du Givre - EXTREME
            boss.hp = boss.maxHp = 1800;
            boss.speed = 80; boss.damage = 2; boss.size = 84;
            boss.shootCd = 0.7f; boss.burstCd = 3.5f;
            boss.specialCd = 5.f; boss.summonCd = 6.f; boss.chargeCd = 4.5f;
            boss.type = ET_FinalBoss;
        }
        m_enemies.push_back(boss);
        m_bossIndex = (int)m_enemies.size() - 1;
        return;
    }

    // ─── ROOMS NORMALES ───
    int roomInWorld = ((roomIndex - 1) % 5) + 1;   // 1..4 (5 = boss)
    int count = 3 + roomInWorld + worldIdx;
    if (count > 12) count = 12;

    for (int i = 0; i < count; ++i) {
        float ex, ey; int safety = 50;
        do {
            ex = rndF(TILE*2.5f, GW-TILE*2.5f);
            ey = rndF(TILE*2.5f, GH-TILE*2.5f);
            safety--;
        } while ((distF(ex,ey,m_player.x,m_player.y) < 130.f
                  || collidesObstacle(ex,ey,18)) && safety > 0);

        Enemy e;
        e.id = m_nextEnemyId++;
        e.x = ex; e.y = ey;
        float r = QRandomGenerator::global()->generateDouble();

        // Mix d'ennemis par monde
        if (worldIdx == 0) {  // World 1: slime/bat
            if (r < 0.6) { e.type=ET_Slime; e.hp=e.maxHp=30; e.speed=rndF(45,65); e.size=32; }
            else { e.type=ET_Bat; e.hp=e.maxHp=22; e.speed=rndF(55,75); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
        } else if (worldIdx == 1) {  // World 2: + skel
            if (r < 0.30) { e.type=ET_Slime; e.hp=e.maxHp=40; e.speed=rndF(50,70); e.size=32; }
            else if (r < 0.65) { e.type=ET_Skel; e.hp=e.maxHp=55; e.speed=rndF(45,60); e.size=32; e.shootCd=rndF(1.5f,2.5f); }
            else { e.type=ET_Bat; e.hp=e.maxHp=28; e.speed=rndF(58,78); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
        } else if (worldIdx == 2) {  // World 3: + brute/mage
            if (r < 0.20) { e.type=ET_Slime; e.hp=e.maxHp=50; e.speed=rndF(55,75); e.size=32; }
            else if (r < 0.45) { e.type=ET_Skel; e.hp=e.maxHp=70; e.speed=rndF(45,60); e.size=32; e.shootCd=rndF(1.3f,2.2f); }
            else if (r < 0.65) { e.type=ET_Bat; e.hp=e.maxHp=35; e.speed=rndF(60,80); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
            else if (r < 0.85) { e.type=ET_Brute; e.hp=e.maxHp=110; e.speed=rndF(35,50); e.size=36; e.damage=2; }
            else { e.type=ET_Mage; e.hp=e.maxHp=70; e.speed=rndF(35,50); e.size=32; e.shootCd=rndF(1.8f,2.8f); }
        } else if (worldIdx == 3) {  // World 4: tougher mix
            if (r < 0.15) { e.type=ET_Slime; e.hp=e.maxHp=70; e.speed=rndF(60,80); e.size=32; }
            else if (r < 0.35) { e.type=ET_Skel; e.hp=e.maxHp=90; e.speed=rndF(50,65); e.size=32; e.shootCd=rndF(1.1f,2.0f); }
            else if (r < 0.55) { e.type=ET_Bat; e.hp=e.maxHp=45; e.speed=rndF(65,90); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
            else if (r < 0.80) { e.type=ET_Brute; e.hp=e.maxHp=160; e.speed=rndF(40,55); e.size=36; e.damage=2; }
            else { e.type=ET_Mage; e.hp=e.maxHp=100; e.speed=rndF(40,55); e.size=32; e.shootCd=rndF(1.4f,2.4f); }
        } else {  // World 5: very tough
            if (r < 0.10) { e.type=ET_Slime; e.hp=e.maxHp=90; e.speed=rndF(65,85); e.size=32; }
            else if (r < 0.30) { e.type=ET_Skel; e.hp=e.maxHp=120; e.speed=rndF(55,70); e.size=32; e.shootCd=rndF(1.0f,1.8f); }
            else if (r < 0.50) { e.type=ET_Bat; e.hp=e.maxHp=60; e.speed=rndF(75,100); e.size=28; e.zigDir=(rndI(0,1)==0)?1.f:-1.f; }
            else if (r < 0.75) { e.type=ET_Brute; e.hp=e.maxHp=210; e.speed=rndF(45,60); e.size=36; e.damage=2; }
            else { e.type=ET_Mage; e.hp=e.maxHp=140; e.speed=rndF(45,60); e.size=32; e.shootCd=rndF(1.2f,2.0f); }
        }
        e.anim = AN_Walk;
        m_enemies.push_back(e);
    }
}

// ============================================================
//  COLLISION
// ============================================================
bool GameWidget::isObstacleTile(int col, int row) const
{
    if (col<0||col>=COLS||row<0||row>=ROWS) return true;
    bool isPerim = (col==0||col==COLS-1||row==0||row==ROWS-1);
    if (isPerim) {
        if (row==0      && col==DOOR_COL && m_doorOpen[0]) return false;
        if (col==COLS-1 && row==DOOR_ROW && m_doorOpen[1]) return false;
        if (row==ROWS-1 && col==DOOR_COL && m_doorOpen[2]) return false;
        if (col==0      && row==DOOR_ROW && m_doorOpen[3]) return false;
        return true;
    }
    return false;
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
            if (dx*dx+dy*dy < r*r) return true;
        }
    return false;
}

void GameWidget::resolveCollision(float &x, float &y, float r)
{
    for (int iter=0; iter<8; ++iter) {
        bool collided=false;
        int c0=std::max(0,int((x-r)/TILE)), c1=std::min(COLS-1,int((x+r)/TILE));
        int r0=std::max(0,int((y-r)/TILE)), r1=std::min(ROWS-1,int((y+r)/TILE));
        for (int row=r0;row<=r1;++row)
            for (int col=c0;col<=c1;++col) {
                if (!isObstacleTile(col,row)) continue;
                float tx0=col*TILE,ty0=row*TILE,tx1=tx0+TILE,ty1=ty0+TILE;
                float cx=clampF(x,tx0,tx1),cy=clampF(y,ty0,ty1);
                float dx=x-cx,dy=y-cy,d2=dx*dx+dy*dy;
                if (d2<r*r && d2>0.0001f) {
                    float d=std::sqrt(d2),push=r-d+0.5f;
                    x+=(dx/d)*push; y+=(dy/d)*push; collided=true;
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
    for (int i=0; i<steps; ++i) {
        float nx=x+sx;
        if (!collidesObstacle(nx,y,r)) x=nx;
        float ny=y+sy;
        if (!collidesObstacle(x,ny,r)) y=ny;
    }
}


// ============================================================
//  TICK / UPDATE
// ============================================================
void GameWidget::tick()
{
    qint64 now = m_elapsed.nsecsElapsed();
    float dt = std::min(0.05f, float(now-m_lastTickNs)/1e9f);
    m_lastTickNs = now;
    m_globalTime += dt;

    if (m_state==GS_Playing || m_state==GS_RoomCleared) {
        updateGame(dt);
    } else if (m_state==GS_FadeOut) {
        m_fadeAmount += dt/0.4f;
        if (m_fadeAmount >= 1.f) {
            m_fadeAmount = 1.f;
            if (m_currentRoom+1 > ROOMS_TOTAL) {
                m_state = GS_Victory;
                if (m_currentRoom > m_highScore) m_highScore = m_currentRoom;
                m_worldsUnlocked = 5;
                saveSettings();
            } else { generateSkills(); m_state=GS_SkillSelect; }
        }
    } else if (m_state==GS_FadeIn) {
        m_fadeAmount -= dt/0.4f;
        if (m_fadeAmount <= 0.f) { m_fadeAmount=0.f; m_state=GS_Playing; }
        for (auto &p:m_particles) { p.x+=p.vx*dt; p.y+=p.vy*dt; p.life-=dt; }
    }
    update();
}

void GameWidget::updateGame(float dt)
{
    // Time stop : ralenti tous sauf le joueur
    float worldDt = dt;
    if (m_player.timeStopActive > 0) worldDt *= 0.25f;

    updatePlayer(dt);
    updateEnemies(worldDt);
    updateBullets(worldDt);
    updateParticles(dt);

    // SK_FIRE_TRAIL : laisse trainee de feu (en jeu)
    if (hasSkill(SK_FIRE_TRAIL) && m_player.moving && (int)(m_globalTime*8) % 2 == 0) {
        Particle p;
        p.x = m_player.x + rndF(-6,6); p.y = m_player.y + rndF(-4,8);
        p.vx = rndF(-15,15); p.vy = rndF(-30,-10);
        p.color = (rndI(0,1)==0)?PAL_FIRE[0]:PAL_FIRE[1];
        p.life = p.maxLife = 0.7f; p.size = 4; p.noGravity = true;
        m_particles.push_back(p);
        // Inflige des dmg aux ennemis proches
        for (auto &e : m_enemies) {
            if (e.dead) continue;
            if (distF(e.x, e.y, m_player.x, m_player.y) < 30) {
                if ((int)(m_globalTime*5) != (int)((m_globalTime-dt)*5))
                    hurtEnemy(e, 4 * dt * 5);
            }
        }
    }

    // SK_DECAY : ennemis a 60px perdent 1 PV/s
    if (hasSkill(SK_DECAY)) {
        for (auto &e : m_enemies) {
            if (e.dead) continue;
            if (distF(e.x, e.y, m_player.x, m_player.y) < 60) {
                e.hp -= dt * 1.f;
                if (e.hp <= 0 && !e.dead) hurtEnemy(e, 0.01f);
            }
        }
    }

    // SK_REGEN : +1 PV / 5s
    if (hasSkill(SK_REGEN)) {
        m_player.regenTimer += dt;
        if (m_player.regenTimer >= 5.f) {
            m_player.regenTimer = 0;
            m_player.hp = std::min(m_player.maxHp, m_player.hp + 1.f);
        }
    }
    // SK_BONE_SHIELD : +1 PV / 8s
    if (hasSkill(SK_BONE_SHIELD)) {
        m_player.regenTimer += dt;
        if (m_player.regenTimer >= 8.f) {
            m_player.regenTimer = 0;
            m_player.hp = std::min(m_player.maxHp, m_player.hp + 1.f);
        }
    }

    if (m_player.adrenalineTimer > 0) m_player.adrenalineTimer -= dt;
    if (m_player.regenAfterKillCd > 0) m_player.regenAfterKillCd -= dt;
    if (m_player.killStreakTimer > 0) {
        m_player.killStreakTimer -= dt;
        if (m_player.killStreakTimer <= 0) m_player.killStreakCount = 0;
    }
    if (m_player.shieldReadyTimer > 0) m_player.shieldReadyTimer -= dt;
    if (m_player.invulBurstTimer > 0)  m_player.invulBurstTimer  -= dt;
    if (m_player.timeStopActive > 0)   m_player.timeStopActive   -= dt;
    if (m_player.timeStopCd > 0)       m_player.timeStopCd       -= dt;
    if (m_player.dashCd > 0)           m_player.dashCd           -= dt;
    if (m_player.dashActive > 0)       m_player.dashActive       -= dt;

    // Cleanup corpses
    for (auto &e : m_enemies)
        if (e.dead && e.anim==AN_Death) {
            e.corpseFadeTimer += dt;
            if (e.corpseFadeTimer > 0.6f) e.corpse = true;
        }
    m_enemies.erase(std::remove_if(m_enemies.begin(),m_enemies.end(),
                    [](const Enemy &e){return e.corpse;}),m_enemies.end());

    if (m_player.hp <= 0 && m_state != GS_GameOver) {
        // SK_PHOENIX
        if (hasSkill(SK_PHOENIX) && !m_player.phoenixUsed) {
            m_player.phoenixUsed = true;
            m_player.hp = m_player.maxHp * 0.5f;
            m_player.invincibility = 2.5f;
            for (int i=0;i<24;++i) {
                float a = rndF(0, 6.28f);
                Particle p; p.x = m_player.x; p.y = m_player.y;
                p.vx = std::cos(a)*rndF(80,180); p.vy = std::sin(a)*rndF(80,180);
                p.color = QColor("#ff7733"); p.life = p.maxLife = 1.f;
                p.size = 5; p.noGravity = true;
                m_particles.push_back(p);
            }
            return;
        }
        // SK_LAST_HOPE : 50% chance de survie a 1 PV
        if (hasSkill(SK_LAST_HOPE) && QRandomGenerator::global()->generateDouble() < 0.5) {
            m_player.hp = 1; m_player.invincibility = 1.5f;
            return;
        }
        m_state = GS_GameOver;
        if (m_currentRoom > m_highScore) { m_highScore = m_currentRoom; }
        saveSettings();
        return;
    }

    if (m_state == GS_Playing) {
        bool anyAlive = false;
        for (auto &e : m_enemies) if (!e.dead) { anyAlive = true; break; }
        if (!anyAlive) {
            // Boss tue ?
            if (isBossRoom(m_currentRoom)) {
                m_player.bossesKilled++;
                int worldCleared = worldOf(m_currentRoom);
                if (worldCleared >= m_worldsUnlocked && m_worldsUnlocked < 5) {
                    m_worldsUnlocked = worldCleared + 1;
                    saveSettings();
                }
                if (isFinalBossRoom(m_currentRoom)) {
                    m_state = GS_Victory;
                    if (m_currentRoom > m_highScore) m_highScore = m_currentRoom;
                    saveSettings();
                    return;
                }
            }
            m_state = GS_RoomCleared;
            for (int i = 0; i < 4; ++i) m_doorOpen[i] = true;
        }
    }
    if (m_state == GS_RoomCleared) checkDoorTransition();
}

// ============================================================
//  PLAYER UPDATE
// ============================================================
void GameWidget::updatePlayer(float dt)
{
    Player &pl = m_player;
    pl.animTimer += dt;
    if (pl.atkTimer > 0) pl.atkTimer -= dt;
    if (pl.invincibility > 0) pl.invincibility -= dt;
    if (pl.grenadeCd > 0) pl.grenadeCd -= dt;

    float vx = 0, vy = 0;
    if (m_keys.contains(Qt::Key_Left) ||m_keys.contains(Qt::Key_A)||m_keys.contains(Qt::Key_Q)) vx -= 1;
    if (m_keys.contains(Qt::Key_Right)||m_keys.contains(Qt::Key_D)) vx += 1;
    if (m_keys.contains(Qt::Key_Up)   ||m_keys.contains(Qt::Key_W)||m_keys.contains(Qt::Key_Z)) vy -= 1;
    if (m_keys.contains(Qt::Key_Down) ||m_keys.contains(Qt::Key_S)) vy += 1;

    float len = std::hypot(vx, vy);
    pl.moving = (len > 0);
    pl.stillSec = pl.moving ? 0 : (pl.stillSec + dt);

    float spd = pl.speed * speedMul();
    if (pl.dashActive > 0) spd *= 4.5f;       // dash actif

    if (pl.moving) {
        vx /= len; vy /= len;
        tryMove(pl.x, pl.y, vx*spd*dt, vy*spd*dt, 8.f);
        pl.facing = std::atan2(vy, vx);
        pl.facingLeft = vx < 0;
        if (pl.anim != AN_Walk) { pl.anim = AN_Walk; pl.animTimer = 0; }
        // SK_RAM : dash inflige des dmg
        if (hasSkill(SK_RAM) && pl.dashActive > 0) {
            for (auto &e : m_enemies) {
                if (e.dead) continue;
                if (distF(e.x, e.y, pl.x, pl.y) < 22 + e.size*0.4f) {
                    if (e.hitFlash <= 0.05f) hurtEnemy(e, 25);
                }
            }
        }
    } else {
        if (pl.atkTimer<=0 && pl.anim!=AN_Hurt) pl.anim = AN_Idle;
    }
    pl.x = clampF(pl.x, -TILE, GW+TILE);
    pl.y = clampF(pl.y, -TILE, GH+TILE);

    pl.fireCd -= dt;
    bool anyEnemy = false;
    for (auto &e : m_enemies) if (!e.dead) { anyEnemy = true; break; }
    float effFireRate = pl.fireRate * fireRateMul();
    if (!pl.moving && pl.fireCd <= 0 && anyEnemy) {
        pl.fireCd = effFireRate;
        shootPlayer();
        pl.anim = AN_Atk; pl.animTimer = 0; pl.atkTimer = 0.4f;
        // SK_BURST_FIRE : 25% chance double tir
        if (hasSkill(SK_BURST_FIRE) && QRandomGenerator::global()->generateDouble() < 0.25)
            pl.burstQueueTimer = 0.10f;
    }
    if (pl.burstQueueTimer > 0) {
        pl.burstQueueTimer -= dt;
        if (pl.burstQueueTimer <= 0 && anyEnemy) shootPlayer();
    }
    if (pl.moving && pl.fireCd < effFireRate*0.5f) pl.fireCd = effFireRate*0.5f;

    if (m_keys.contains(Qt::Key_G) && pl.grenadeAmmo>0 && pl.grenadeCd<=0) {
        throwGrenade(); pl.grenadeAmmo--; pl.grenadeCd = 0.3f;
        if (hasSkill(SK_DOUBLE_GRENADE) && pl.grenadeAmmo > 0) {
            throwGrenade(); pl.grenadeAmmo--;
        }
    }
    if (m_keys.contains(Qt::Key_Shift) && hasSkill(SK_DASH) && pl.dashCd <= 0 && pl.dashActive <= 0) {
        activateDash();
    }
    if (m_keys.contains(Qt::Key_T) && hasSkill(SK_TIME_STOP) && !pl.timeStopUsed) {
        activateTimeStop();
    }

    // Touch dmg
    if (pl.invincibility <= 0 && pl.dashActive <= 0) {
        for (auto &e : m_enemies) {
            if (e.dead) continue;
            if (distF(pl.x, pl.y, e.x, e.y) < 10 + e.size*0.35f) {
                // SK_DODGE
                if (hasSkill(SK_DODGE) && QRandomGenerator::global()->generateDouble()<0.06) break;
                // SK_SHIELD
                if (hasSkill(SK_SHIELD) && pl.shieldReadyTimer <= 0) {
                    pl.shieldReadyTimer = 12.f; pl.invincibility = 1.f; break;
                }
                // SK_THORNS : renvoie 50% au contact
                if (hasSkill(SK_THORNS)) hurtEnemy(e, e.damage * 0.5f);
                hurtPlayer(e.damage * incomingDmgMul());
                break;
            }
        }
        for (auto &b : m_bullets) {
            if (!b.enemy || b.dead) continue;
            if (distF(pl.x, pl.y, b.x, b.y) < 12) {
                if (hasSkill(SK_DODGE) && QRandomGenerator::global()->generateDouble()<0.06) { b.dead=true; break; }
                if (hasSkill(SK_REFLECT) && QRandomGenerator::global()->generateDouble()<0.20) {
                    b.enemy = false; b.vx *= -1; b.vy *= -1; b.angle += M_PI; continue;
                }
                if (hasSkill(SK_SHIELD) && pl.shieldReadyTimer <= 0) {
                    pl.shieldReadyTimer = 12.f; pl.invincibility = 1.f; b.dead=true; break;
                }
                b.dead = true;
                hurtPlayer(b.damage * incomingDmgMul());
            }
        }
    }
}

void GameWidget::activateDash()
{
    m_player.dashActive = 0.4f;
    m_player.dashCd = 3.f;
    for (int i=0;i<14;++i) {
        Particle p; p.x = m_player.x + rndF(-6,6); p.y = m_player.y + rndF(-4,8);
        p.vx = rndF(-30,30); p.vy = rndF(-40,-10);
        p.color = QColor("#00ddff"); p.life=p.maxLife=0.4f; p.size=3; p.noGravity=true;
        m_particles.push_back(p);
    }
}

void GameWidget::activateTimeStop()
{
    m_player.timeStopActive = 4.f;
    m_player.timeStopUsed = true;
    for (int i=0;i<32;++i) {
        Particle p; p.x = m_player.x; p.y = m_player.y;
        float a = rndF(0,6.28f);
        p.vx = std::cos(a)*120; p.vy = std::sin(a)*120;
        p.color = QColor("#7799ff"); p.life=p.maxLife=0.8f; p.size=4; p.noGravity=true;
        m_particles.push_back(p);
    }
}


// ============================================================
//  SHOOT / GRENADE
// ============================================================
void GameWidget::shootPlayer()
{
    Player &pl = m_player;
    Enemy *near = nullptr;
    float nd = 1e9f;
    for (auto &e : m_enemies) {
        if (e.dead) continue;
        // Nearest enemy or marked one if SK_DEATH_MARK
        float d = distF(pl.x, pl.y, e.x, e.y);
        if (pl.nextEnemyMarked == e.id) { near = &e; break; }
        if (d < nd) { nd = d; near = &e; }
    }
    if (!near) return;

    bool prc = pierceLevel() > 0;
    int  bnc = bounceLevel();
    bool tri = hasSkill(SK_TRI);
    bool dbl = hasSkill(SK_DBL);
    bool dia = hasSkill(SK_DIA);
    bool quad= hasSkill(SK_QUAD_NERFED);
    bool crit= hasSkill(SK_CRIT_NERFED);
    bool icy = hasSkill(SK_FRZ_NERFED) || hasSkill(SK_FREEZE_HIT) || hasSkill(SK_FROST_AURA);
    bool poi = hasSkill(SK_POISON);
    bool brn = hasSkill(SK_BLAZE);
    bool hom = hasSkill(SK_HOMING);
    bool ghost = hasSkill(SK_GHOST_ARROW);
    float speed = 320.f;
    if (hasSkill(SK_RANGE)) speed *= 1.30f;

    float dmg = pl.damage * damageMul();
    // SK_HUNTER : +25% si distance > 200
    if (hasSkill(SK_HUNTER) && nd > 200.f) dmg *= 1.25f;
    // SK_FOCUSED already in damageMul()
    // SK_DEATH_MARK
    if (hasSkill(SK_DEATH_MARK) && pl.nextEnemyMarked == near->id) {
        dmg *= 1.30f;
        pl.nextEnemyMarked = -1;
    }
    // SK_FINISHER
    if (hasSkill(SK_FINISHER) && near->hp < near->maxHp * 0.30f) dmg *= 1.35f;
    // SK_REVENGE handled via state
    // SK_CRIT
    if (crit && QRandomGenerator::global()->generateDouble() < 0.18) dmg *= 1.7f;

    float a = std::atan2(near->y - pl.y, near->x - pl.x);
    pl.facing = a;
    pl.facingLeft = std::cos(a) < 0;

    // SK_TRI nerf : -15% dmg
    float triDmg = dmg * 0.85f;
    // SK_DBL : 100% dmg
    // SK_QUAD_NERFED : -25% dmg per arrow
    float quadDmg = dmg * 0.75f;

    auto fire = [&](float ang, float ox, float oy, float useDmg) {
        Bullet b;
        b.x = pl.x + ox; b.y = pl.y + oy;
        b.vx = std::cos(ang)*speed; b.vy = std::sin(ang)*speed;
        b.angle = ang;
        b.damage = useDmg;
        b.pierce = prc; b.pierceLeft = pierceLevel();
        b.bounce = bnc;
        b.enemy = false;
        b.icy = icy; b.poison = poi; b.burn = brn; b.homing = hom; b.ghost = ghost;
        m_bullets.push_back(b);
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
        // SK_FIRST_HIT : +50% dmg
        float useDmg = hasSkill(SK_FIRST_HIT) ? dmg * 1.5f : dmg;
        fire(a, 0, 0, useDmg);
    }
    if (dia) { fire(a + M_PI/4, 0, 0, dmg*0.85f); fire(a - M_PI/4, 0, 0, dmg*0.85f); }
    // SK_ECHO : 18% chance double tir avec leger delai
    if (hasSkill(SK_ECHO) && QRandomGenerator::global()->generateDouble() < 0.18)
        pl.burstQueueTimer = 0.08f;
}

void GameWidget::throwGrenade()
{
    float a = m_player.facing;
    Bullet b;
    b.x = m_player.x; b.y = m_player.y;
    b.vx = std::cos(a)*220; b.vy = std::sin(a)*220;
    b.angle = a; b.damage = 0; b.enemy = false;
    b.grenade = true; b.grenadeFuse = 1.0f; b.grenadeTotal = 1.0f;
    m_bullets.push_back(b);
}

// ============================================================
//  HURT
// ============================================================
void GameWidget::hurtPlayer(float d)
{
    if (m_player.invincibility > 0 || m_player.dashActive > 0) return;
    m_player.hp = std::max(0.f, m_player.hp - d);
    float invul = 0.8f;
    if (hasSkill(SK_RESILIENCE)) invul *= 1.5f;
    m_player.invincibility = invul;
    m_player.anim = AN_Hurt; m_player.animTimer = 0; m_player.atkTimer = 0.4f;

    // SK_RAGE_STACK
    if (hasSkill(SK_RAGE_STACK)) {
        m_player.rageStacks = std::min(10, m_player.rageStacks + 1);
        m_player.rageStackTimer = 12.f;
    }
    // SK_ADRENALINE
    if (hasSkill(SK_ADRENALINE)) m_player.adrenalineTimer = 3.f;

    for (int i=0; i<8; ++i) {
        Particle p;
        p.x = m_player.x + rndF(-10,10); p.y = m_player.y + rndF(-10,10);
        p.vx = rndF(-100,100); p.vy = rndF(-140,-40);
        p.color = C_HP; p.life = p.maxLife = rndF(0.3f,0.6f); p.size = (int)rndF(3,6);
        m_particles.push_back(p);
    }
}

void GameWidget::hurtEnemy(Enemy &e, float dmg, bool fromExplosion)
{
    e.hp -= dmg; e.hitFlash = 0.18f;

    // Status effects
    if (hasSkill(SK_FRZ_NERFED)) e.slowTimer = std::max(e.slowTimer, 1.0f);
    if (hasSkill(SK_FROST_FATE)) e.slowTimer = std::max(e.slowTimer, 1.5f);
    if (hasSkill(SK_FREEZE_HIT) && QRandomGenerator::global()->generateDouble() < 0.12 && e.type < ET_FinalBoss)
        e.slowTimer = std::max(e.slowTimer, 1.0f);
    if (hasSkill(SK_STICKY)) e.slowTimer = std::max(e.slowTimer, 1.0f);
    if (hasSkill(SK_POISON)) { e.poisonTimer = 3.0f; e.poisonDps = 4.f; }
    if (hasSkill(SK_BLAZE) && QRandomGenerator::global()->generateDouble() < 0.20) {
        e.burnTimer = 4.0f; e.burnDps = 3.f;
    }

    // SK_LIFESTEAL : on hit
    if (hasSkill(SK_LIFESTEAL))
        m_player.hp = std::min(m_player.maxHp, m_player.hp + dmg * 0.04f);

    const QColor *pal = (e.type==ET_Slime)?PAL_SLIME:
                        (e.type==ET_Mage||e.type==ET_FinalBoss)?PAL_FIRE:PAL_BONE;
    bool icy = hasSkill(SK_FRZ_NERFED) || hasSkill(SK_FREEZE_HIT) || hasSkill(SK_FROST_AURA);
    for (int i=0;i<5;++i) {
        Particle p; p.x = e.x + rndF(-8,8); p.y = e.y + rndF(-8,8);
        p.vx = rndF(-70,70); p.vy = rndF(-110,-20);
        p.color = icy ? PAL_ICE[rndI(0,1)] : pal[rndI(0,1)];
        p.life = p.maxLife = rndF(0.3f,0.6f); p.size = (int)rndF(2,5);
        m_particles.push_back(p);
    }

    if (e.hp <= 0) {
        e.dead = true; e.anim = AN_Death; e.animTimer = 0;

        // Killstreak / FrostNova counts
        m_player.killStreakCount++;
        m_player.killStreakTimer = 5.f;
        m_player.frostNovaKills++;

        // SK_VAMP_NERFED
        if (hasSkill(SK_VAMP_NERFED) && QRandomGenerator::global()->generateDouble() < 0.12)
            m_player.hp = std::min(m_player.maxHp, m_player.hp + 1.f);
        // SK_SOUL_DRAIN
        if (hasSkill(SK_SOUL_DRAIN) && m_player.regenAfterKillCd <= 0) {
            m_player.hp = std::min(m_player.maxHp, m_player.hp + 1.f);
            m_player.regenAfterKillCd = 5.f;
        }
        // SK_DEATH_MARK : marquer le suivant
        if (hasSkill(SK_DEATH_MARK)) {
            int closestId = -1; float md = 1e9f;
            for (auto &en : m_enemies) {
                if (en.dead || &en == &e) continue;
                float d = distF(en.x, en.y, e.x, e.y);
                if (d < md) { md = d; closestId = en.id; }
            }
            m_player.nextEnemyMarked = closestId;
        }
        // SK_NECRO : invoquer un esprit
        if (hasSkill(SK_NECRO) && QRandomGenerator::global()->generateDouble() < 0.12) {
            spawnMinion(e.x, e.y, 1, 80, ET_Skel);
            // mais c'est notre allie - simulons en lui faisant tirer rare et un alpha violet
            // (simplification : on le fait juste apparaitre comme petit skel ennemi affaibli)
        }
        // SK_EXPLODE_KILL
        if (hasSkill(SK_EXPLODE_KILL) && !fromExplosion) {
            float r = 45.f * (hasSkill(SK_BIG_BOOM) ? 1.35f : 1.f);
            for (auto &en : m_enemies) {
                if (en.dead) continue;
                if (distF(en.x, en.y, e.x, e.y) < r) hurtEnemy(en, 12, true);
            }
            for (int i=0;i<14;++i) {
                Particle p; p.x = e.x; p.y = e.y;
                float ang = rndF(0,6.28f), spd = rndF(60,160);
                p.vx = std::cos(ang)*spd; p.vy = std::sin(ang)*spd-40;
                p.color = (i%2==0)?PAL_FIRE[0]:PAL_FIRE[1];
                p.life=p.maxLife=rndF(0.3f,0.7f); p.size=4;
                m_particles.push_back(p);
            }
        }
        // SK_INFERNO : brules explosent
        if (e.burnTimer > 0 && hasSkill(SK_INFERNO)) {
            for (auto &en : m_enemies) {
                if (en.dead) continue;
                if (distF(en.x, en.y, e.x, e.y) < 50) hurtEnemy(en, 10, true);
            }
        }
        // SK_VOLCANO : 8% pluie de feu
        if (hasSkill(SK_VOLCANO) && QRandomGenerator::global()->generateDouble() < 0.08) {
            for (int i=0;i<6;++i) {
                Bullet b; b.x = e.x + rndF(-40,40); b.y = e.y + rndF(-50,-30);
                b.vx = 0; b.vy = 200; b.angle = M_PI/2; b.damage = 10; b.enemy = false;
                m_bullets.push_back(b);
            }
        }
        // SK_FROST_NOVA : tous les 5 kills
        if (hasSkill(SK_FROST_NOVA) && m_player.frostNovaKills >= 5) {
            m_player.frostNovaKills = 0;
            for (auto &en : m_enemies) if (!en.dead && en.type < ET_FinalBoss) en.slowTimer = std::max(en.slowTimer, 2.f);
            for (int i=0;i<24;++i) {
                Particle p; p.x = m_player.x; p.y = m_player.y;
                float ang = (i/24.f)*6.28f; p.vx = std::cos(ang)*180; p.vy = std::sin(ang)*180;
                p.color = PAL_ICE[rndI(0,1)]; p.life = p.maxLife = 0.8f; p.size = 4; p.noGravity = true;
                m_particles.push_back(p);
            }
        }

        // Death particles
        for (int i=0;i<12;++i) {
            float ang = (i/12.f)*6.28f;
            Particle p; p.x = e.x; p.y = e.y;
            p.vx = std::cos(ang)*rndF(50,130); p.vy = std::sin(ang)*rndF(50,130);
            p.color = pal[rndI(0,1)]; p.life = p.maxLife = rndF(0.4f,0.8f);
            p.size = (int)rndF(3,6);
            m_particles.push_back(p);
        }
    } else {
        e.anim = AN_Hurt; e.animTimer = 0;
    }
}


// ============================================================
//  ENEMY UPDATE - boss patterns uniques
// ============================================================
void GameWidget::updateEnemies(float dt)
{
    // Snapshot of enemies size to avoid issues with summoning during iteration
    int origSize = (int)m_enemies.size();
    for (int idx = 0; idx < origSize; ++idx) {
        Enemy &e = m_enemies[idx];
        if (e.hitFlash > 0) e.hitFlash -= dt;
        if (e.slowTimer > 0) e.slowTimer -= dt;
        if (e.dead) { e.animTimer += dt; continue; }

        float adt = dt * (e.slowTimer > 0 ? 0.5f : 1.f);
        e.animTimer += adt;

        // Poison/burn dot
        if (e.poisonTimer > 0) {
            e.poisonTimer -= dt;
            e.hp -= e.poisonDps * dt;
            if (e.hp <= 0) { hurtEnemy(e, 0.01f); continue; }
        }
        if (e.burnTimer > 0) {
            e.burnTimer -= dt;
            e.hp -= e.burnDps * dt;
            if (e.hp <= 0) { hurtEnemy(e, 0.01f); continue; }
        }

        float dx = m_player.x - e.x, dy = m_player.y - e.y;
        float d = std::hypot(dx, dy);
        float collR = e.size * 0.4f;

        // ─── ENNEMIS NORMAUX ───
        if (e.type == ET_Slime) {
            if (d > 0) {
                tryMove(e.x, e.y, dx/d * e.speed * adt, dy/d * e.speed * adt, collR);
                e.facingLeft = dx < 0;
            }
            if (e.anim==AN_Hurt && e.animTimer>0.4f) { e.anim=AN_Walk; e.animTimer=0; }
            else if (e.anim!=AN_Hurt) e.anim = AN_Walk;
        } else if (e.type == ET_Skel || e.type == ET_Minion) {
            e.aimAngle = std::atan2(dy, dx); e.facingLeft = dx<0;
            if (d > 0) {
                float ds = 0;
                if (d < 150) ds = -0.4f; else if (d > 220) ds = 1.f;
                if (ds != 0) tryMove(e.x, e.y, dx/d*e.speed*ds*adt, dy/d*e.speed*ds*adt, collR);
            }
            e.shootCd -= adt;
            if (e.shootCd <= 0 && d < 320 && e.type != ET_Minion) {
                e.shootCd = rndF(1.8f, 2.8f);
                Bullet b; b.x = e.x; b.y = e.y;
                b.vx = std::cos(e.aimAngle)*175; b.vy = std::sin(e.aimAngle)*175;
                b.angle = e.aimAngle; b.damage = 1; b.enemy = true;
                m_bullets.push_back(b);
            }
            if (e.anim==AN_Hurt && e.animTimer>0.4f) { e.anim=AN_Walk; e.animTimer=0; }
            else if (e.anim!=AN_Hurt) e.anim = (d>5)?AN_Walk:AN_Idle;
        } else if (e.type == ET_Bat) {
            e.zigTimer -= adt;
            if (e.zigTimer <= 0) { e.zigDir *= -1; e.zigTimer = rndF(0.3f, 0.7f); }
            if (d > 0) {
                float px = -dy/d, py = dx/d;
                tryMove(e.x, e.y, (dx/d*e.speed+px*35*e.zigDir)*adt,
                                  (dy/d*e.speed+py*35*e.zigDir)*adt, collR);
                e.facingLeft = dx < 0;
            }
            if (e.anim==AN_Hurt && e.animTimer>0.4f) { e.anim=AN_Walk; e.animTimer=0; }
            else if (e.anim!=AN_Hurt) e.anim = AN_Walk;
        } else if (e.type == ET_Brute) {
            if (d > 0) {
                tryMove(e.x, e.y, dx/d*e.speed*adt, dy/d*e.speed*adt, collR);
                e.facingLeft = dx < 0;
            }
            if (e.anim==AN_Hurt && e.animTimer>0.4f) { e.anim=AN_Walk; e.animTimer=0; }
            else if (e.anim!=AN_Hurt) e.anim = AN_Walk;
        } else if (e.type == ET_Mage) {
            e.aimAngle = std::atan2(dy, dx); e.facingLeft = dx<0;
            if (d > 0) {
                float ds = 0;
                if (d < 200) ds = -0.5f; else if (d > 280) ds = 0.7f;
                if (ds != 0) tryMove(e.x, e.y, dx/d*e.speed*ds*adt, dy/d*e.speed*ds*adt, collR);
            }
            e.shootCd -= adt;
            if (e.shootCd <= 0 && d < 380) {
                e.shootCd = rndF(2.0f, 3.0f);
                for (float off : {-0.3f, 0.f, 0.3f}) {
                    Bullet b; b.x = e.x; b.y = e.y;
                    float a = e.aimAngle + off;
                    b.vx = std::cos(a)*160; b.vy = std::sin(a)*160;
                    b.angle = a; b.damage = 1; b.enemy = true;
                    m_bullets.push_back(b);
                }
            }
            if (e.anim==AN_Hurt && e.animTimer>0.4f) { e.anim=AN_Walk; e.animTimer=0; }
            else if (e.anim!=AN_Hurt) e.anim = (d>5)?AN_Walk:AN_Idle;
        }

        // ─── BOSSES ───
        else if (e.type == ET_MiniBoss || e.type == ET_FinalBoss) {
            // Phase progression
            if (e.type == ET_FinalBoss) {
                if (e.hp < e.maxHp*0.75f && e.phase==1) e.phase=2;
                if (e.hp < e.maxHp*0.50f && e.phase==2) e.phase=3;
                if (e.hp < e.maxHp*0.25f && e.phase==3) e.phase=4;
            } else {
                if (e.hp < e.maxHp*0.5f && e.phase==1) e.phase=2;
            }

            // Random move target
            e.moveCd -= adt;
            if (e.moveCd <= 0 && !e.charging) {
                e.moveCd = rndF(0.7f, 1.5f);
                float pad = TILE * 2.5f;
                e.moveTargetX = rndF(pad, GW-pad);
                e.moveTargetY = rndF(pad, GH-pad);
            }

            // Subtype-specific behavior
            if (e.subType == 0) {
                // ─── VER DE FEU : trail + occasional charge ───
                if (e.charging) {
                    e.x += e.chargeVx * adt;
                    e.y += e.chargeVy * adt;
                    e.chargeT -= adt;
                    if (e.chargeT <= 0) e.charging = false;
                    if (e.x < TILE*1.5f || e.x > GW-TILE*1.5f) e.charging=false;
                    if (e.y < TILE*1.5f || e.y > GH-TILE*1.5f) e.charging=false;
                } else {
                    float mdx = e.moveTargetX-e.x, mdy = e.moveTargetY-e.y, md = std::hypot(mdx,mdy);
                    if (md > 5) tryMove(e.x, e.y, mdx/md*e.speed*adt, mdy/md*e.speed*adt, collR);
                    e.facingLeft = mdx < 0;
                }
                e.chargeCd -= adt;
                if (e.chargeCd <= 0 && d < 350 && !e.charging) {
                    e.chargeCd = (e.phase==2) ? 3.f : 5.f;
                    e.charging = true; e.chargeT = 1.0f;
                    float ca = std::atan2(dy,dx);
                    float cspd = (e.phase==2)?280.f:220.f;
                    e.chargeVx = std::cos(ca)*cspd; e.chargeVy = std::sin(ca)*cspd;
                }
                // Trail de feu
                e.trailCd -= adt;
                if (e.trailCd <= 0) {
                    e.trailCd = (e.phase==2)?0.4f:0.7f;
                    Bullet pool; pool.x = e.x; pool.y = e.y; pool.vx=0; pool.vy=0;
                    pool.angle = 0; pool.damage = 0;
                    pool.enemy = true; pool.grenade = true;
                    pool.grenadeFuse = (e.phase==2)?3.f:2.5f; pool.grenadeTotal = pool.grenadeFuse;
                    m_bullets.push_back(pool);
                }
                e.shootCd -= adt;
                if (e.shootCd <= 0) {
                    e.shootCd = (e.phase==2)?1.0f:1.4f;
                    float a = std::atan2(dy,dx);
                    for (float off : {-0.2f, 0.f, 0.2f}) {
                        Bullet b; b.x = e.x; b.y = e.y;
                        b.vx = std::cos(a+off)*180; b.vy = std::sin(a+off)*180;
                        b.angle = a+off; b.damage = e.damage; b.enemy = true;
                        m_bullets.push_back(b);
                    }
                }
            } else if (e.subType == 1) {
                // ─── EXECUTEUR DES MORTS : invoque sbires ───
                float mdx = e.moveTargetX-e.x, mdy = e.moveTargetY-e.y, md = std::hypot(mdx,mdy);
                if (md > 5) tryMove(e.x, e.y, mdx/md*e.speed*adt, mdy/md*e.speed*adt, collR);
                e.facingLeft = mdx < 0;
                e.summonCd -= adt;
                if (e.summonCd <= 0) {
                    e.summonCd = (e.phase==2) ? 4.f : 7.f;
                    int n = (e.phase==2) ? 4 : 2;
                    for (int i=0;i<n;++i) {
                        float ang = (i/float(n))*6.28f + rndF(-0.2f, 0.2f);
                        float sx = e.x + std::cos(ang)*40, sy = e.y + std::sin(ang)*40;
                        spawnMinion(sx, sy, 25, 70, ET_Skel);
                    }
                    for (int i=0;i<16;++i) {
                        Particle p; p.x = e.x; p.y = e.y;
                        float a = (i/16.f)*6.28f;
                        p.vx = std::cos(a)*100; p.vy = std::sin(a)*100;
                        p.color = QColor("#aabbcc"); p.life=p.maxLife=0.7f; p.size=4; p.noGravity=true;
                        m_particles.push_back(p);
                    }
                }
                e.shootCd -= adt;
                if (e.shootCd <= 0) {
                    e.shootCd = (e.phase==2)?0.9f:1.3f;
                    float a = std::atan2(dy,dx);
                    Bullet b; b.x = e.x; b.y = e.y;
                    b.vx = std::cos(a)*180; b.vy = std::sin(a)*180;
                    b.angle = a; b.damage = e.damage; b.enemy = true;
                    m_bullets.push_back(b);
                }
                e.burstCd -= adt;
                if (e.burstCd <= 0) {
                    e.burstCd = (e.phase==2)?4.f:6.f;
                    for (int i=0;i<10;++i) {
                        float a = (i/10.f)*6.28f;
                        Bullet b; b.x = e.x; b.y = e.y;
                        b.vx = std::cos(a)*120; b.vy = std::sin(a)*120;
                        b.angle = a; b.damage = 1; b.enemy = true;
                        m_bullets.push_back(b);
                    }
                }
            } else if (e.subType == 2) {
                // ─── DEMON VASE : teleport ───
                e.specialCd -= adt;
                if (e.specialCd <= 0) {
                    e.specialCd = (e.phase==2) ? 2.5f : 4.f;
                    // Particles a l'ancien emplacement
                    for (int i=0;i<14;++i) {
                        Particle p; p.x = e.x; p.y = e.y;
                        float a = rndF(0,6.28f); p.vx = std::cos(a)*120; p.vy = std::sin(a)*120;
                        p.color = QColor("#aa44ff"); p.life=p.maxLife=0.5f; p.size=4; p.noGravity=true;
                        m_particles.push_back(p);
                    }
                    e.x = m_player.x + rndF(-160,160); e.y = m_player.y + rndF(-100,100);
                    e.x = clampF(e.x, TILE*2, GW-TILE*2); e.y = clampF(e.y, TILE*2, GH-TILE*2);
                    for (int i=0;i<14;++i) {
                        Particle p; p.x = e.x; p.y = e.y;
                        float a = rndF(0,6.28f); p.vx = std::cos(a)*60; p.vy = std::sin(a)*60;
                        p.color = QColor("#dd66ff"); p.life=p.maxLife=0.4f; p.size=3; p.noGravity=true;
                        m_particles.push_back(p);
                    }
                }
                e.shootCd -= adt;
                if (e.shootCd <= 0) {
                    e.shootCd = (e.phase==2)?0.7f:1.f;
                    float a = std::atan2(dy,dx);
                    int n = (e.phase==2)?5:3;
                    for (int i=0;i<n;++i) {
                        float off = (i-(n-1)/2.f) * 0.20f;
                        Bullet b; b.x = e.x; b.y = e.y;
                        b.vx = std::cos(a+off)*200; b.vy = std::sin(a+off)*200;
                        b.angle = a+off; b.damage = e.damage; b.enemy = true;
                        m_bullets.push_back(b);
                    }
                }
                if (e.phase == 2) {
                    e.burstCd -= adt;
                    if (e.burstCd <= 0) {
                        e.burstCd = 4.f;
                        // Spawn 2 minions
                        spawnMinion(e.x+30, e.y, 20, 80, ET_Slime);
                        spawnMinion(e.x-30, e.y, 20, 80, ET_Slime);
                    }
                }
            } else if (e.subType == 3) {
                // ─── MINOTAURE : charges puissantes en croix ───
                if (e.charging) {
                    e.x += e.chargeVx * adt;
                    e.y += e.chargeVy * adt;
                    e.chargeT -= adt;
                    if (e.chargeT <= 0 || e.x < TILE*1.5f || e.x > GW-TILE*1.5f
                        || e.y < TILE*1.5f || e.y > GH-TILE*1.5f) {
                        e.charging = false;
                        // Boom on stop
                        for (int i=0;i<16;++i) {
                            Particle p; p.x = e.x; p.y = e.y;
                            float a = (i/16.f)*6.28f; p.vx=std::cos(a)*200; p.vy=std::sin(a)*200;
                            p.color = QColor("#cc8822"); p.life=p.maxLife=0.6f; p.size=5;
                            m_particles.push_back(p);
                        }
                    }
                } else {
                    float mdx = e.moveTargetX-e.x, mdy = e.moveTargetY-e.y, md = std::hypot(mdx,mdy);
                    if (md > 5) tryMove(e.x, e.y, mdx/md*e.speed*adt, mdy/md*e.speed*adt, collR);
                    e.facingLeft = mdx < 0;
                }
                e.chargeCd -= adt;
                if (e.chargeCd <= 0 && !e.charging) {
                    e.chargeCd = (e.phase==2) ? 1.8f : 3.f;
                    e.charging = true; e.chargeT = (e.phase==2) ? 1.0f : 1.2f;
                    float ca = std::atan2(dy,dx);
                    float cspd = (e.phase==2) ? 480.f : 360.f;
                    e.chargeVx = std::cos(ca)*cspd; e.chargeVy = std::sin(ca)*cspd;
                    e.facingLeft = std::cos(ca) < 0;
                }
                if (e.phase == 2) {
                    e.burstCd -= adt;
                    if (e.burstCd <= 0) {
                        e.burstCd = 5.f;
                        // Onde de choc en croix
                        for (int dir=0; dir<4; ++dir) {
                            float a = dir * (M_PI/2);
                            for (int i=0;i<6;++i) {
                                Bullet b; b.x = e.x; b.y = e.y;
                                b.vx = std::cos(a)*(120+i*40); b.vy = std::sin(a)*(120+i*40);
                                b.angle = a; b.damage = 1; b.enemy = true;
                                m_bullets.push_back(b);
                            }
                        }
                    }
                }
            } else if (e.subType == 4) {
                // ─── GARDIEN DU GIVRE : EXTREME, 4 phases ───
                // Mouvement : teleport en phase >=2
                if (e.phase >= 2) {
                    e.specialCd -= adt;
                    if (e.specialCd <= 0) {
                        e.specialCd = (e.phase==4) ? 1.5f : (e.phase==3 ? 2.f : 3.f);
                        for (int i=0;i<14;++i) {
                            Particle p; p.x=e.x; p.y=e.y;
                            float a=rndF(0,6.28f); p.vx=std::cos(a)*120; p.vy=std::sin(a)*120;
                            p.color = PAL_ICE[rndI(0,1)]; p.life=p.maxLife=0.5f; p.size=4; p.noGravity=true;
                            m_particles.push_back(p);
                        }
                        e.x = clampF(m_player.x+rndF(-180,180), TILE*2, GW-TILE*2);
                        e.y = clampF(m_player.y+rndF(-110,110), TILE*2, GH-TILE*2);
                        for (int i=0;i<14;++i) {
                            Particle p; p.x=e.x; p.y=e.y;
                            float a=rndF(0,6.28f); p.vx=std::cos(a)*60; p.vy=std::sin(a)*60;
                            p.color = PAL_ICE[rndI(0,1)]; p.life=p.maxLife=0.3f; p.size=3; p.noGravity=true;
                            m_particles.push_back(p);
                        }
                    }
                } else {
                    float mdx = e.moveTargetX-e.x, mdy = e.moveTargetY-e.y, md = std::hypot(mdx,mdy);
                    if (md > 5) tryMove(e.x, e.y, mdx/md*e.speed*adt, mdy/md*e.speed*adt, collR);
                    e.facingLeft = mdx < 0;
                }

                // Tirs continus - plus rapides a chaque phase
                e.shootCd -= adt;
                if (e.shootCd <= 0) {
                    e.shootCd = (e.phase==4)?0.30f : (e.phase==3?0.45f : (e.phase==2?0.6f : 0.8f));
                    float a = std::atan2(dy, dx);
                    int n = e.phase + 1;
                    for (int i=0;i<n;++i) {
                        float off = (n>1) ? (i-(n-1)/2.f)*0.18f : 0;
                        Bullet b; b.x = e.x; b.y = e.y;
                        b.vx = std::cos(a+off)*220; b.vy = std::sin(a+off)*220;
                        b.angle = a+off; b.damage = e.damage; b.enemy = true;
                        m_bullets.push_back(b);
                    }
                }

                // Onde radiale phase 3+
                if (e.phase >= 3) {
                    e.burstCd -= adt;
                    if (e.burstCd <= 0) {
                        e.burstCd = (e.phase==4)?2.0f:3.0f;
                        int rays = (e.phase==4)?20:14;
                        float baseAng = m_globalTime * 0.7f;
                        for (int i=0;i<rays;++i) {
                            float a = baseAng + (i/float(rays))*6.28f;
                            Bullet b; b.x = e.x; b.y = e.y;
                            b.vx = std::cos(a)*180; b.vy = std::sin(a)*180;
                            b.angle = a; b.damage = 1; b.enemy = true;
                            m_bullets.push_back(b);
                        }
                    }
                }
                // Invocations phase >= 2
                if (e.phase >= 2) {
                    e.summonCd -= adt;
                    if (e.summonCd <= 0) {
                        e.summonCd = (e.phase==4)?4.f : (e.phase==3?5.f : 7.f);
                        int n = (e.phase==4) ? 3 : 2;
                        for (int i=0;i<n;++i) {
                            float ang = (i/float(n))*6.28f + rndF(-0.3f,0.3f);
                            float sx = e.x + std::cos(ang)*60, sy = e.y + std::sin(ang)*60;
                            sx = clampF(sx, TILE*2, GW-TILE*2);
                            sy = clampF(sy, TILE*2, GH-TILE*2);
                            spawnMinion(sx, sy, 30, 100, ET_Bat);
                        }
                    }
                }
                // Charge phase 4
                if (e.phase == 4) {
                    e.chargeCd -= adt;
                    if (e.chargeCd <= 0 && !e.charging) {
                        e.chargeCd = 3.5f;
                        e.charging = true; e.chargeT = 0.8f;
                        float ca = std::atan2(dy,dx);
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


// ============================================================
//  BULLETS / PARTICLES / DOORS / SKILLS
// ============================================================
void GameWidget::updateBullets(float dt)
{
    for (size_t bi = 0; bi < m_bullets.size(); ++bi) {
        Bullet &b = m_bullets[bi];
        if (b.dead) continue;

        if (b.grenade) {
            float drag = 0.985f; b.vx *= drag; b.vy *= drag;
            b.x += b.vx*dt; b.y += b.vy*dt; b.grenadeFuse -= dt;
            if (b.x < TILE*1.2f || b.x > GW-TILE*1.2f) b.vx *= -0.6f;
            if (b.y < TILE*1.2f || b.y > GH-TILE*1.2f) b.vy *= -0.6f;
            b.x = clampF(b.x, TILE*1.2f, GW-TILE*1.2f);
            b.y = clampF(b.y, TILE*1.2f, GH-TILE*1.2f);

            // Trail boss = grenade ennemi (continuous flame pool)
            if (b.enemy && b.grenadeTotal > 1.5f) {
                // Damage player if standing in pool
                if (distF(b.x, b.y, m_player.x, m_player.y) < 30 && m_player.invincibility <= 0)
                    if ((int)(m_globalTime*4) != (int)((m_globalTime-dt)*4)) hurtPlayer(1);
                if (b.grenadeFuse <= 0) b.dead = true;
                continue;
            }

            if (b.grenadeFuse <= 0) {
                float exX = b.x, exY = b.y;
                float rad = 70.f * (hasSkill(SK_BIG_BOOM) ? 1.35f : 1.f);
                float dmg = 50.f;
                for (auto &e : m_enemies)
                    if (!e.dead && distF(e.x, e.y, exX, exY) < rad + e.size*0.3f)
                        hurtEnemy(e, dmg, true);
                for (int i=0; i<32; ++i) {
                    float ang = (i/32.f)*6.28f + rndF(-0.1f, 0.1f), spd = rndF(80, 250);
                    Particle p; p.x = exX; p.y = exY;
                    p.vx = std::cos(ang)*spd; p.vy = std::sin(ang)*spd-60;
                    p.color = (i%2==0)?PAL_FIRE[0]:PAL_FIRE[1];
                    p.life = p.maxLife = rndF(0.4f, 0.9f); p.size = (int)rndF(4, 8);
                    m_particles.push_back(p);
                }
                b.dead = true;
            }
            continue;
        }

        // SK_HOMING
        if (b.homing && !b.enemy) {
            Enemy *near=nullptr; float nd=1e9f;
            for (auto &e : m_enemies) {
                if (e.dead) continue;
                float d = distF(b.x, b.y, e.x, e.y);
                if (d < nd && d < 200) { nd = d; near = &e; }
            }
            if (near) {
                float ta = std::atan2(near->y - b.y, near->x - b.x);
                float ca = b.angle;
                float diff = ta - ca;
                while (diff > M_PI) diff -= 2*M_PI;
                while (diff < -M_PI) diff += 2*M_PI;
                float turn = std::min(std::abs(diff), 3.5f * dt);
                b.angle += (diff > 0) ? turn : -turn;
                float speed = std::hypot(b.vx, b.vy);
                b.vx = std::cos(b.angle)*speed; b.vy = std::sin(b.angle)*speed;
            }
        }

        b.x += b.vx*dt; b.y += b.vy*dt;
        float m = TILE * 1.0f;
        if (!b.ghost) {
            if (b.x < m || b.x > GW-m) {
                if (b.bounce > 0) { b.vx *= -1; b.bounce--; b.angle = std::atan2(b.vy, b.vx); }
                else b.dead = true;
            }
            if (b.y < m || b.y > GH-m) {
                if (b.bounce > 0) { b.vy *= -1; b.bounce--; b.angle = std::atan2(b.vy, b.vx); }
                else b.dead = true;
            }
        } else {
            if (b.x < -50 || b.x > GW+50 || b.y < -50 || b.y > GH+50) b.dead = true;
        }
        if (!b.dead && !b.ghost && collidesObstacle(b.x, b.y, 4)) b.dead = true;

        if (!b.dead && !b.enemy) {
            for (auto &e : m_enemies) {
                if (e.dead || b.hitIds.contains(e.id)) continue;
                if (distF(b.x, b.y, e.x, e.y) < 8 + e.size*0.4f) {
                    hurtEnemy(e, b.damage);
                    // SK_KNOCKBACK
                    if (hasSkill(SK_KNOCKBACK) && !e.dead) {
                        float ka = b.angle;
                        e.x += std::cos(ka)*8; e.y += std::sin(ka)*8;
                    }
                    // SK_ICE_SHARDS : 20% chance fragment
                    if (hasSkill(SK_ICE_SHARDS) && !b.splitChild
                        && QRandomGenerator::global()->generateDouble() < 0.20) {
                        for (int k=0;k<4;++k) {
                            float a = (k/4.f)*6.28f;
                            Bullet sb; sb.x = b.x; sb.y = b.y;
                            sb.vx = std::cos(a)*240; sb.vy = std::sin(a)*240;
                            sb.angle = a; sb.damage = b.damage*0.4f; sb.enemy = false;
                            sb.splitChild = true; sb.icy = true; sb.hitIds.insert(e.id);
                            m_bullets.push_back(sb);
                        }
                    }
                    // SK_SPLT_NERFED
                    if (hasSkill(SK_SPLT_NERFED) && !b.splitChild) {
                        float perp = b.angle + M_PI/2;
                        for (int sgn=-1; sgn<=1; sgn+=2) {
                            Bullet sb; sb.x = b.x; sb.y = b.y;
                            sb.vx = std::cos(perp)*sgn*240; sb.vy = std::sin(perp)*sgn*240;
                            sb.angle = perp + (sgn<0?M_PI:0); sb.damage = b.damage*0.35f;
                            sb.splitChild = true; sb.enemy = false; sb.hitIds.insert(e.id);
                            m_bullets.push_back(sb);
                        }
                    }
                    if (!b.pierce || b.pierceLeft <= 0) { b.dead = true; }
                    else { b.pierceLeft--; b.hitIds.insert(e.id); }
                    break;
                }
            }
        }
    }
    m_bullets.erase(std::remove_if(m_bullets.begin(), m_bullets.end(),
                    [](const Bullet &b){ return b.dead; }), m_bullets.end());
}

void GameWidget::updateParticles(float dt)
{
    for (auto &p : m_particles) {
        p.x += p.vx*dt; p.y += p.vy*dt;
        if (!p.noGravity) p.vy += 180*dt;
        p.life -= dt;
    }
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(),
                      [](const Particle &p){ return p.life <= 0; }), m_particles.end());
}

void GameWidget::checkDoorTransition()
{
    int door = -1;
    if (m_doorOpen[0] && m_player.y < TILE*1.0f
        && std::abs(m_player.x - DOOR_COL*TILE - TILE/2.f) < TILE*0.7f) door = 0;
    else if (m_doorOpen[1] && m_player.x > GW-TILE*1.0f
        && std::abs(m_player.y - DOOR_ROW*TILE - TILE/2.f) < TILE*0.7f) door = 1;
    else if (m_doorOpen[2] && m_player.y > GH-TILE*1.0f
        && std::abs(m_player.x - DOOR_COL*TILE - TILE/2.f) < TILE*0.7f) door = 2;
    else if (m_doorOpen[3] && m_player.x < TILE*1.0f
        && std::abs(m_player.y - DOOR_ROW*TILE - TILE/2.f) < TILE*0.7f) door = 3;
    if (door >= 0) { m_exitDoor = door; m_state = GS_FadeOut; m_fadeAmount = 0; }
}

void GameWidget::generateSkills()
{
    std::vector<int> avail;
    for (auto &s : m_allSkills) {
        if (s.worldTier > m_worldsUnlocked) continue;   // pas encore debloque
        bool has = hasSkill(s.id);
        // Skills cumulables
        if (s.id == SK_GRN || s.id == SK_MAXHP_NERFED || s.id == SK_HARDCORE ||
            s.id == SK_TANK || s.id == SK_BLOOD_PACT || s.id == SK_LEGEND) {
            avail.push_back(s.id); continue;
        }
        if (!has) avail.push_back(s.id);
    }
    for (int i = (int)avail.size()-1; i > 0; --i) {
        int j = rndI(0, i); std::swap(avail[i], avail[j]);
    }
    m_skillChoices.clear();
    for (int i = 0; i < std::min((int)avail.size(), 3); ++i)
        m_skillChoices.push_back(avail[i]);
}

void GameWidget::applySkill(int skillId)
{
    if (skillId == SK_HARDCORE)      { m_player.maxHp += 1; m_player.hp += 1; }
    else if (skillId == SK_MAXHP_NERFED) { m_player.maxHp += 1; m_player.hp = m_player.maxHp; }
    else if (skillId == SK_TANK)     { m_player.maxHp += 3; m_player.hp = std::min(m_player.maxHp, m_player.hp+3); }
    else if (skillId == SK_BLOOD_PACT){ m_player.maxHp = std::max(1.f, m_player.maxHp - 1); m_player.hp = std::min(m_player.maxHp, m_player.hp); }
    else if (skillId == SK_GRN)      m_player.grenadeAmmo += 3;

    m_player.skills.push_back(skillId);
    // +1 PV regen entre salles
    m_player.hp = std::min(m_player.maxHp, m_player.hp + 1.f);

    int entryDoor = (m_exitDoor + 2) % 4;
    buildRoom(m_currentRoom + 1, entryDoor);
    m_state = GS_FadeIn;
    m_fadeAmount = 1.f;
}


// ============================================================
//  INPUT
// ============================================================
void GameWidget::keyPressEvent(QKeyEvent *event)
{
    int k = event->key();
    m_keys.insert(k);
    if (m_state == GS_Menu) {
        if (k==Qt::Key_Space || k==Qt::Key_Return) startGame();
        else if (k==Qt::Key_C) m_state = GS_SkinSelect;
        else if (k==Qt::Key_D) { m_state = GS_Discoveries; m_discoveryHover = 0; }
    } else if (m_state == GS_SkinSelect) {
        if (k==Qt::Key_Left || k==Qt::Key_A) m_menuSelectedSkin = (m_menuSelectedSkin+3)%4;
        else if (k==Qt::Key_Right || k==Qt::Key_D) m_menuSelectedSkin = (m_menuSelectedSkin+1)%4;
        else if (k==Qt::Key_Up || k==Qt::Key_W) m_menuSelectedAtk = (m_menuSelectedAtk+2)%3;
        else if (k==Qt::Key_Down || k==Qt::Key_S) m_menuSelectedAtk = (m_menuSelectedAtk+1)%3;
        else if (k==Qt::Key_Escape || k==Qt::Key_Return || k==Qt::Key_Space) {
            QSettings s("RetroArcher","RetroArcher");
            s.setValue("skin", m_menuSelectedSkin); s.setValue("atk", m_menuSelectedAtk);
            m_state = GS_Menu;
        }
    } else if (m_state == GS_Discoveries) {
        if (k==Qt::Key_Escape || k==Qt::Key_Return) m_state = GS_Menu;
        else if (k==Qt::Key_Left || k==Qt::Key_A) m_discoveryHover = (m_discoveryHover+4)%5;
        else if (k==Qt::Key_Right || k==Qt::Key_D) m_discoveryHover = (m_discoveryHover+1)%5;
    } else if (m_state == GS_GameOver || m_state == GS_Victory) {
        if (k==Qt::Key_Space) startGame();
        else if (k==Qt::Key_Escape) m_state = GS_Menu;
    } else if (m_state == GS_SkillSelect) {
        int idx = -1;
        if (k==Qt::Key_1) idx = 0; else if (k==Qt::Key_2) idx = 1; else if (k==Qt::Key_3) idx = 2;
        if (idx >= 0 && idx < (int)m_skillChoices.size()) applySkill(m_skillChoices[idx]);
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
    p.scale(DISPLAY_SCALE, DISPLAY_SCALE);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.setRenderHint(QPainter::Antialiasing, false);
    renderGame(p);
}

void GameWidget::drawSpriteAt(QPainter &p, const QImage &sheet, int frameCount, float fps,
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

void GameWidget::drawBossInGame(QPainter &p, const Enemy &boss, float scaleMul)
{
    int w = boss.subType;
    if (w < 0 || w >= m_worlds.size()) return;
    const WorldInfo &wi = m_worlds[w];
    QImage sheet = m_bossSheets[wi.bossKey];
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

void GameWidget::renderGame(QPainter &p)
{
    p.fillRect(0, 0, CW, CH, Qt::black);
    if (m_state == GS_Menu) { drawMenu(p); return; }
    if (m_state == GS_SkinSelect) { drawSkinSelect(p); return; }
    if (m_state == GS_Discoveries) { drawDiscoveries(p); return; }
    drawRoom(p);

    // Particles
    for (auto &pa : m_particles) {
        float a = std::max(0.f, pa.life/pa.maxLife);
        QColor c = pa.color; c.setAlphaF(a);
        p.fillRect(QRectF(pa.x - pa.size/2.f, pa.y - pa.size/2.f, pa.size, pa.size), c);
    }

    // Enemies
    for (auto &e : m_enemies) {
        // Bosses use real sprites
        if (e.type == ET_MiniBoss || e.type == ET_FinalBoss) {
            drawBossInGame(p, e, 1.f);
            // Hit flash via overlay
            if (e.hitFlash > 0 && !e.dead) {
                p.save();
                p.setCompositionMode(QPainter::CompositionMode_Plus);
                p.setOpacity(std::min(0.7f, e.hitFlash * 4));
                drawBossInGame(p, e, 1.f);
                p.restore();
            }
            // HP bar
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
        const QImage &sheet = m_sprOrc[variant][anim];
        float scale = 1.f;
        if      (e.type==ET_Slime)  scale = 1.05f;
        else if (e.type==ET_Skel)   scale = 1.10f;
        else if (e.type==ET_Bat)    scale = 0.95f;
        else if (e.type==ET_Brute)  scale = 1.30f;
        else if (e.type==ET_Mage)   scale = 1.10f;
        else if (e.type==ET_Minion) scale = 0.85f;
        int fc = 6; float fps = 8.f;
        switch (anim) {
            case AN_Idle:  fc = s_idleFrameCount; fps = 8.f; break;
            case AN_Walk:  fc = s_walkFrameCount; fps = 12.f; break;
            case AN_Atk:   fc = 6; fps = 14.f; break;
            case AN_Hurt:  fc = s_hurtFrameCount; fps = 10.f; break;
            case AN_Death: fc = s_deathFrameCount; fps = 7.f; break;
        }
        float fadeAlpha = 1.f;
        if (e.dead && e.corpseFadeTimer > 0)
            fadeAlpha = std::max(0.f, 1.f - e.corpseFadeTimer/0.6f);
        if (fadeAlpha < 1.f) p.setOpacity(fadeAlpha);
        drawSpriteAt(p, sheet, fc, fps, e.x, e.y, scale, e.facingLeft, e.animTimer);
        if (fadeAlpha < 1.f) p.setOpacity(1.f);
        // Frozen indicator
        if (e.slowTimer > 0 && !e.dead) {
            for (int i=0; i<3; ++i) {
                float ph = m_globalTime*2 + i*2.f;
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

    // Bullets
    for (auto &b : m_bullets) {
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
            float w = m_sprArrow.width(), h = m_sprArrow.height();
            if (b.icy) { p.setOpacity(0.8); }
            p.drawImage(QRectF(-w/2, -h/2, w, h), m_sprArrow);
            p.setOpacity(1);
            p.restore();
            // Trail color by type
            if (b.poison || b.burn || b.icy) {
                QColor tc = b.poison ? PAL_POISON[0] : (b.burn ? PAL_FIRE[0] : PAL_ICE[0]);
                tc.setAlphaF(0.6f);
                p.fillRect(QRectF(b.x-2, b.y-2, 4, 4), tc);
            }
        }
    }

    // Player
    if (!(m_player.invincibility > 0 && (int)(m_player.invincibility*28) % 2 == 0)) {
        const QImage &sheet = soldierSheet(m_player.anim);
        drawSpriteAt(p, sheet, soldierFrameCount(m_player.anim), soldierFps(m_player.anim),
                     m_player.x, m_player.y, 1.3f, m_player.facingLeft, m_player.animTimer);
        // Dash trail
        if (m_player.dashActive > 0) {
            p.setOpacity(0.4f);
            drawSpriteAt(p, sheet, soldierFrameCount(m_player.anim), soldierFps(m_player.anim),
                         m_player.x - std::cos(m_player.facing)*8,
                         m_player.y - std::sin(m_player.facing)*8,
                         1.3f, m_player.facingLeft, m_player.animTimer);
            p.setOpacity(1);
        }
    }

    drawHUD(p);

    if (m_fadeAmount > 0) {
        QColor c(0,0,0); c.setAlphaF(std::min(1.f, m_fadeAmount));
        p.fillRect(0, 0, CW, CH, c);
    }
    if (m_state == GS_SkillSelect) drawSkillSelect(p);
    if (m_state == GS_GameOver) drawEndScreen(p, false);
    if (m_state == GS_Victory)  drawEndScreen(p, true);
}

void GameWidget::drawRoom(QPainter &p)
{
    for (int r=1; r<ROWS-1; ++r) for (int c=1; c<COLS-1; ++c) {
        int x = c*TILE, y = r*TILE;
        p.fillRect(x, y, TILE, TILE, ((r+c)%2==0)?C_FL1:C_FL2);
    }
    for (int c=0; c<COLS; ++c) {
        p.fillRect(c*TILE, 0, TILE, TILE, C_WT); p.fillRect(c*TILE, TILE-8, TILE, 8, C_WF);
        p.fillRect(c*TILE, (ROWS-1)*TILE, TILE, TILE, C_WT); p.fillRect(c*TILE, (ROWS-1)*TILE, TILE, 8, C_WF);
    }
    for (int r=0; r<ROWS; ++r) {
        p.fillRect(0, r*TILE, TILE, TILE, C_WT); p.fillRect(TILE-8, r*TILE, 8, TILE, C_WF);
        p.fillRect((COLS-1)*TILE, r*TILE, TILE, TILE, C_WT); p.fillRect((COLS-1)*TILE, r*TILE, 8, TILE, C_WF);
    }
    auto drawDoor = [&](int col, int row, bool open) {
        int x = col*TILE, y = row*TILE;
        p.fillRect(x+4, y+4, TILE-8, TILE-8, open?C_DO:C_DC);
        if (open) {
            p.fillRect(x+6, y+6, TILE-12, TILE-12, QColor(20,10,5));
            QColor glow("#ffd544");
            int alpha = 120 + (int)(std::sin(m_globalTime*4) * 40);
            glow.setAlpha(std::max(0, std::min(255, alpha)));
            p.setPen(QPen(glow, 1)); p.setBrush(Qt::NoBrush);
            p.drawRect(x+4, y+4, TILE-9, TILE-9); p.setPen(Qt::NoPen);
        }
    };
    drawDoor(DOOR_COL, 0, m_doorOpen[0]);
    drawDoor(COLS-1, DOOR_ROW, m_doorOpen[1]);
    drawDoor(DOOR_COL, ROWS-1, m_doorOpen[2]);
    drawDoor(0, DOOR_ROW, m_doorOpen[3]);
}

void GameWidget::drawProgressionBar(QPainter &p, int x, int y, int w)
{
    int h = 8;
    p.fillRect(QRectF(x, y, w, h), QColor(20, 15, 10));
    p.setPen(QPen(C_GOLD, 1)); p.setBrush(Qt::NoBrush);
    p.drawRect(x, y, w, h); p.setPen(Qt::NoPen);
    float per = float(w) / ROOMS_TOTAL;
    for (int i = 1; i <= ROOMS_TOTAL; ++i) {
        float rx = x + (i-1)*per;
        QColor col = (i < m_currentRoom) ? QColor(40, 110, 40) :
                     (i == m_currentRoom) ? QColor(220, 180, 40) : QColor(60, 50, 40);
        p.fillRect(QRectF(rx+0.5f, y+1, per-1, h-2), col);
    }
    // Boss markers : 5/10/15/20 = circle, 25 = star
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
            QColor mc = (wIdx >= 0 && wIdx < m_worlds.size()) ? m_worlds[wIdx].accent : QColor("#cc2244");
            p.setBrush(mc); p.setPen(QPen(QColor("#ffcc44"), 1));
            p.drawEllipse(QPointF(cx, cy), 5, 5);
        }
        p.setPen(Qt::NoPen); p.setBrush(Qt::NoBrush);
    };
    drawMarker(5, false); drawMarker(10, false); drawMarker(15, false);
    drawMarker(20, false); drawMarker(25, true);
}

void GameWidget::drawHUD(QPainter &p)
{
    p.fillRect(0, GH, GW, HUD, C_HUD);
    p.setPen(QPen(C_HB, 2)); p.drawRect(0, GH, GW, HUD-1); p.setPen(Qt::NoPen);
    QFont font("monospace", 9, QFont::Bold);
    p.setFont(font); p.setPen(C_GOLD); p.drawText(8, GH+18, "PV");
    int hpDisplay = std::min((int)m_player.maxHp, 8);
    for (int i = 0; i < hpDisplay; ++i) {
        bool full = i < m_player.hp;
        p.fillRect(QRectF(28+i*15, GH+8, 12, 12), full?C_HP:C_HPB);
        if (full) p.fillRect(QRectF(30+i*15, GH+10, 4, 3), QColor("#ff6688"));
    }
    if ((int)m_player.maxHp > 8) {
        p.setFont(QFont("monospace", 7));
        p.setPen(C_GOLD);
        p.drawText(28+8*15+4, GH+18, QString("+%1").arg((int)m_player.maxHp - 8));
    }
    if (m_player.grenadeAmmo > 0 || hasSkill(SK_GRN)) {
        QFont gf("monospace", 7, QFont::Bold);
        p.setFont(gf); p.setPen(QColor("#ff8844"));
        p.drawText(8, GH+33, QString("[G]x%1").arg(m_player.grenadeAmmo));
    }
    if (hasSkill(SK_DASH)) {
        QFont gf("monospace", 7, QFont::Bold);
        p.setFont(gf);
        p.setPen(m_player.dashCd <= 0 ? QColor("#00ddff") : QColor("#557799"));
        p.drawText(60, GH+33, QString("[SHIFT]%1").arg(m_player.dashCd > 0 ? QString::number(m_player.dashCd, 'f', 1) : "OK"));
    }
    if (hasSkill(SK_TIME_STOP) && !m_player.timeStopUsed) {
        QFont gf("monospace", 7, QFont::Bold);
        p.setFont(gf); p.setPen(QColor("#7799ff"));
        p.drawText(140, GH+33, "[T]");
    }
    if ((m_enemies.size() > 0) && (m_enemies[0].type == ET_MiniBoss || m_enemies[0].type == ET_FinalBoss)
        || (m_bossIndex >= 0 && m_bossIndex < (int)m_enemies.size()
            && (m_enemies[m_bossIndex].type == ET_MiniBoss || m_enemies[m_bossIndex].type == ET_FinalBoss))) {
        // Find boss
        Enemy *boss = nullptr;
        for (auto &e : m_enemies) if (e.type==ET_MiniBoss || e.type==ET_FinalBoss) { boss = &e; break; }
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
            QString name = (wIdx>=0 && wIdx<m_worlds.size()) ? m_worlds[wIdx].bossNameFr : "BOSS";
            QString label = (boss->type == ET_FinalBoss) ?
                QString("%1 - PHASE %2").arg(name).arg(boss->phase) :
                ((boss->phase==2) ? QString("%1 - FUREUR").arg(name) : name);
            QFontMetrics fm(sf);
            p.drawText(GW/2 - fm.horizontalAdvance(label)/2, by-2, label);
        }
    }
    drawProgressionBar(p, 8, GH+38, GW-16);
    QFont sf("monospace", 7, QFont::Bold); p.setFont(sf); p.setPen(C_GOLD);
    int worldNum = worldOf(m_currentRoom);
    QString roomTxt = QString("M%1 SALLE %2/%3").arg(worldNum).arg(m_currentRoom).arg(ROOMS_TOTAL);
    if (m_state == GS_RoomCleared) roomTxt += " > PORTE";
    p.drawText(8, GH+56, roomTxt);
    QFont skf("monospace", 6, QFont::Bold); p.setFont(skf);
    QFontMetrics skfm(skf);
    int sx = GW - 8;
    for (int i = (int)m_player.skills.size()-1; i >= 0; --i) {
        const Skill *sk = nullptr;
        for (auto &s : m_allSkills) if (s.id == m_player.skills[i]) { sk = &s; break; }
        if (!sk) continue;
        int tw = skfm.horizontalAdvance(sk->icon) + 6;
        sx -= tw + 2;
        if (sx < 200) break;
        QColor bgc = sk->color; bgc.setAlpha(60);
        p.fillRect(QRectF(sx, GH+56, tw, 8), bgc);
        p.setPen(sk->color); p.drawText(sx+2, GH+62, sk->icon); p.setPen(Qt::NoPen);
    }
}


// ============================================================
//  MENU principal
// ============================================================
void GameWidget::drawMenu(QPainter &p)
{
    p.fillRect(0, 0, CW, CH, QColor(15, 8, 25));
    // Etoiles fond
    QRandomGenerator starRng(12345);
    for (int i = 0; i < 80; ++i) {
        float sx = starRng.bounded(CW), sy = starRng.bounded(CH);
        int alpha = 80 + (int)(std::sin(m_globalTime*1.5f + i)*60);
        QColor sc("#ffeebb"); sc.setAlpha(std::max(0, std::min(255, alpha)));
        p.fillRect(QRectF(sx, sy, 2, 2), sc);
    }

    // Titre
    QFont titleFont("monospace", 28, QFont::Bold);
    p.setFont(titleFont); QFontMetrics tfm(titleFont);
    QString title = "RETRO ARCHER";
    int tw = tfm.horizontalAdvance(title);
    p.setPen(QColor(0,0,0)); p.drawText(CW/2 - tw/2 + 3, 70+3, title);
    p.setPen(C_GOLD); p.drawText(CW/2 - tw/2, 70, title);

    QFont subFont("monospace", 9);
    p.setFont(subFont); QFontMetrics sfm(subFont);
    QString sub = "5 mondes - 50 power-ups - 5 boss legendaires";
    p.setPen(QColor("#bbaaff"));
    p.drawText(CW/2 - sfm.horizontalAdvance(sub)/2, 92, sub);

    // Apercu sprite
    drawSpriteAt(p, m_sprSoldierSkin[m_menuSelectedSkin][AN_Idle], s_idleFrameCount, 8.f,
                 CW/2.f, 165, 1.6f, false, m_globalTime);

    // Boutons (4)
    auto drawBtn = [&](int y, const QString &key, const QString &label, QColor accent) {
        QFont bf("monospace", 11, QFont::Bold); p.setFont(bf); QFontMetrics bfm(bf);
        QString text = QString("[%1] %2").arg(key, label);
        int w = bfm.horizontalAdvance(text) + 24;
        int x = CW/2 - w/2;
        QColor bg(20, 20, 40); p.fillRect(QRectF(x, y, w, 26), bg);
        p.setPen(QPen(accent, 2)); p.setBrush(Qt::NoBrush);
        p.drawRect(x, y, w, 26); p.setPen(Qt::NoPen);
        p.setPen(accent); p.drawText(x+12, y+18, text); p.setPen(Qt::NoPen);
    };
    drawBtn(220, "ESPACE", "Jouer",         QColor("#88ff88"));
    drawBtn(254, "C",      "Personnaliser", QColor("#ffaa44"));
    drawBtn(288, "D",      "Decouvertes",   QColor("#aaccff"));

    // Stats bas
    QFont mf("monospace", 8); p.setFont(mf);
    p.setPen(QColor("#ffd544"));
    QString hsTxt = QString("Record : Salle %1/%2  -  Mondes debloques : %3/5")
                    .arg(m_highScore).arg(ROOMS_TOTAL).arg(m_worldsUnlocked);
    QFontMetrics mfm(mf);
    p.drawText(CW/2 - mfm.horizontalAdvance(hsTxt)/2, CH-20, hsTxt);
    QString controls = "ZQSD : Bouger  -  Tirs auto si immobile  -  ESC : retour";
    p.setPen(QColor("#7766aa"));
    p.drawText(CW/2 - mfm.horizontalAdvance(controls)/2, CH-8, controls);
}

// ============================================================
//  ECRAN DECOUVERTES - 5 mondes, boss grises si non debloques
// ============================================================
void GameWidget::drawDiscoveries(QPainter &p)
{
    p.fillRect(0, 0, CW, CH, QColor(8, 5, 20));
    // Trame fond
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
    QString s = QString("Mondes debloques : %1 / 5").arg(m_worldsUnlocked);
    p.setPen(QColor("#bbaaff"));
    p.drawText(CW/2 - sfm.horizontalAdvance(s)/2, 38, s);

    // 5 cards horizontales
    int cardW = 95, cardH = 200, gap = 8;
    int totalW = cardW*5 + gap*4;
    int startX = CW/2 - totalW/2;
    int cardY = 60;

    for (int i = 0; i < 5; ++i) {
        int cx = startX + i*(cardW+gap);
        bool unlocked = (i+1) <= m_worldsUnlocked;
        bool hover = (i == m_discoveryHover);
        const WorldInfo &wi = m_worlds[i];
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

        // Boss preview
        QImage sheet = m_bossSheets[wi.bossKey];
        if (!sheet.isNull()) {
            int frame = unlocked ? ((int)(m_globalTime * wi.bossFps) % wi.bossFrameCount) : 0;
            int fw = wi.bossFrameW, fh = wi.bossFrameH;
            float zone = 75.f;
            float scale = std::min(zone/fw, zone/fh) * 0.95f;
            float dw = fw*scale, dh = fh*scale;
            float dx = cx + cardW/2.f - dw/2.f;
            float dy = cardY + 22 + 80 - dh/2.f;
            QRectF dst(dx, dy, dw, dh), src(frame*fw, 0, fw, fh);
            if (unlocked) p.drawImage(dst, sheet, src);
            else {
                // Silhouette grise
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
                // Cadenas
                QFont lf("monospace", 14, QFont::Bold); p.setFont(lf);
                QFontMetrics lfm(lf);
                QString lock = "X";
                p.setPen(QColor("#ff4466"));
                p.drawText(cx + cardW/2 - lfm.horizontalAdvance(lock)/2,
                           cardY + 22 + 80 + 6, lock);
                p.setPen(Qt::NoPen);
            }
        }

        // Boss name
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

        // Skills count
        int total = 0, available = 0;
        for (auto &sk : m_allSkills) {
            if (sk.worldTier == i+1) {
                total++;
                if ((i+1) <= m_worldsUnlocked) available++;
            }
        }
        QFont skf("monospace", 6); p.setFont(skf); QFontMetrics skfm(skf);
        QString sktxt = QString("%1/%2 skills").arg(available).arg(total);
        p.setPen(QColor("#88ccff"));
        p.drawText(cx + cardW/2 - skfm.horizontalAdvance(sktxt)/2, cardY + 158, sktxt);

        // Status
        p.setFont(skf);
        QString status = unlocked ? (i+1 < m_worldsUnlocked ? "VAINCU" : "ACTIF") : "VERROUILLE";
        QColor stc = unlocked ? (i+1 < m_worldsUnlocked ? QColor("#88ff88") : QColor("#ffcc44")) : QColor("#ff5566");
        p.setPen(stc);
        p.drawText(cx + cardW/2 - skfm.horizontalAdvance(status)/2, cardY + 175, status);
    }

    // Detail flavor pour hover
    if (m_discoveryHover >= 0 && m_discoveryHover < 5) {
        const WorldInfo &wi = m_worlds[m_discoveryHover];
        bool unlocked = (m_discoveryHover+1) <= m_worldsUnlocked;
        QFont nf("monospace", 9, QFont::Bold); p.setFont(nf); QFontMetrics nfm(nf);
        QString name = wi.name; int nw = nfm.horizontalAdvance(name);
        p.setPen(unlocked ? wi.accent : QColor("#7066aa"));
        p.drawText(CW/2 - nw/2, cardY + cardH + 22, name);
        QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
        QString fla = unlocked ? wi.flavor : "Survis aux mondes precedents pour le decouvrir...";
        p.setPen(QColor("#ddccff"));
        p.drawText(CW/2 - ffm.horizontalAdvance(fla)/2, cardY + cardH + 38, fla);
    }

    // Footer
    QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
    QString footer = "<-/-> selection  -  ESC : retour";
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - ffm.horizontalAdvance(footer)/2, CH-12, footer);
}

// ============================================================
//  SKIN / ATK SELECT
// ============================================================
void GameWidget::drawSkinSelect(QPainter &p)
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
        bool sel = (i == m_menuSelectedSkin);
        QColor bg = sel ? QColor(60, 50, 100) : QColor(25, 18, 40);
        p.fillRect(x, sy, cardW, 110, bg);
        if (sel) { p.setPen(QPen(g_skins[i].accent, 3)); p.setBrush(Qt::NoBrush);
                   p.drawRect(x, sy, cardW, 110); p.setPen(Qt::NoPen); }
        drawSpriteAt(p, m_sprSoldierSkin[i][AN_Idle], s_idleFrameCount, 8.f,
                     x + cardW/2.f, sy + 78, 1.4f, false, m_globalTime);
        QFont nf("monospace", 7, QFont::Bold); p.setFont(nf); QFontMetrics nfm(nf);
        p.setPen(g_skins[i].accent);
        p.drawText(x + cardW/2 - nfm.horizontalAdvance(g_skins[i].name)/2, sy+102, g_skins[i].name);
    }

    p.setFont(sub); p.setPen(QColor("#bbaaff"));
    QString s2 = "Style d'attaque (Z/S) :";
    p.drawText(CW/2 - sfm.horizontalAdvance(s2)/2, 215, s2);
    int aSY = 230;
    for (int i = 0; i < 3; ++i) {
        int aY = aSY + i*30;
        bool sel = (i == m_menuSelectedAtk);
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

// ============================================================
//  SKILL SELECT
// ============================================================
void GameWidget::drawSkillSelect(QPainter &p)
{
    QColor overlay(0,0,0); overlay.setAlphaF(0.78f);
    p.fillRect(0, 0, CW, CH, overlay);
    QFont titleFont("monospace", 14, QFont::Bold);
    p.setFont(titleFont); QFontMetrics tfm(titleFont);
    QString title = "CHOISIS UNE AMELIORATION";
    int tw = tfm.horizontalAdvance(title);
    p.setPen(C_GOLD); p.drawText(CW/2 - tw/2, 40, title);

    int n = (int)m_skillChoices.size();
    if (n <= 0) { applySkill(SK_HARDCORE); return; }
    int cardW = 145, gap = 12;
    int totalW = cardW*n + gap*(n-1);
    int sx = CW/2 - totalW/2, sy = 80;
    for (int i = 0; i < n; ++i) {
        int x = sx + i*(cardW+gap);
        const Skill *sk = nullptr;
        for (auto &s : m_allSkills) if (s.id == m_skillChoices[i]) { sk = &s; break; }
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
        // wrap simple
        QStringList words = desc.split(' ');
        int lineY = sy+128, ww = 0; QString line;
        for (auto &w : words) {
            int aw = dfm.horizontalAdvance(line + (line.isEmpty()?"":" ") + w);
            if (aw > cardW-12 && !line.isEmpty()) {
                p.drawText(x + cardW/2 - dfm.horizontalAdvance(line)/2, lineY, line);
                lineY += 11; line = w;
            } else line = line.isEmpty() ? w : line + " " + w;
        }
        if (!line.isEmpty())
            p.drawText(x + cardW/2 - dfm.horizontalAdvance(line)/2, lineY, line);

        // Tier label
        QFont tf("monospace", 6, QFont::Bold); p.setFont(tf); QFontMetrics tfm2(tf);
        QString tier = QString("MONDE %1").arg(sk->worldTier);
        p.setPen(sk->color.lighter(170));
        p.drawText(x + cardW/2 - tfm2.horizontalAdvance(tier)/2, sy+193, tier);
    }
    QFont ff("monospace", 8); p.setFont(ff); QFontMetrics ffm(ff);
    QString tip = "Appuie sur 1, 2 ou 3 pour choisir";
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - ffm.horizontalAdvance(tip)/2, CH-30, tip);
}

// ============================================================
//  END SCREEN
// ============================================================
void GameWidget::drawEndScreen(QPainter &p, bool win)
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
    QString s = win ? QString("Tu as terrasse le Gardien du Givre !")
                    : QString("Tu es tombe dans la salle %1 (M%2)").arg(m_currentRoom).arg(worldOf(m_currentRoom));
    p.setPen(QColor("#ddccff"));
    p.drawText(CW/2 - sfm.horizontalAdvance(s)/2, CH/2 + 8, s);

    QString h = QString("Record : Salle %1/%2  -  Mondes debloques : %3/5")
                .arg(m_highScore).arg(ROOMS_TOTAL).arg(m_worldsUnlocked);
    p.setPen(QColor("#aaaaff"));
    p.drawText(CW/2 - sfm.horizontalAdvance(h)/2, CH/2 + 26, h);

    QString r = "ESPACE : rejouer    -    ESC : menu";
    p.setPen(QColor("#88ff88"));
    p.drawText(CW/2 - sfm.horizontalAdvance(r)/2, CH/2 + 50, r);
}
