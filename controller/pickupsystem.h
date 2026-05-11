#ifndef CONTROLLER_PICKUPSYSTEM_H
#define CONTROLLER_PICKUPSYSTEM_H
namespace Model { struct GameModel; }
namespace Controller { namespace Pickup {
void spawnGold(Model::GameModel &m, float x, float y, int amount);
void spawnHeart(Model::GameModel &m, float x, float y);
void spawnPotion(Model::GameModel &m, float x, float y);
void spawnScroll(Model::GameModel &m, float x, float y);
void spawnChest(Model::GameModel &m, float x, float y);
void tryRandomDrop(Model::GameModel &m, float x, float y, bool elite);
void update(Model::GameModel &m, float dt);
}}
#endif
