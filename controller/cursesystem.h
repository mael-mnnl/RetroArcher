#ifndef CONTROLLER_CURSESYSTEM_H
#define CONTROLLER_CURSESYSTEM_H
namespace Model { struct GameModel; }
namespace Controller { namespace Curse {
bool  active(const Model::GameModel &m, int curseId);
void  generateChoices(Model::GameModel &m);
void  apply(Model::GameModel &m, int curseId);
float enemySpeedMul(const Model::GameModel &m);
float playerIncomingMul(const Model::GameModel &m);
float dashCdMul(const Model::GameModel &m);
}}
#endif
