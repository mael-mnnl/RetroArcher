#ifndef MODEL_SPELLSDATA_H
#define MODEL_SPELLSDATA_H

#include <vector>
#include "entities.h"

namespace Model {
    void loadSpells(std::vector<SpellInfo> &out);
    EquipItem makeEquipItem(EquipType type, int rarity, int worldTier);
}

#endif
