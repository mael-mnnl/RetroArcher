#ifndef CONTROLLER_LEVELGEN_H
#define CONTROLLER_LEVELGEN_H

namespace Model { struct GameModel; }

namespace Controller { namespace LevelGen {

// Determine type de salle a partir du room index
int  pickRoomType(const Model::GameModel &m, int roomIndex);

// Genere les obstacles dans la salle (apres clear m.obstacles)
void generateObstacles(Model::GameModel &m, int roomIndex);

// Verifie si une tile est obstruee par un obstacle (en plus des murs)
bool obstacleAt(const Model::GameModel &m, int col, int row);

}}

#endif
