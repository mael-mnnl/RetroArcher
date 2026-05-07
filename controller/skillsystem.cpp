#include "skillsystem.h"
#include "../model/gamemodel.h"
#include "../model/types.h"
#include "../core/utils.h"
#include <algorithm>

namespace Controller { namespace Skills {

using namespace Model;

bool hasSkill(const GameModel &m, int id) {
    return std::find(m.player.skills.begin(), m.player.skills.end(), id) != m.player.skills.end();
}

int skillStacks(const GameModel &m, int id) {
    return (int)std::count(m.player.skills.begin(), m.player.skills.end(), id);
}

float damageMul(const GameModel &m)
{
    float mul = 1.f;
    if (hasSkill(m, SK_POW))        mul *= 1.30f;
    if (hasSkill(m, SK_SHARP))      mul *= 1.08f;
    if (hasSkill(m, SK_BIG_ARROW))  mul *= 1.20f;
    if (hasSkill(m, SK_BLOOD_PACT)) mul *= 1.25f;
    if (hasSkill(m, SK_HEAVY))      mul *= 1.45f;
    if (hasSkill(m, SK_RECKLESS))   mul *= 1.40f;
    if (hasSkill(m, SK_BERSERKER) && m.player.hp < m.player.maxHp * 0.35f) mul *= 1.5f;
    if (hasSkill(m, SK_RAGE_STACK)) mul *= (1.f + 0.05f * std::min(10, m.player.rageStacks));
    if (hasSkill(m, SK_LEGEND))     mul *= (1.f + 0.10f * m.player.bossesKilled);
    if (hasSkill(m, SK_MASTERY))    mul *= 1.18f;
    if (hasSkill(m, SK_FOCUSED) && m.player.stillSec >= 1.f) mul *= 1.25f;
    return mul;
}

float speedMul(const GameModel &m)
{
    float mul = 1.f;
    if (hasSkill(m, SK_SPD))        mul *= 1.15f;
    if (hasSkill(m, SK_LIGHT_FOOT)) mul *= 1.08f;
    if (hasSkill(m, SK_TANK))       mul *= 0.90f;
    if (m.player.adrenalineTimer > 0) mul *= 1.30f;
    if (hasSkill(m, SK_LEGEND))     mul *= (1.f + 0.05f * m.player.bossesKilled);
    if (hasSkill(m, SK_MASTERY))    mul *= 1.18f;
    return mul;
}

float fireRateMul(const GameModel &m)
{
    float mul = 1.f;
    if (hasSkill(m, SK_FST))        mul *= 0.78f;
    if (hasSkill(m, SK_BIG_ARROW))  mul *= 1.08f;
    if (hasSkill(m, SK_HEAVY))      mul *= 1.18f;
    if (m.player.hp < m.player.maxHp * 0.25f && hasSkill(m, SK_LAST_HOPE)) mul *= 0.50f;
    if (hasSkill(m, SK_MASTERY))    mul *= 0.85f;
    return mul;
}

float incomingDmgMul(const GameModel &m)
{
    float mul = 1.f;
    if (hasSkill(m, SK_RECKLESS)) mul *= 1.25f;
    if (hasSkill(m, SK_TANK))     mul *= 0.92f;
    return mul;
}

int pierceLevel(const GameModel &m) { return hasSkill(m, SK_PRC) ? 1 : 0; }
int bounceLevel(const GameModel &m) { return hasSkill(m, SK_BNC) ? 1 : 0; }

void generateSkills(GameModel &m)
{
    std::vector<int> avail;
    for (auto &s : m.allSkills) {
        if (s.worldTier > m.worldsUnlocked) continue;
        bool has = hasSkill(m, s.id);
        if (s.id == SK_GRN || s.id == SK_MAXHP_NERFED || s.id == SK_HARDCORE
            || s.id == SK_TANK || s.id == SK_BLOOD_PACT || s.id == SK_LEGEND) {
            avail.push_back(s.id); continue;
        }
        if (!has) avail.push_back(s.id);
    }
    for (int i = (int)avail.size()-1; i > 0; --i) {
        int j = Core::rndI(0, i); std::swap(avail[i], avail[j]);
    }
    m.skillChoices.clear();
    for (int i = 0; i < std::min((int)avail.size(), 3); ++i)
        m.skillChoices.push_back(avail[i]);
}

void applySkill(GameModel &m, int skillId, void (*onAfter)(GameModel&))
{
    if (skillId == SK_HARDCORE)         { m.player.maxHp += 1; m.player.hp += 1; }
    else if (skillId == SK_MAXHP_NERFED){ m.player.maxHp += 1; m.player.hp = m.player.maxHp; }
    else if (skillId == SK_TANK)        { m.player.maxHp += 3; m.player.hp = std::min(m.player.maxHp, m.player.hp+3); }
    else if (skillId == SK_BLOOD_PACT)  { m.player.maxHp = std::max(1.f, m.player.maxHp-1);
                                          m.player.hp = std::min(m.player.maxHp, m.player.hp); }
    else if (skillId == SK_GRN)         m.player.grenadeAmmo += 3;

    m.player.skills.push_back(skillId);
    m.player.hp = std::min(m.player.maxHp, m.player.hp + 1.f);

    if (onAfter) onAfter(m);
}

}}
