#ifndef CONTROLLER_TUTORIALSYSTEM_H
#define CONTROLLER_TUTORIALSYSTEM_H

namespace Model { struct GameModel; }

namespace Controller { namespace Tutorial {
void start(Model::GameModel &m);
void buildRoom(Model::GameModel &m, int step);
void update(Model::GameModel &m, float dt);
}}

#endif
