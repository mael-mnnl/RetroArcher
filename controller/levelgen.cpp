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
    // Boss
    if (roomIndex % 5 == 0) return RT_Boss;
    // Salle d'élite : 1 sur 5 (pas une boss), assez deterministe
    int posInWorld = ((roomIndex - 1) % 5) + 1;
    int worldIdx = ((roomIndex - 1) / 5) + 1;
    // Décision pseudo-aléatoire stable par run pour les salles speciales :
    // - 1ere salle d'un monde : 30% chance shop OU forge OU defi
    // - 3eme salle : 25% chance élite
    // - 4eme : 20% chance élite
    quint32 seed = (quint32)(roomIndex * 9176 + (qint64)m.globalTime/40);
    QRandomGenerator g(seed);
    double r = g.generateDouble();
    if (posInWorld == 1 && roomIndex > 1) {
        if (r < 0.33) return RT_Shop;
        if (r < 0.55) return RT_Forge;
        if (r < 0.75) return RT_Challenge;
    } else if (posInWorld == 2 || posInWorld == 3) {
        if (r < 0.22 + 0.05*worldIdx) return RT_Elite;
    } else if (posInWorld == 4) {
        if (r < 0.18) return RT_Elite;
    }
    return RT_Normal;
}

bool obstacleAt(const GameModel &m, int col, int row)
{
    for (auto &o : m.obstacles) if (o.col == col && o.row == row) return true;
    return false;
}

void generateObstacles(GameModel &m, int roomIndex)
{
    m.obstacles.clear();
    // Salles spéciales : layouts dédiés
    if (m.roomType == RT_Shop || m.roomType == RT_Forge) {
        // Quelques piliers décoratifs
        for (int c : {3, COLS-4}) {
            for (int r : {3, ROWS-4}) {
                m.obstacles.push_back({c, r, 0, 0.f});
            }
        }
        return;
    }
    if (m.roomType == RT_Challenge) {
        // Defi : labyrinthe légeret - lignes de piliers
        for (int r = 3; r <= ROWS-4; r += 2) {
            for (int c = 3; c <= COLS-4; c += 2) {
                if ((c+r)%3 == 0) m.obstacles.push_back({c, r, 0, 0.f});
            }
        }
        return;
    }
    if (m.roomType == RT_Boss) {
        // Salle boss : épurée, 4 piliers aux coins
        for (int c : {2, COLS-3}) for (int r : {2, ROWS-3})
            m.obstacles.push_back({c, r, 0, 0.f});
        return;
    }

    // Salle normale / élite : génération semi-aléatoire
    int worldIdx = std::min(6, ((roomIndex-1)/5)+1) - 1;
    quint32 seed = (quint32)(roomIndex * 7919 + worldIdx * 3137);
    QRandomGenerator g(seed);

    int count = 4 + g.bounded(0, 5); // 4..8 obstacles
    int safety = 40;
    while ((int)m.obstacles.size() < count && safety > 0) {
        --safety;
        int c = 2 + g.bounded(0, COLS-4);
        int r = 2 + g.bounded(0, ROWS-4);
        // Pas trop pres du centre (joueur)
        if (std::abs(c - COLS/2) <= 1 && std::abs(r - ROWS/2) <= 1) continue;
        // Pas devant les portes
        if (c == DOOR_COL && (r <= 2 || r >= ROWS-3)) continue;
        if (r == DOOR_ROW && (c <= 2 || c >= COLS-3)) continue;
        if (obstacleAt(m, c, r)) continue;
        // Pas 4 obstacles cote a cote
        int adj = 0;
        if (obstacleAt(m, c-1, r)) adj++;
        if (obstacleAt(m, c+1, r)) adj++;
        if (obstacleAt(m, c, r-1)) adj++;
        if (obstacleAt(m, c, r+1)) adj++;
        if (adj >= 2) continue;
        int kind = g.bounded(0, 3);
        m.obstacles.push_back({c, r, kind, 0.f});
    }
    // Symetrie partielle : ajouter le miroir pour certaines salles
    if ((roomIndex % 3) == 0) {
        std::vector<ObstacleTile> mirror;
        for (auto &o : m.obstacles) {
            int mc = COLS - 1 - o.col;
            if (mc != o.col && !obstacleAt(m, mc, o.row))
                mirror.push_back({mc, o.row, o.kind, 0.f});
        }
        for (auto &o : mirror) m.obstacles.push_back(o);
    }
}

}}
