#ifndef VIEW_ASSETMANAGER_H
#define VIEW_ASSETMANAGER_H

#include "../model/types.h"
#include <QImage>
#include <QHash>
#include <QString>

namespace View {

class AssetManager {
public:
    static const int s_walkFrameCount  = 8;
    static const int s_idleFrameCount  = 6;
    static const int s_hurtFrameCount  = 4;
    static const int s_deathFrameCount = 4;
    static const int s_atk1Frames      = 6;
    static const int s_atk2Frames      = 6;
    static const int s_atk3Frames      = 9;

    AssetManager();
    void load();
    void buildSkinSprites();

    QImage soldierSheet(Model::AnimType anim, int skinIndex, int atkVariant) const;
    int    soldierFrameCount(Model::AnimType anim, int atkVariant) const;
    float  soldierFps(Model::AnimType anim, int atkVariant) const;

    static QImage hueShift(const QImage &src, int degrees, double sat, double val);

    // Public sprite arrays (read-only consumers - kept simple)
    QImage sprSoldierBase[5];
    QImage sprSoldierAtkVariants[3];
    QImage sprSoldierSkin[4][5];
    QImage sprSoldierAtkSkin[4][3];
    QImage sprArrow;
    QImage sprOrc[6][5];
    QHash<QString, QImage> bossSheets;
};

}

#endif
