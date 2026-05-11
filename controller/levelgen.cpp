#include "levelgen.h"
#include "../model/gamemodel.h"
#include "../model/types.h"
#include "../core/utils.h"
#include "../core/constants.h"
#include <QRandomGenerator>
#include <algorithm>

namespace Controller { namespace LevelGen {

using namespace Model;
using namespace Core;

int pickRoomType(const GameModel &m, int roomIndex)
{
    if (roomIndex % 5 == 0) return RT_Boss;

    int posInWorld = ((roomIndex - 1) % 5) + 1;
    quint32 seed = (quint32)(roomIndex * 9176 + (qint64)m.globalTime/40);
    QRandomGenerator g(seed);
    double r = g.generateDouble();

    if (posInWorld == 1 && roomIndex > 1) {
        if (r < 0.25) return RT_Shop;
        if (r < 0.45) return RT_Forge;
        if (r < 0.58) return RT_Challenge;
        if (r < 0.72) return RT_BlackMarket;
    } else if (posInWorld == 3) {
        if (r < 0.15) return RT_Shop;
        if (r < 0.25) return RT_BlackMarket;
    }
    return RT_Normal;
}

void generateObstacles(GameModel &m, int /*roomIndex*/)
{
    m.obstacles.clear();
    // Salles sans obstacles - seulement les piliers décoratifs pour shop/forge
    if (m.roomType == RT_Shop || m.roomType == RT_Forge || m.roomType == RT_BlackMarket) {
        for (int c : {3, COLS-4}) {
            for (int r : {3, ROWS-4}) {
                m.obstacles.push_back({c, r, 0, 0.f});
            }
        }
    }
    // Toutes les autres salles (Normal, Boss, Challenge, Elite) : vides
}

}}
