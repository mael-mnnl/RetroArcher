#include "cheatsystem.h"
#include "../model/gamemodel.h"
#include "../model/savemanager.h"

namespace Controller { namespace Cheat {

void tryValidate(Model::GameModel &m)
{
    if (m.cheatInput == "123") {
        m.cheatInvincible = true;
        m.cheatFlashTimer = 2.5f;
    } else if (m.cheatInput == "0") {
        m.cheatInvincible = false;
        m.cheatFlashTimer = 1.5f;
    } else if (m.cheatInput == "999") {
        m.fragments      = 5;
        m.worldsUnlocked = 5;
        Model::saveSettings(m);
        m.cheatFlashTimer = 2.5f;
    }
    m.cheatInput.clear();
}

}}
