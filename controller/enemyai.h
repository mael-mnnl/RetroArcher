#ifndef CONTROLLER_ENEMYAI_H
#define CONTROLLER_ENEMYAI_H

namespace Model { struct GameModel; }

namespace Controller { namespace EnemyAI {
void update(Model::GameModel &m, float dt);
}}

#endif
