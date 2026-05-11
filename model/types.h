#ifndef MODEL_TYPES_H
#define MODEL_TYPES_H

namespace Model {

enum EnemyType {
    ET_None, ET_Slime, ET_Skel, ET_Bat, ET_Brute, ET_Mage,
    ET_Minion, ET_MiniBoss, ET_FinalBoss,
    ET_Elite,
    ET_TrueFinalBoss
};

enum AnimType { AN_Idle, AN_Walk, AN_Atk, AN_Hurt, AN_Death };

enum GameState {
    GS_Menu, GS_SkinSelect, GS_Discoveries, GS_Lore,
    GS_Tutorial,
    GS_ClassSelect,
    GS_RelicSelect,
    GS_Playing, GS_RoomCleared, GS_FadeOut, GS_SkillSelect,
    GS_CurseSelect,
    GS_Shop, GS_Forge, GS_Challenge,
    GS_FadeIn,
    GS_GameOver, GS_Victory,
    GS_Blessings,
    GS_Leaderboard,
    GS_Paused,       // menu pause (Echap en jeu)
    GS_BlackMarket   // marche noir - achat de sorts
};

enum RoomType {
    RT_Normal, RT_Boss, RT_Elite, RT_Shop, RT_Forge, RT_Challenge, RT_BlackMarket
};

enum CharClass {
    CC_Hero = 0,
    CC_Hunter = 1,
    CC_Paladin = 2,
    CC_Alchemist = 3
};

enum PickupType {
    PU_Gold, PU_Potion, PU_Scroll, PU_Heart, PU_Chest, PU_EquipDrop
};

enum CurseId {
    CRS_FastEnemies, CRS_PiercingShots, CRS_FragileGlass,
    CRS_NoRegen, CRS_SlowDash, CRS_BloodTax,
    CRS__COUNT
};

enum RelicId {
    REL_BloodOrb,
    REL_BrokenWatch,
    REL_SoulMirror,
    REL_GoldenSeed,
    REL_PhoenixTear,
    REL_VoidPact,
    REL__COUNT
};

// Sorts actifs (achetés au marché noir, slots 1-4)
enum SpellId {
    SP_Teleport = 0,   // téléporte 200px en direction de visée
    SP_Lightning,      // éclair en chaîne sur 3 ennemis proches
    SP_Invisibility,   // invisible + vitesse 3s
    SP_Repulsion,      // repousse tous les ennemis dans 110px
    SP_Fireball,       // boule de feu en direction de visée
    SP_Blizzard,       // gèle tous les ennemis visibles 2s
    SP_Laser,          // rayon laser perforant dans la direction
    SP_ShieldWall,     // invincibilité totale 2s
    SP__COUNT
};

// Type de pièce d'équipement
enum EquipType {
    EQ_Weapon = 0,
    EQ_Armor,
    EQ_Accessory,
    EQ__COUNT
};

enum EquipRarity {
    ER_Common = 0,
    ER_Rare,
    ER_Epic
};

// Skill IDs - 65 originaux + 50 nouveaux
enum SkillId {
    // ── World 1 (original) ──
    SK_DBL=0, SK_TRI, SK_FST, SK_PRC, SK_SPD, SK_POW, SK_BNC, SK_DIA,
    SK_HARDCORE, SK_DODGE, SK_LIGHT_FOOT, SK_RANGE, SK_KNOCKBACK, SK_BIG_ARROW,
    SK_HUNTER, SK_SHARP, SK_FIRST_HIT, SK_FOCUSED,
    // ── World 2 (original) ──
    SK_GRN,
    SK_LIFESTEAL, SK_NECRO, SK_DEATH_MARK, SK_BONE_SHIELD, SK_BLOOD_PACT,
    SK_REGEN, SK_RESILIENCE, SK_PHOENIX, SK_SOUL_DRAIN, SK_DECAY,
    SK_VAMP_NERFED, SK_FROST_FATE,
    // ── World 3 (original) ──
    SK_POISON, SK_BLAZE, SK_EXPLODE_KILL, SK_FIRE_TRAIL, SK_DOUBLE_GRENADE,
    SK_BIG_BOOM, SK_VOLCANO, SK_RECKLESS, SK_BURST_FIRE, SK_INFERNO,
    SK_STICKY, SK_GHOST_ARROW,
    // ── World 4 (original) ──
    SK_DASH, SK_RAM, SK_BERSERKER, SK_HEAVY, SK_THORNS, SK_TANK,
    SK_RAGE_STACK, SK_SHIELD, SK_LAST_HOPE, SK_ADRENALINE, SK_REFLECT, SK_FINISHER,
    // ── World 5 (original) ──
    SK_FROST_AURA, SK_FREEZE_HIT, SK_ICE_SHARDS, SK_TIME_STOP, SK_LEGEND,
    SK_MASTERY, SK_ECHO, SK_INVUL_BURST, SK_FROST_NOVA, SK_HOMING,
    SK_CRIT_NERFED, SK_QUAD_NERFED,
    SK_SPLT_NERFED, SK_FRZ_NERFED, SK_MAXHP_NERFED,

