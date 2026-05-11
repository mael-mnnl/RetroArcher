#include "relicsystem.h"
#include "../model/gamemodel.h"
#include "../model/types.h"
#include "../core/utils.h"
#include <algorithm>

namespace Controller { namespace Relic {
using namespace Model;

void generateChoices(GameModel &m)
{
    std::vector<int> avail;
    for (auto &r : m.allRelics) avail.push_back(r.id);
    for (int i = (int)avail.size()-1; i > 0; --i) {
        int j = Core::rndI(0, i); std::swap(avail[i], avail[j]);
    }
    m.relicChoices.clear();
    for (int i = 0; i < std::min((int)avail.size(), 3); ++i)
        m.relicChoices.push_back(avail[i]);
}

void apply(GameModel &m, int rid)
{
    m.selectedRelicId = rid;
    if (rid == REL_GoldenSeed) {
        m.player.maxHp = std::max(1.f, m.player.maxHp - 1);
        m.player.hp = std::min(m.player.maxHp, m.player.hp);
    }
    if (rid == REL_PhoenixTear) m.runHasFreeRevive = true;
}

bool  has(const GameModel &m, int rid)   { return m.selectedRelicId == rid; }
float enemyHpMul(const GameModel &m)     { return has(m, REL_VoidPact) ? 1.20f : 1.f; }
float damageDealtMul(const GameModel &m) { return has(m, REL_VoidPact) ? 1.30f : 1.f; }
float goldMul(const GameModel &m)        { return has(m, REL_GoldenSeed) ? 1.50f : 1.f; }
}}
