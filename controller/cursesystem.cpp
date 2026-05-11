#include "cursesystem.h"
#include "../model/gamemodel.h"
#include "../model/types.h"
#include "../core/utils.h"
#include <algorithm>

namespace Controller { namespace Curse {

using namespace Model;

bool active(const GameModel &m, int id) {
    return std::find(m.activeCurses.begin(), m.activeCurses.end(), id) != m.activeCurses.end();
}

void generateChoices(GameModel &m)
{
    std::vector<int> avail;
    for (auto &c : m.allCurses)
        if (!active(m, c.id)) avail.push_back(c.id);
    for (int i = (int)avail.size()-1; i > 0; --i) {
        int j = Core::rndI(0, i); std::swap(avail[i], avail[j]);
    }
    m.curseChoices.clear();
    for (int i = 0; i < std::min((int)avail.size(), 2); ++i)
        m.curseChoices.push_back(avail[i]);
}

void apply(GameModel &m, int curseId) {
    if (!active(m, curseId)) m.activeCurses.push_back(curseId);
}

float enemySpeedMul(const GameModel &m)    { return active(m, CRS_FastEnemies) ? 1.20f : 1.f; }
float playerIncomingMul(const GameModel &m){ return active(m, CRS_FragileGlass) ? 2.0f : 1.f; }
float dashCdMul(const GameModel &m)        { return active(m, CRS_SlowDash) ? 2.0f : 1.f; }

}}
