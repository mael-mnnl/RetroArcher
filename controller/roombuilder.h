#ifndef CONTROLLER_ROOMBUILDER_H
#define CONTROLLER_ROOMBUILDER_H

#include "../model/types.h"

namespace Model { struct GameModel; }

namespace Controller { namespace Room {

int  worldOf(int room);
bool isBossRoom(int room);
bool isFinalBossRoom(int room);

void startGame(Model::GameModel &m);
void buildRoom(Model::GameModel &m, int roomIndex, int entryDoor);
void spawnMinion(Model::GameModel &m, float x, float y, int hp, float speed,
                 Model::EnemyType type = Model::ET_Minion);

}}

#endif
