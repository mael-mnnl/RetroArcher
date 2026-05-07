#ifndef CONTROLLER_COLLISIONSYSTEM_H
#define CONTROLLER_COLLISIONSYSTEM_H

namespace Model { struct GameModel; }

namespace Controller { namespace Collision {
bool isObstacleTile(const Model::GameModel &m, int col, int row);
bool collidesObstacle(const Model::GameModel &m, float x, float y, float r);
void resolveCollision(const Model::GameModel &m, float &x, float &y, float r);
void tryMove(const Model::GameModel &m, float &x, float &y, float dx, float dy, float r);
}}

#endif
