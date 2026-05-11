#ifndef CONTROLLER_RELICSYSTEM_H
#define CONTROLLER_RELICSYSTEM_H
namespace Model { struct GameModel; }
namespace Controller { namespace Relic {
void  generateChoices(Model::GameModel &m);
void  apply(Model::GameModel &m, int relicId);
bool  has(const Model::GameModel &m, int relicId);
float enemyHpMul(const Model::GameModel &m);
float damageDealtMul(const Model::GameModel &m);
float goldMul(const Model::GameModel &m);
}}
#endif
