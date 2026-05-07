#ifndef CONTROLLER_PLAYERCONTROLLER_H
#define CONTROLLER_PLAYERCONTROLLER_H

namespace Model { struct GameModel; }

namespace Controller { namespace PlayerCtl {
void update(Model::GameModel &m, float dt);
}}

#endif
