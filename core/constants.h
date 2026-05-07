#ifndef CORE_CONSTANTS_H
#define CORE_CONSTANTS_H

namespace Core {
constexpr int TILE = 32;
constexpr int COLS = 17;
constexpr int ROWS = 13;
constexpr int GW   = COLS * TILE;     // 544
constexpr int GH   = ROWS * TILE;     // 416
constexpr int HUD  = 64;
constexpr int CW   = GW;
constexpr int CH   = GH + HUD;        // 480
constexpr int DISPLAY_SCALE = 2;
constexpr int ROOMS_TOTAL = 25;
constexpr int DOOR_COL = COLS / 2;
constexpr int DOOR_ROW = ROWS / 2;
}

#endif
