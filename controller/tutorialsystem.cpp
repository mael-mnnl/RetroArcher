#include "tutorialsystem.h"
#include "dialogsystem.h"
#include "../model/gamemodel.h"
#include "../model/types.h"
#include "../core/constants.h"

namespace Controller { namespace Tutorial {

using namespace Model;
using namespace Core;

void buildRoom(GameModel &m, int step)
{
    m.enemies.clear(); m.bullets.clear();
    m.bossIndex = -1;
    for (int i=0; i<4; ++i) m.doorOpen[i] = false;
    m.player.invincibility = 1.5f;

    if (step == 0) {
        // Salle vide
    } else if (step == 1) {
        Enemy e; e.id = m.nextEnemyId++;
        e.x = GW*0.75f; e.y = GH*0.5f;
        e.type = ET_Slime; e.hp = e.maxHp = 12; e.speed = 30; e.size = 32;
        e.anim = AN_Walk;
        m.enemies.push_back(e);
    } else if (step == 2) {
        for (int i=0; i<2; ++i) {
            Enemy e; e.id = m.nextEnemyId++;
            e.x = GW*(0.6f+i*0.15f); e.y = GH*(0.3f+i*0.4f);
            e.type = ET_Slime; e.hp = e.maxHp = 18; e.speed = 40; e.size = 32;
            e.anim = AN_Walk;
            m.enemies.push_back(e);
        }
        Enemy s; s.id = m.nextEnemyId++;
        s.x = GW*0.85f; s.y = GH*0.5f;
        s.type = ET_Skel; s.hp = s.maxHp = 28; s.speed = 35; s.size = 32;
        s.shootCd = 2.f; s.anim = AN_Walk;
        m.enemies.push_back(s);
    } else if (step == 3) {
        Enemy boss;
        boss.id = m.nextEnemyId++;
        boss.x = GW/2.f; boss.y = GH*0.4f;
        boss.subType = 0;
        boss.hp = boss.maxHp = 100;
        boss.speed = 45; boss.damage = 1; boss.size = 60;
        boss.shootCd = 1.6f; boss.burstCd = 7;
        boss.chargeCd = 7.f;
        boss.type = ET_MiniBoss;
        boss.moveTargetX = boss.x; boss.moveTargetY = boss.y;
        m.enemies.push_back(boss);
        m.bossIndex = (int)m.enemies.size()-1;
    }
}

void start(GameModel &m)
{
    m.tutorialActive = true;
    m.tutStep = 0;
    m.tutAdvanceCd = 0;
    m.player = Player();
    m.player.skinIndex  = m.menuSelectedSkin;
    m.player.atkVariant = m.menuSelectedAtk;
    m.player.x = GW/2.f; m.player.y = GH/2.f;
    m.enemies.clear(); m.bullets.clear(); m.particles.clear();
    m.currentRoom = 1; m.bossIndex = -1;
    for (int i=0; i<4; ++i) m.doorOpen[i] = false;
    buildRoom(m, 0);
    m.state = GS_Playing;
    Dialog::schedule(m, "HEROS INCONNU",
        "Eveille-toi. Tu portes le Fragment initial. ZQSD pour bouger.",
        5.f, QColor("#ffd544"));
}

void update(GameModel &m, float dt)
{
    m.tutAdvanceCd -= dt;
    bool anyAlive = false;
    for (auto &e : m.enemies) if (!e.dead) { anyAlive = true; break; }

    if (m.tutStep == 0) {
        if (m.player.moving && m.tutAdvanceCd <= 0) {
            Dialog::schedule(m, "HEROS INCONNU",
                "Bien. Reste maintenant immobile : tu tires automatiquement vers les ennemis.",
                4.5f, QColor("#ffd544"));
            m.tutStep = 1; m.tutAdvanceCd = 1.5f;
            buildRoom(m, 1);
        }
    } else if (m.tutStep == 1) {
        if (!anyAlive && m.tutAdvanceCd <= 0) {
            Dialog::schedule(m, "HEROS INCONNU",
                "Tuer des ennemis te soigne parfois. Affronte ce groupe !",
                4.0f, QColor("#88ff88"));
            m.tutStep = 2; m.tutAdvanceCd = 1.5f;
            buildRoom(m, 2);
        }
    } else if (m.tutStep == 2) {
        if (!anyAlive && m.tutAdvanceCd <= 0) {
            Dialog::schedule(m, "HEROS INCONNU",
                "Voici un boss en miniature. Vaincs-le pour clore l'entrainement !",
                4.0f, QColor("#ff7733"));
            m.tutStep = 3; m.tutAdvanceCd = 1.5f;
            buildRoom(m, 3);
        }
    } else if (m.tutStep == 3) {
        if (!anyAlive && m.tutAdvanceCd <= 0) {
            Dialog::schedule(m, "HEROS INCONNU",
                "Felicitations Heros Inconnu. Tu maitrises l'essentiel. La quete commence !",
                5.0f, QColor("#ffd544"));
            m.tutStep = 4;
            m.tutAdvanceCd = 5.5f;
        }
    } else if (m.tutStep == 4) {
        if (m.tutAdvanceCd <= 0) {
            m.tutorialActive = false;
            m.state = GS_Menu;
        }
    }
}

}}
