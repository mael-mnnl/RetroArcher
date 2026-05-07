#ifndef CONTROLLER_COMBATSYSTEM_H
#define CONTROLLER_COMBATSYSTEM_H

namespace Model { struct GameModel; struct Enemy; }

namespace Controller { namespace Combat {

void shootPlayer(Model::GameModel &m);
void throwGrenade(Model::GameModel &m);
void activateDash(Model::GameModel &m);
void activateTimeStop(Model::GameModel &m);
void hurtPlayer(Model::GameModel &m, float dmg);
void hurtEnemy(Model::GameModel &m, Model::Enemy &e, float dmg, bool fromExplosion = false);
void updateBullets(Model::GameModel &m, float dt);
void updateParticles(Model::GameModel &m, float dt);

}}

#endif
