#ifndef CORE_PALETTE_H
#define CORE_PALETTE_H

#include <QColor>

namespace Palette {
inline const QColor C_FL1 ("#3a2a1a"), C_FL2 ("#302215");
inline const QColor C_WT  ("#3d1e08"), C_WF  ("#261408");
inline const QColor C_DC  ("#7a5c0a"), C_DO  ("#4a3408");
inline const QColor C_HUD ("#0a0a18"), C_HB  ("#c8a800");
inline const QColor C_HP  ("#dd1a33"), C_HPB ("#3a0010");
inline const QColor C_GOLD("#c8a800"), C_WH  ("#e8e8e8");
inline const QColor C_BE  ("#ff4400"), C_BEG ("#881100");
inline const QColor PAL_SLIME[2]  = { QColor("#1ab83a"), QColor("#0d7a22") };
inline const QColor PAL_BONE[2]   = { QColor("#c8c8c8"), QColor("#7a7a7a") };
inline const QColor PAL_FIRE[2]   = { QColor("#ffaa00"), QColor("#ff4400") };
inline const QColor PAL_ICE[2]    = { QColor("#aaddff"), QColor("#5599cc") };
inline const QColor PAL_POISON[2] = { QColor("#88ff44"), QColor("#226611") };
}

#endif
