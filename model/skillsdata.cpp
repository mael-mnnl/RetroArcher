#include "skillsdata.h"
#include "types.h"

namespace Model {

void loadSkills(std::vector<Skill> &out)
{
    out.clear();
    auto add = [&](int id, int tier, const char *n, const char *d, const char *ic, const char *col){
        out.push_back({id, tier, QString::fromUtf8(n), QString::fromUtf8(d),
                       QString::fromUtf8(ic), QColor(col)});
    };

    // ─── World 1 (originaux) ───
    add(SK_DBL,1,"Double Flèche","2 flèches parallèles","x2","#44aaff");
    add(SK_TRI,1,"Triple Flèche","3 flèches en éventail (-15% dmg)","x3","#44ffbb");
    add(SK_FST,1,"Tir Rapide","Cadence +22%",">>","#ffff44");
    add(SK_PRC,1,"Perforant","Flèches traversent 1 ennemi","->","#ff44aa");
    add(SK_SPD,1,"Sprint","Vitesse +15%","~~","#88ff88");
    add(SK_POW,1,"Force","Dégâts +30%","!!","#ff8844");
    add(SK_BNC,1,"Ricochet","Flèches rebondissent x1","<>","#ffaaff");
    add(SK_DIA,1,"Oblique","Tir diagonal aussi","X","#cc44ff");
    add(SK_HARDCORE,1,"Hardcore","+1 PV max","+H","#ff6699");
    add(SK_DODGE,1,"Coup d'œil","6% chance d'esquive","??","#88ccff");
    add(SK_LIGHT_FOOT,1,"Pied léger","Vitesse +8%","..","#bbff99");
    add(SK_RANGE,1,"Portée","Flèches +30% rapides",">>","#ddee77");
    add(SK_KNOCKBACK,1,"Repousse","Flèches projettent",">!","#aabbcc");
    add(SK_BIG_ARROW,1,"Pointes","Flèches +20% dégâts, -8% cadence","[]","#cc9966");
    add(SK_HUNTER,1,"Chasseur","+25% dégâts si distance > 200","-O","#aaff77");
    add(SK_SHARP,1,"Aiguisage","+8% dégâts","/'","#ffddaa");
    add(SK_FIRST_HIT,1,"Premier Coup","1re flèche d'une salve +50%","1!","#ffcc88");
    add(SK_FOCUSED,1,"Concentration","+25% dégâts si immobile 1s","[*","#9999ff");

    // ─── World 2 (originaux) ───
    add(SK_GRN,2,"Lance-grenade","Touche G - 50 dmg rayon 70","*","#ff6622");
    add(SK_LIFESTEAL,2,"Vol de vie","4% des dmg infligés en PV","Vh","#aa1144");
    add(SK_NECRO,2,"Nécromancie","12% chance esprit allié sur kill","sP","#7755aa");
    add(SK_DEATH_MARK,2,"Marque","Tuer = prochain ennemi +30%","Mk","#aa4477");
    add(SK_BONE_SHIELD,2,"Bouclier osseux","+1 PV regen / 8s en combat","[]","#ccccaa");
    add(SK_BLOOD_PACT,2,"Pacte de sang","+25% dégâts mais -1 PV max","BP","#dd0033");
    add(SK_REGEN,2,"Régénération","+1 PV / 5s","RG","#66dd66");
    add(SK_RESILIENCE,2,"Résilience","Invincibilité +50%","Rs","#ddddaa");
    add(SK_PHOENIX,2,"Phénix","Revit à 50% PV (1x/run)","Ph","#ff7733");
    add(SK_SOUL_DRAIN,2,"Drain","+1 PV par kill (cd 5s)","sD","#cc66cc");
    add(SK_DECAY,2,"Pourriture","Ennemis à 60px perdent 1 PV/s","D-","#778822");
    add(SK_VAMP_NERFED,2,"Vampirisme","12% chance soin sur kill","V+","#aa1166");
    add(SK_FROST_FATE,2,"Gel Renforcé","Durée de gel +50%","F+","#aaddff");

    // ─── World 3 (originaux) ───
    add(SK_POISON,3,"Poison","Flèches : 4 dmg/s pendant 3s","Pn","#88ff44");
    add(SK_BLAZE,3,"Embrasement","20% chance brûlure 3dmg/s 4s","Bz","#ff8822");
    add(SK_EXPLODE_KILL,3,"Boom!","Kills explosent 12 dmg / 45px","EX","#ff5500");
    add(SK_FIRE_TRAIL,3,"Traînée","Tu laisses du feu 4dmg/s","FT","#ff4400");
    add(SK_DOUBLE_GRENADE,3,"Bi-grenade","Lance 2 grenades à la fois","2*","#ff6622");
    add(SK_BIG_BOOM,3,"Big Boom","Explosions +35% rayon","BB","#ff7711");
    add(SK_VOLCANO,3,"Volcan","8% pluie de feu sur kill","Vc","#dd3300");
    add(SK_RECKLESS,3,"Imprudence","+40% dégâts mais +25% subis","Rk","#cc0000");
    add(SK_BURST_FIRE,3,"Rafale","25% chance double tir","Bf","#ffcc00");
    add(SK_INFERNO,3,"Inferno","Brûlés explosent en mourant","In","#ff5522");
    add(SK_STICKY,3,"Glu acide","Flèches ralentissent ennemis 1s","Gl","#88ff44");
    add(SK_GHOST_ARROW,3,"Flèche fantôme","Flèches traversent les murs","Gh","#aaaaff");

    // ─── World 4 (originaux) ───
    add(SK_DASH,4,"Dash","Touche SHIFT - immune 0.4s, cd 3s","->","#00ddff");
    add(SK_RAM,4,"Bélier","Le dash inflige 25 dmg","BR","#ddaa44");
    add(SK_BERSERKER,4,"Berserker","+50% dégâts sous 35% PV","BS","#cc1133");
    add(SK_HEAVY,4,"Lourd","+45% dégâts, -18% cadence","HV","#996644");
    add(SK_THORNS,4,"Épines","50% des dmg subis renvoyés au contact","Tn","#557722");
    add(SK_TANK,4,"Tank","+3 PV max, -10% vitesse","Tk","#777799");
    add(SK_RAGE_STACK,4,"Rage","+5% dégâts par dmg subi (max 50%)","Rg","#dd2244");
    add(SK_SHIELD,4,"Bouclier","Bloque 1 coup, cd 12s","Sh","#aabbdd");
    add(SK_LAST_HOPE,4,"Dernière chance","0 PV : 50% chance survie à 1 PV","Lh","#dd9933");
    add(SK_ADRENALINE,4,"Adrénaline","+30% vitesse 3s après dmg","Ad","#dd44aa");
    add(SK_REFLECT,4,"Reflet","20% chance de renvoyer un tir","Rf","#aaccee");
    add(SK_FINISHER,4,"Acheveur","+35% sur ennemis < 30% PV","Fn","#aa3322");

    // ─── World 5 (originaux) ───
    add(SK_FROST_AURA,5,"Aura glaciale","Aura 80px ralentit ennemis 30%","FA","#88ccff");
    add(SK_FREEZE_HIT,5,"Givre total","12% chance gel complet 1s","FH","#aaeeff");
    add(SK_ICE_SHARDS,5,"Éclats de glace","20% chance flèche -> 4 shards","IS","#bbeeff");
    add(SK_TIME_STOP,5,"Arrêt du temps","Touche T : ralenti 4s (1x/run)","TS","#7799ff");
    add(SK_LEGEND,5,"Légende","+1 à toutes les stats par boss tué","Lg","#ffd700");
    add(SK_MASTERY,5,"Maîtrise","Tous les autres skills +18%","Ma","#ffcc44");
    add(SK_ECHO,5,"Écho","18% chance flèche tirée 2x","Ec","#bbaaff");
    add(SK_INVUL_BURST,5,"Aegis","4s invul à l'entrée d'une salle boss","Ag","#dddd88");
    add(SK_FROST_NOVA,5,"Frost Nova","Tous les 5 kills, gèle tous 2s","Fv","#aaddff");
    add(SK_HOMING,5,"Tête-chercheuse","Flèches incurvent vers cible","H>","#bb88ff");
    add(SK_CRIT_NERFED,5,"Critique","18% chance x1.7 dégâts","C!","#ffdd00");
    add(SK_QUAD_NERFED,5,"Quadruple","4 flèches parallèles -25% dmg","x4","#ff77ff");

    add(SK_SPLT_NERFED,3,"Éclats","Flèches éclatent en 2 (-65% dmg)","<*","#66ddff");
    add(SK_FRZ_NERFED,2,"Glaçant","Ralentit 1s sur hit","*~","#88ccff");
    add(SK_MAXHP_NERFED,2,"Vitalité","+1 PV max + soin total","++","#ff3366");

    // ─── 50 NOUVEAUX SKILLS ───

    // Tier 1 — Arcs et attaques de base
    add(SK_CHAIN_ARROW,1,"Flèche en Chaîne","Enchaîne sur 1 ennemi voisin (70% dmg)","~~","#44ddff");
    add(SK_RAPID_RELOAD,1,"Recharge Rapide","Cadence +20%","R>","#ffff88");
    add(SK_HEAVY_ARROW,1,"Flèche Lourde","+35% dégâts, -12% vitesse","=>","#cc9944");
    add(SK_MULTISHOT,1,"Multiflèche","5 flèches en éventail (-30% dmg)","x5","#88ffdd");
    add(SK_LUCKY_SHOT,1,"Tir Chanceux","Chaque 7e tir fait x2 dégâts","7!","#ffdd44");
    add(SK_ACCELERATE,1,"Accélération","Vitesse croît jusqu'à +30% en courant","~>","#bbff44");

    // Tier 2 — Magie légère et défense
    add(SK_ELECTRIC_HIT,2,"Choc Électrique","15% chance d'éclair sur ennemi voisin","E~","#ffff00");
    add(SK_REGEN_BURST,2,"Éclat vital","Kill → +0.5 PV (cd 3s)","V+","#66ff88");
    add(SK_STONE_SKIN,2,"Peau de pierre","-1 dégâts reçus (min 0.5)","[S","#aaaaaa");
    add(SK_ABSORB,2,"Absorption","15% chance d'absorber un coup","Ab","#88bbff");
    add(SK_LIFE_TAP,2,"Toucher vital","-0.15 PV/tir → +60% dégâts","LT","#ff4488");
    add(SK_SCATTERSHOT,2,"Tir dispersé","30% chance : 3 mini-flèches à l'impact","**","#66ddff");
    add(SK_WARDING,2,"Gardien","+1 PV en entrant dans une nouvelle salle","Wd","#44ff88");
    add(SK_ACID_ARROWS,2,"Flèches Acides","Corrodent : -15% def ennemi (stack x3)","Ac","#aaff00");

    // Tier 3 — Mobilité et combats avancés
    add(SK_SHADOW_DASH,3,"Dash Fantôme","Le dash laisse un clone distrayant 2s","SD","#8888ff");
    add(SK_DOUBLE_DASH,3,"Double Dash","2 charges de dash","DD","#00eeff");
    add(SK_EXECUTE,3,"Exécution","Kill instantané si ennemi < 8% PV","XX","#ff2200");
    add(SK_OVERLOAD,3,"Surcharge","Après 8s sans dégâts, prochain tir x5","O!","#ffaa00");
    add(SK_FRENZY,3,"Frénésie","5 kills en 10s → +50% dmg pendant 5s","F!","#ff5555");
    add(SK_BLOOD_ARROWS,3,"Flèches de sang","-0.15 PV/tir → +60% dégâts","B>","#cc0044");
    add(SK_EARTHQUAKE,3,"Séisme","Tous les 8 kills : stun + repousse tous","EQ","#886622");
    add(SK_GLASS_CANNON,3,"Canon de verre","+80% dégâts, -2 PV max","GC","#ff0044");
    add(SK_THUNDER_ARROW,3,"Tonnerre","25% chance éclair qui enchaîne","T~","#ffee00");
    add(SK_MAGMA,3,"Magma","Ennemis brûlés laissent une flaque de lave","Mg","#ff3300");
    add(SK_FREEZE_NOVA_KILL,3,"Nova de Glace","Kill sur gelé → explosion 20 dmg","Nv","#aaddff");
    add(SK_ACID_BURN,3,"Brûlure acide","Poison+brûlure actifs → +50% prochain hit","AB","#88ff00");

    // Tier 4 — Dash et mêlée avancés
    add(SK_SPECTRAL_DASH,4,"Dash Spectral","Après dash : invulnérable 1s de plus","S>","#aaaaff");
    add(SK_DIVE_DASH,4,"Dash Plongeant","Dash traverse et étourdit les ennemis 0.8s","Dv","#00ccff");
    add(SK_IRON_WILL,4,"Volonté de fer","<30% PV → -40% dégâts reçus","IW","#778877");
    add(SK_BARRIER_ROOM,4,"Barrière","1x/salle : bloque automatiquement 1 attaque","Ba","#ddddff");
    add(SK_WARCRY,4,"Cri de guerre","Entrée salle : étourdit tous les ennemis 0.8s","WC","#ffaa44");
    add(SK_REAPER_STACKS,4,"Moissonneur","Kill → +1% dégâts (max +40%, reset salle)","Rs","#cc2266");
    add(SK_MOMENTUM,4,"Élan","+3% dégâts par 100px parcourus (reset sur coup)","Mo","#44ffaa");
    add(SK_VOID_STEP,4,"Pas du Vide","Phase à travers ennemis 2s (Maj+T, cd 20s)","Vd","#6644ff");
    add(SK_ARCANE_BOOST,4,"Amplification","Dégâts de sorts +40%","AM","#cc44ff");
    add(SK_COOLDOWN_MASTERY,4,"Maîtrise CD","Cooldowns des sorts -25%","CD","#88aaff");

    // Tier 5 — Sorts et puissance ultime
    add(SK_SPELL_ECHO,5,"Écho de Sort","20% chance qu'un sort se déclenche 2x","SE","#ff88ff");
    add(SK_ELEMENTAL_FURY,5,"Furie élémentaire","Après un sort : +30% flèches 4s","EF","#ff8833");
    add(SK_RICOCHET_PLUS,5,"Super Ricochet","Flèches rebondissent x3","R+","#ffaaff");
    add(SK_STAR_POWER,5,"Puissance Stellaire","Après 200 kills : +20% tout","SP","#ffffaa");
    add(SK_LEGEND_HUNTER,5,"Chasseur légendaire","+5% dégâts par monde complété ce run","LH","#ffdd88");
    add(SK_SOUL_STONE,5,"Pierre d'âme","Conserve 1 skill aléatoire à la mort (1x)","SS","#bb88ff");
    add(SK_MANA_SHIELD,5,"Bouclier de mana","Sorts coûtent 1 PV mais font x3 dégâts","MS","#4488ff");
    add(SK_AWAKENING,5,"Éveil","Chaque salle : buff +10% aléatoire 30s","Ev","#ffcc88");
    add(SK_PERSEVERANCE,5,"Persévérance","+10% tout par boss vivant ce run","Pv","#88ffcc");
    add(SK_VAMPIRE_AURA,5,"Aura vampirique","Drain 1 PV/s aux ennemis dans 60px","VA","#cc1166");
}

}
