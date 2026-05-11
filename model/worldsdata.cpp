#include "worldsdata.h"

namespace Model {

void loadWorlds(QVector<WorldInfo> &out)
{
    out.clear();
    out.append({"Forge ardente",     "Ouverture - le Ver du magma",
                "fireworm", "Ver de Feu",            9,  90,  90, 8.f,  QColor("#ff6600")});
    out.append({"Catacombes",        "Le marechal des morts",
                "undead",   "Executeur des Morts",   5, 100, 100, 7.f,  QColor("#88aacc")});
    out.append({"Marais maudit",     "Le seigneur visqueux",
                "demonslime","Demon Vase",           6, 288, 160, 8.f,  QColor("#aa44ff")});
    out.append({"Arene de l'anneau", "Le titan a cornes",
                "mino",     "Minotaure",            16, 288, 160, 10.f, QColor("#cc8822")});
    out.append({"Citadelle gelee",   "Le Gardien du Givre - tu vas souffrir",
                "frostguardian","Gardien du Givre",  6, 192, 128, 8.f,  QColor("#44ccff")});
    out.append({"Abysses du Tyran",  "Lord Malificus - la Pierre attend d'etre reforgee",
                "malificus", "Lord Malificus",       1, 64, 96, 6.f,  QColor("#bb44ff")});
}

}
