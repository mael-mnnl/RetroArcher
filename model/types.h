#ifndef MODEL_TYPES_H
#define MODEL_TYPES_H

namespace Model {

enum EnemyType {
    ET_None, ET_Slime, ET_Skel, ET_Bat, ET_Brute, ET_Mage,
    ET_Minion, ET_MiniBoss, ET_FinalBoss,
    ET_Elite,         // champion en salle d'élite
    ET_TrueFinalBoss  // Lord Malificus (monde 6)
};

enum AnimType { AN_Idle, AN_Walk, AN_Atk, AN_Hurt, AN_Death };

enum GameState {
    GS_Menu, GS_SkinSelect, GS_Discoveries, GS_Lore,
    GS_Tutorial,
    GS_ClassSelect,    // choisir archetype
    GS_RelicSelect,    // choisir relique au depart
    GS_Playing, GS_RoomCleared, GS_FadeOut, GS_SkillSelect,
    GS_CurseSelect,    // apres boss, choisir malediction
    GS_Shop, GS_Forge, GS_Challenge,  // salles speciales (states actifs)
    GS_FadeIn,
    GS_GameOver, GS_Victory,
    GS_Blessings,      // ecran meta-progression
    GS_Leaderboard
};

enum RoomType {
    RT_Normal, RT_Boss, RT_Elite, RT_Shop, RT_Forge, RT_Challenge
};

enum CharClass {
    CC_Hero = 0,      // l'original, equilibre
    CC_Hunter = 1,    // chasseur : portee + vitesse
    CC_Paladin = 2,   // paladin : resistance + shield
    CC_Alchemist = 3  // alchimiste : grenade + elements
};

enum PickupType {
    PU_Gold, PU_Potion, PU_Scroll, PU_Heart, PU_Chest
};

enum CurseId {
    CRS_FastEnemies, CRS_PiercingShots, CRS_FragileGlass,
    CRS_NoRegen, CRS_SlowDash, CRS_BloodTax,
    CRS__COUNT
};

enum RelicId {
    REL_BloodOrb,      // kill heals 0.5, lose skill on death
    REL_BrokenWatch,   // timestop per room, no skills
    REL_SoulMirror,    // bullets bounce, hurt self
    REL_GoldenSeed,    // +50% gold but -1 maxHP
    REL_PhoenixTear,   // 1 free revive at full HP
    REL_VoidPact,      // +30% dmg but enemies +20% HP
    REL__COUNT
};

// Skill IDs - 65 total
enum SkillId {
    SK_DBL=0, SK_TRI, SK_FST, SK_PRC, SK_SPD, SK_POW, SK_BNC, SK_DIA,
    SK_HARDCORE, SK_DODGE, SK_LIGHT_FOOT, SK_RANGE, SK_KNOCKBACK, SK_BIG_ARROW,
    SK_HUNTER, SK_SHARP, SK_FIRST_HIT, SK_FOCUSED,
    SK_GRN,
    SK_LIFESTEAL, SK_NECRO, SK_DEATH_MARK, SK_BONE_SHIELD, SK_BLOOD_PACT,
    SK_REGEN, SK_RESILIENCE, SK_PHOENIX, SK_SOUL_DRAIN, SK_DECAY,
    SK_VAMP_NERFED, SK_FROST_FATE,
    SK_POISON, SK_BLAZE, SK_EXPLODE_KILL, SK_FIRE_TRAIL, SK_DOUBLE_GRENADE,
    SK_BIG_BOOM, SK_VOLCANO, SK_RECKLESS, SK_BURST_FIRE, SK_INFERNO,
    SK_STICKY, SK_GHOST_ARROW,
    SK_DASH, SK_RAM, SK_BERSERKER, SK_HEAVY, SK_THORNS, SK_TANK,
    SK_RAGE_STACK, SK_SHIELD, SK_LAST_HOPE, SK_ADRENALINE, SK_REFLECT, SK_FINISHER,
    SK_FROST_AURA, SK_FREEZE_HIT, SK_ICE_SHARDS, SK_TIME_STOP, SK_LEGEND,
    SK_MASTERY, SK_ECHO, SK_INVUL_BURST, SK_FROST_NOVA, SK_HOMING,
    SK_CRIT_NERFED, SK_QUAD_NERFED,
    SK_SPLT_NERFED, SK_FRZ_NERFED, SK_MAXHP_NERFED,
    SK__COUNT
};

}

#endif