    // ── 50 nouveaux skills ──
    // Tier 1
    SK_CHAIN_ARROW,      // flèche enchaîne sur 1 ennemi voisin (70% dmg)
    SK_RAPID_RELOAD,     // cadence +20%
    SK_HEAVY_ARROW,      // +35% dmg, -12% vitesse
    SK_MULTISHOT,        // 5 flèches en éventail (-30% dmg)
    SK_LUCKY_SHOT,       // chaque 7e tir fait x2 dmg
    SK_ACCELERATE,       // vitesse augmente jusqu'à +30% en courant
    // Tier 2
    SK_ELECTRIC_HIT,     // 15% chance d'éclair sur 1 ennemi voisin (15 dmg)
    SK_REGEN_BURST,      // kill donne +0.5 PV (cd 3s)
    SK_STONE_SKIN,       // -1 dmg reçus (min 0.5)
    SK_ABSORB,           // 15% chance d'absorber un coup en PV temp
    SK_LIFE_TAP,         // perd 0.15 PV par tir → +60% dmg
    SK_SCATTERSHOT,      // 30% chance d'éclater en 3 mini-flèches à l'impact
    SK_WARDING,          // +1 PV en entrant dans une salle
    SK_ACID_ARROWS,      // flèches corrodent : -15% def sur 3s (stack x3)
    // Tier 3
    SK_SHADOW_DASH,      // le dash laisse un clone qui distrait les ennemis 2s
    SK_DOUBLE_DASH,      // 2 charges de dash
    SK_EXECUTE,          // kill instantané si ennemi < 8% PV
    SK_OVERLOAD,         // après 8s sans dégâts, prochain tir x5
    SK_FRENZY,           // 5 kills en 10s → +50% dmg 5s
    SK_BLOOD_ARROWS,     // coûte 0.15 PV par tir → +60% dmg
    SK_EARTHQUAKE,       // tous les 8 kills : stun + repousse tous
    SK_GLASS_CANNON,     // +80% dmg, -2 PV max
    SK_THUNDER_ARROW,    // 25% chance d'éclair qui enchaîne
    SK_MAGMA,            // ennemis en feu laissent une flaque de lave
    SK_FREEZE_NOVA_KILL, // kill sur ennemi gelé → explosion de glace 20 dmg
    SK_ACID_BURN,        // poison+brûlure actifs → +50% dmg prochain hit
    // Tier 4
    SK_SPECTRAL_DASH,    // après dash : invulnérable 1s de plus
    SK_DIVE_DASH,        // dash traverse et étourdit les ennemis 0.8s
    SK_IRON_WILL,        // <30% PV → -40% dégâts reçus
    SK_BARRIER_ROOM,     // 1x par salle : bloque automatiquement 1 attaque
    SK_WARCRY,           // entrée de salle : étourdit tous les ennemis 0.8s
    SK_REAPER_STACKS,    // chaque kill +1% dmg (max +40%, reset salle)
    SK_MOMENTUM,         // +3% dmg par 100px parcourus (reset sur coup)
    SK_VOID_STEP,        // phase à travers ennemis 2s (cd 20s) avec Maj+T
    SK_ARCANE_BOOST,     // dégâts de sorts +40%
    SK_COOLDOWN_MASTERY, // cd des sorts -25%
    // Tier 5
    SK_SPELL_ECHO,       // 20% chance qu'un sort se déclenche 2x
    SK_ELEMENTAL_FURY,   // après un sort : +30% dégâts flèches 4s
    SK_RICOCHET_PLUS,    // flèches rebondissent x3 (améliore SK_BNC)
    SK_STAR_POWER,       // après 200 kills dans un run : +20% tout
    SK_LEGEND_HUNTER,    // +5% dmg par monde complété ce run
    SK_SOUL_STONE,       // conserve 1 skill aléatoire à la mort (1x/run)
    SK_MANA_SHIELD,      // sorts coûtent 1 PV mais font x3 dmg
    SK_AWAKENING,        // chaque salle : buff aléatoire de 10% pendant 30s
    SK_PERSEVERANCE,     // +10% toutes stats par boss vivant ce run
    SK_VAMPIRE_AURA,     // drain 1 PV/s aux ennemis dans 60px autour

    SK__COUNT
};

}

#endif
