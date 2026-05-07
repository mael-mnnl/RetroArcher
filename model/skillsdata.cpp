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

    // ─── World 1 ───
    add(SK_DBL,1,"Double Fleche","2 fleches paralleles","x2","#44aaff");
    add(SK_TRI,1,"Triple Fleche","3 fleches en eventail (-15% dmg)","x3","#44ffbb");
    add(SK_FST,1,"Tir Rapide","Cadence +22%",">>","#ffff44");
    add(SK_PRC,1,"Perforant","Fleches traversent 1 ennemi","->","#ff44aa");
    add(SK_SPD,1,"Sprint","Vitesse +15%","~~","#88ff88");
    add(SK_POW,1,"Force","Degats +30%","!!","#ff8844");
    add(SK_BNC,1,"Ricochet","Fleches rebondissent x1","<>","#ffaaff");
    add(SK_DIA,1,"Oblique","Tir diagonal aussi","X","#cc44ff");
    add(SK_HARDCORE,1,"Hardcore","+1 PV max","+H","#ff6699");
    add(SK_DODGE,1,"Coup d'oeil","6% chance d'esquive","??","#88ccff");
    add(SK_LIGHT_FOOT,1,"Pied leger","Vitesse +8%","..","#bbff99");
    add(SK_RANGE,1,"Portee","Fleches +30% rapides",">>","#ddee77");
    add(SK_KNOCKBACK,1,"Repousse","Fleches projettent",">!","#aabbcc");
    add(SK_BIG_ARROW,1,"Pointes","Fleches +20% degats, -8% cadence","[]","#cc9966");
    add(SK_HUNTER,1,"Chasseur","+25% degats si distance > 200","-O","#aaff77");
    add(SK_SHARP,1,"Aiguisage","+8% degats","/'","#ffddaa");
    add(SK_FIRST_HIT,1,"Premier Coup","1ere fleche d'1 salve +50%","1!","#ffcc88");
    add(SK_FOCUSED,1,"Concentration","+25% degats si immobile 1s","[*","#9999ff");

    // ─── World 2 ───
    add(SK_GRN,2,"Lance-grenade","Touche G - 50 dmg radius 70","*","#ff6622");
    add(SK_LIFESTEAL,2,"Vol de vie","4% des dmg infliges en PV","Vh","#aa1144");
    add(SK_NECRO,2,"Necromancie","12% chance esprit allie sur kill","sP","#7755aa");
    add(SK_DEATH_MARK,2,"Marque","Tuer = prochain ennemi +30%","Mk","#aa4477");
    add(SK_BONE_SHIELD,2,"Bouclier osseux","+1 PV regen / 8s en combat","[]","#ccccaa");
    add(SK_BLOOD_PACT,2,"Pacte de sang","+25% degats mais -1 PV max","BP","#dd0033");
    add(SK_REGEN,2,"Regeneration","+1 PV / 5s","RG","#66dd66");
    add(SK_RESILIENCE,2,"Resilience","Invincibilite +50%","Rs","#ddddaa");
    add(SK_PHOENIX,2,"Phenix","Revive 50% PV (1x/run)","Ph","#ff7733");
    add(SK_SOUL_DRAIN,2,"Drain","+1 PV par kill (cd 5s)","sD","#cc66cc");
    add(SK_DECAY,2,"Pourriture","Ennemis a 60px perdent 1 PV/s","D-","#778822");
    add(SK_VAMP_NERFED,2,"Vampirisme","12% chance soin sur kill","V+","#aa1166");
    add(SK_FROST_FATE,2,"Gel Renforce","Duree de gel +50%","F+","#aaddff");

    // ─── World 3 ───
    add(SK_POISON,3,"Poison","Fleches : 4 dmg/s pendant 3s","Pn","#88ff44");
    add(SK_BLAZE,3,"Embrasement","20% chance brulure 3dmg/s 4s","Bz","#ff8822");
    add(SK_EXPLODE_KILL,3,"Boom!","Kills explosent 12 dmg / 45px","EX","#ff5500");
    add(SK_FIRE_TRAIL,3,"Trainee","Tu laisses du feu 4dmg/s","FT","#ff4400");
    add(SK_DOUBLE_GRENADE,3,"Bi-grenade","Lance 2 grenades a la fois","2*","#ff6622");
    add(SK_BIG_BOOM,3,"Big Boom","Explosions +35% rayon","BB","#ff7711");
    add(SK_VOLCANO,3,"Volcan","8% pluie de feu sur kill","Vc","#dd3300");
    add(SK_RECKLESS,3,"Imprudence","+40% degats mais +25% subis","Rk","#cc0000");
    add(SK_BURST_FIRE,3,"Rafale","25% chance double tir","Bf","#ffcc00");
    add(SK_INFERNO,3,"Inferno","Brules explosent en mourant","In","#ff5522");
    add(SK_STICKY,3,"Glu acide","Fleches ralentissent ennemis 1s","Gl","#88ff44");
    add(SK_GHOST_ARROW,3,"Fleche fantome","Fleches traversent les murs","Gh","#aaaaff");

    // ─── World 4 ───
    add(SK_DASH,4,"Dash","Touche SHIFT - immune 0.4s, cd 3s","->","#00ddff");
    add(SK_RAM,4,"Belier","Le dash inflige 25 dmg","BR","#ddaa44");
    add(SK_BERSERKER,4,"Berserker","+50% degats sous 35% PV","BS","#cc1133");
    add(SK_HEAVY,4,"Lourd","+45% degats, -18% cadence","HV","#996644");
    add(SK_THORNS,4,"Epines","50% des dmg subis renvoyes au contact","Tn","#557722");
    add(SK_TANK,4,"Tank","+3 PV max, -10% vitesse","Tk","#777799");
    add(SK_RAGE_STACK,4,"Rage","+5% degats par dmg subi (max 50%)","Rg","#dd2244");
    add(SK_SHIELD,4,"Bouclier","Bloque 1 coup, cd 12s","Sh","#aabbdd");
    add(SK_LAST_HOPE,4,"Derniere chance","0 PV : 50% chance de survie a 1 PV","Lh","#dd9933");
    add(SK_ADRENALINE,4,"Adrenaline","+30% vitesse 3s apres dmg","Ad","#dd44aa");
    add(SK_REFLECT,4,"Reflet","20% chance de renvoyer un tir","Rf","#aaccee");
    add(SK_FINISHER,4,"Acheveur","+35% sur ennemis < 30% PV","Fn","#aa3322");

    // ─── World 5 ───
    add(SK_FROST_AURA,5,"Aura glaciale","Aura 80px ralentit ennemis 30%","FA","#88ccff");
    add(SK_FREEZE_HIT,5,"Givre total","12% chance gel complet 1s","FH","#aaeeff");
    add(SK_ICE_SHARDS,5,"Eclats de glace","20% chance fleche -> 4 shards","IS","#bbeeff");
    add(SK_TIME_STOP,5,"Arret du temps","Touche T : ralenti 4s (1x/run)","TS","#7799ff");
    add(SK_LEGEND,5,"Legende","+1 a toutes les stats par boss tue","Lg","#ffd700");
    add(SK_MASTERY,5,"Maitrise","Tous les autres skills +18%","Ma","#ffcc44");
    add(SK_ECHO,5,"Echo","18% chance fleche tire 2x","Ec","#bbaaff");
    add(SK_INVUL_BURST,5,"Aegis","4s invul a l'entree d'une salle de boss","Ag","#dddd88");
    add(SK_FROST_NOVA,5,"Frost Nova","Tous les 5 kills, gele tous 2s","Fn","#aaddff");
    add(SK_HOMING,5,"Tete-chercheuse","Fleches incurvent vers cible","H>","#bb88ff");
    add(SK_CRIT_NERFED,5,"Critique","18% chance x1.7 degats","C!","#ffdd00");
    add(SK_QUAD_NERFED,5,"Quadruple","4 fleches paralleles -25% dmg","x4","#ff77ff");

    add(SK_SPLT_NERFED,3,"Eclats","Fleches eclatent en 2 (-65% dmg)","<*","#66ddff");
    add(SK_FRZ_NERFED,2,"Glacant","Ralentit 1s sur hit","*~","#88ccff");
    add(SK_MAXHP_NERFED,2,"Vitalite","+1 PV max + soin total","++","#ff3366");
}

}
