#include "spellsdata.h"
#include "types.h"
#include <QRandomGenerator>

namespace Model {

void loadSpells(std::vector<SpellInfo> &out)
{
    out.clear();
    auto add = [&](SpellId id, const char *n, const char *d, const char *col, float cd, int cost) {
        out.push_back({id, QString::fromUtf8(n), QString::fromUtf8(d),
                       QColor(col), cd, cost});
    };
    add(SP_Teleport,    "Téléportation",   "Saute 200px dans la direction visée",       "#00eeff", 8.f,  60);
    add(SP_Lightning,   "Foudre",          "Éclair en chaîne sur 3 ennemis proches",    "#ffee00", 5.f,  50);
    add(SP_Invisibility,"Invisibilité",    "Invisible + vitesse +40% pendant 3s",       "#aaaaff", 15.f, 80);
    add(SP_Repulsion,   "Répulsion",       "Repousse tous les ennemis dans 120px",      "#ff8844", 6.f,  45);
    add(SP_Fireball,    "Boule de Feu",    "Grosse boule enflammée (80 dmg, r=60)",    "#ff4400", 7.f,  55);
    add(SP_Blizzard,    "Blizzard",        "Gèle tous les ennemis visibles 2.5s",       "#88ccff", 12.f, 75);
    add(SP_Laser,       "Rayon Laser",     "Rayon perforant instantané (60 dmg/ennemi)","#ff00ff", 6.f,  65);
    add(SP_ShieldWall,  "Mur de Bouclier", "Invincibilité totale pendant 2s",           "#ffd700", 20.f, 90);
}

// Génère un item d'équipement aléatoire selon type et tier du monde
EquipItem makeEquipItem(EquipType type, int rarity, int worldTier)
{
    EquipItem it;
    it.type    = type;
    it.rarity  = (EquipRarity)rarity;
    it.empty   = false;

    float tierMul = 1.f + worldTier * 0.15f;
    float rarMul  = (rarity == ER_Common) ? 1.f : (rarity == ER_Rare) ? 1.4f : 2.0f;

    if (type == EQ_Weapon) {
        static const char *names[3][3] = {
            {"Arc de bois","Arc court","Arc épée"},
            {"Arc d'acier","Arc composite","Arc de guerre"},
            {"Arc elfique","Baguette arcane","Arc des abysses"}
        };
        int idx = rarity < 3 ? rarity : 0;
        int ni  = QRandomGenerator::global()->bounded(3);
        it.name        = QString::fromUtf8(names[idx][ni]);
        it.dmgMul      = 1.f + 0.18f * rarMul * tierMul;
        it.fireRateMul = 1.f + 0.05f * rarMul;
        it.spellDmgMul = (rarity == ER_Epic) ? 1.30f : 1.f;
        it.color       = (rarity==ER_Common)?QColor("#aa7744"):(rarity==ER_Rare)?QColor("#4488ff"):QColor("#cc44ff");
        it.desc = QString("+%1% dgts").arg((int)((it.dmgMul-1.f)*100));
    } else if (type == EQ_Armor) {
        static const char *names[3][3] = {
            {"Armure de cuir","Manteau","Veste de combat"},
            {"Armure de fer","Cotte de mailles","Robe de mage"},
            {"Armure de titane","Robe des abysses","Aegis ancienne"}
        };
        int idx = rarity < 3 ? rarity : 0;
        int ni  = QRandomGenerator::global()->bounded(3);
        it.name       = QString::fromUtf8(names[idx][ni]);
        it.shieldMax  = (rarity == ER_Common) ? 1 : (rarity == ER_Rare) ? 2 : 3;
        it.shieldMax += (worldTier / 2);
        it.spellDmgMul= (rarity == ER_Epic) ? 1.5f : (rarity == ER_Rare) ? 1.2f : 1.0f;
        it.color      = (rarity==ER_Common)?QColor("#886644"):(rarity==ER_Rare)?QColor("#5566aa"):QColor("#8844cc");
        it.desc = QString("Bouclier %1 | sorts +%2%").arg(it.shieldMax).arg((int)((it.spellDmgMul-1.f)*100));
    } else { // EQ_Accessory
        static const char *names[3][3] = {
            {"Anneau de force","Amulette simple","Bague de vitesse"},
            {"Saphir du mage","Rubis de combat","Émeraude du vent"},
            {"Cristal de mana","Pendant de vide","Couronne obscure"}
        };
        int idx = rarity < 3 ? rarity : 0;
        int ni  = QRandomGenerator::global()->bounded(3);
        it.name       = QString::fromUtf8(names[idx][ni]);
        it.dmgMul     = 1.f + 0.08f * rarMul;
        it.speedMul   = 1.f + 0.08f * rarMul;
        it.spellDmgMul= 1.f + 0.10f * rarMul;
        it.goldBonus  = rarity;
        it.color      = (rarity==ER_Common)?QColor("#44aa44"):(rarity==ER_Rare)?QColor("#ffdd44"):QColor("#ff88ff");
        it.desc = QString("+%1% dgts/vitesse").arg((int)((it.dmgMul-1.f)*100));
    }
    return it;
}

}
