#include "assetmanager.h"

namespace View {

struct SkinDef { int hue; double sat; double val; };
static const SkinDef g_skinHsv[4] = {
    {   0, 1.0, 1.0  },
    {  90, 1.0, 0.95 },
    {-110, 1.2, 0.9  },
    {-150, 1.4, 1.05 }
};

AssetManager::AssetManager() {}

QImage AssetManager::hueShift(const QImage &src, int degrees, double sat, double val)
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

void AssetManager::load()
{
    sprSoldierBase[Model::AN_Idle]  = QImage(":/sprites/soldier_idle.png");
    sprSoldierBase[Model::AN_Walk]  = QImage(":/sprites/soldier_walk.png");
    sprSoldierBase[Model::AN_Atk]   = QImage(":/sprites/soldier_atk1.png");
    sprSoldierBase[Model::AN_Hurt]  = QImage(":/sprites/soldier_hurt.png");
    sprSoldierBase[Model::AN_Death] = QImage(":/sprites/soldier_death.png");
    sprSoldierAtkVariants[0] = QImage(":/sprites/soldier_atk1.png");
    sprSoldierAtkVariants[1] = QImage(":/sprites/soldier_atk2.png");
    sprSoldierAtkVariants[2] = QImage(":/sprites/soldier_atk3.png");
    sprArrow = QImage(":/sprites/arrow.png");

    QImage rawOrc[5];
    rawOrc[Model::AN_Idle]  = QImage(":/sprites/orc_idle.png");
    rawOrc[Model::AN_Walk]  = QImage(":/sprites/orc_walk.png");
    rawOrc[Model::AN_Atk]   = QImage(":/sprites/orc_atk.png");
    rawOrc[Model::AN_Hurt]  = QImage(":/sprites/orc_hurt.png");
    rawOrc[Model::AN_Death] = QImage(":/sprites/orc_death.png");
    for (int a = 0; a < 5; ++a) {
        sprOrc[0][a] = hueShift(rawOrc[a], 120, 1.8, 1.0);
        sprOrc[1][a] = hueShift(rawOrc[a],  40, 0.4, 1.3);
        sprOrc[2][a] = hueShift(rawOrc[a], 260, 2.0, 1.0);
        sprOrc[3][a] = hueShift(rawOrc[a],   0, 0.6, 0.7);
        sprOrc[4][a] = hueShift(rawOrc[a], 200, 2.0, 1.0);
        sprOrc[5][a] = hueShift(rawOrc[a], 350, 2.0, 0.9);
    }
    bossSheets["fireworm"]      = QImage(":/sprites/boss_fireworm.png");
    bossSheets["undead"]        = QImage(":/sprites/boss_undead.png");
    bossSheets["demonslime"]    = QImage(":/sprites/boss_demonslime.png");
    bossSheets["mino"]          = QImage(":/sprites/boss_mino.png");
    bossSheets["frostguardian"] = QImage(":/sprites/boss_frostguardian.png");
}

void AssetManager::buildSkinSprites()
{
    for (int s = 0; s < 4; ++s) {
        const SkinDef &sd = g_skinHsv[s];
        for (int a = 0; a < 5; ++a)
            sprSoldierSkin[s][a] = hueShift(sprSoldierBase[a], sd.hue, sd.sat, sd.val);
        for (int v = 0; v < 3; ++v)
            sprSoldierAtkSkin[s][v] = hueShift(sprSoldierAtkVariants[v], sd.hue, sd.sat, sd.val);
    }
}

QImage AssetManager::soldierSheet(Model::AnimType anim, int skin, int atkVariant) const
{
    if (anim == Model::AN_Atk) return sprSoldierAtkSkin[skin][atkVariant];
    return sprSoldierSkin[skin][anim];
}

int AssetManager::soldierFrameCount(Model::AnimType anim, int atkVariant) const
{
    if (anim == Model::AN_Idle) return s_idleFrameCount;
    if (anim == Model::AN_Walk) return s_walkFrameCount;
    if (anim == Model::AN_Hurt) return s_hurtFrameCount;
    if (anim == Model::AN_Death) return s_deathFrameCount;
    if (atkVariant == 0) return s_atk1Frames;
    if (atkVariant == 1) return s_atk2Frames;
    return s_atk3Frames;
}

float AssetManager::soldierFps(Model::AnimType anim, int atkVariant) const
{
    if (anim == Model::AN_Idle) return 8.f;
    if (anim == Model::AN_Walk) return 12.f;
    if (anim == Model::AN_Hurt) return 10.f;
    if (anim == Model::AN_Death) return 7.f;
    if (atkVariant == 0) return 14.f;
    if (atkVariant == 1) return 18.f;
    return 13.f;
}

}
