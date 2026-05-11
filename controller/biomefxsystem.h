#ifndef CONTROLLER_BIOMEFXSYSTEM_H
#define CONTROLLER_BIOMEFXSYSTEM_H
namespace Model { struct GameModel; }
namespace Controller { namespace BiomeFX {
void update(Model::GameModel &m, float dt);
void spawnFx(Model::GameModel &m, const char *key, float x, float y, float scale = 1.f);
void updateFx(Model::GameModel &m, float dt);
}}
#endif
