#ifndef MODEL_SKILLSDATA_H
#define MODEL_SKILLSDATA_H

#include "entities.h"
#include <vector>

namespace Model {
// Remplit la liste de tous les skills (65 entries)
void loadSkills(std::vector<Skill> &out);
}

#endif
