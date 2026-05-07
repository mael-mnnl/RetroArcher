#ifndef CONTROLLER_SKILLSYSTEM_H
#define CONTROLLER_SKILLSYSTEM_H

namespace Model { struct GameModel; }

namespace Controller { namespace Skills {

bool  hasSkill(const Model::GameModel &m, int id);
int   skillStacks(const Model::GameModel &m, int id);
float damageMul(const Model::GameModel &m);
float speedMul(const Model::GameModel &m);
float fireRateMul(const Model::GameModel &m);
float incomingDmgMul(const Model::GameModel &m);
int   pierceLevel(const Model::GameModel &m);
int   bounceLevel(const Model::GameModel &m);

void  generateSkills(Model::GameModel &m);
void  applySkill(Model::GameModel &m, int skillId, void (*onAfterApply)(Model::GameModel&));

}}

#endif
