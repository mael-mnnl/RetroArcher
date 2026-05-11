#include "contentdata.h"
#include "gamemodel.h"
#include "types.h"

namespace Model {

void loadCurses(GameModel &m)
{
    m.allCurses.clear();
    auto add = [&](int id, const char *n, const char *d, const char *col){
        m.allCurses.push_back({id, QString::fromUtf8(n), QString::fromUtf8(d), QColor(col)});
    };
    add(CRS_FastEnemies,    "Frenesie",       "Les ennemis sont 20% plus rapides", "#ff6644");
    add(CRS_PiercingShots,  "Eclats fragmentes","Les tirs ennemis traversent les murs","#aa66ff");
    add(CRS_FragileGlass,   "Verre fragile",  "Tu prends 1 PV de plus par coup",   "#ff44aa");
    add(CRS_NoRegen,        "Mort lente",     "Plus de regeneration entre salles", "#888888");
    add(CRS_SlowDash,       "Pieds de plomb", "Le dash recharge 2x plus lentement","#ccaa44");
    add(CRS_BloodTax,       "Impot du sang",  "Perds 1 PV a chaque nouvelle salle","#cc2244");
}

void loadRelics(GameModel &m)
{
    m.allRelics.clear();
    auto add = [&](int id, const char *n, const char *d, const char *col){
        m.allRelics.push_back({id, QString::fromUtf8(n), QString::fromUtf8(d), QColor(col)});
    };
    add(REL_BloodOrb,    "Orbe de Sang",     "+1 PV tous les 2 kills mais -1 skill a la mort","#cc2244");
    add(REL_BrokenWatch, "Montre Brisee",    "Time Stop recharge chaque salle mais pas de skills","#aaccff");
    add(REL_SoulMirror,  "Miroir des Ames",  "Tes balles rebondissent mais peuvent te toucher","#88ccff");
    add(REL_GoldenSeed,  "Graine d'Or",      "Gains d'or +50% mais -1 PV max","#ffcc44");
    add(REL_PhoenixTear, "Larme du Phenix",  "1 resurrection gratuite a pleine vie (1x/run)","#ff7733");
    add(REL_VoidPact,    "Pacte du Vide",    "+30% degats infliges mais ennemis +20% PV","#aa44ff");
}

void loadClasses(GameModel &m)
{
    m.allClasses.clear();
    // CC_Hero
    m.allClasses.push_back({CC_Hero, "Heros Inconnu",
        "Equilibre - le porteur originel",
        QColor("#ffd544"), 5.f, 155.f, 0.7f, 10.f, 0, -1});
    // CC_Hunter
    m.allClasses.push_back({CC_Hunter, "Chasseur",
        "Plus rapide et tir longue portee",
        QColor("#88ff88"), 4.f, 175.f, 0.55f, 9.f, 0, SK_RANGE});
    // CC_Paladin
    m.allClasses.push_back({CC_Paladin, "Paladin",
        "Resistant - commence avec un Bouclier",
        QColor("#aaccff"), 7.f, 135.f, 0.8f, 11.f, 0, SK_SHIELD});
    // CC_Alchemist
    m.allClasses.push_back({CC_Alchemist, "Alchimiste",
        "Commence avec 5 grenades et Poison",
        QColor("#bb88ff"), 5.f, 150.f, 0.7f, 9.f, 5, SK_POISON});
}

}
