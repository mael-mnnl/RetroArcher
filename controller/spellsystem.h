#ifndef CONTROLLER_SPELLSYSTEM_H
#define CONTROLLER_SPELLSYSTEM_H

namespace Model { struct GameModel; }

namespace Controller { namespace Spell {
    void castSpell(Model::GameModel &m, int slotIndex);
    void update(Model::GameModel &m, float dt);
    float spellDmgMul(const Model::GameModel &m);
}}

#endif
