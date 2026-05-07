#ifndef MODEL_SAVEMANAGER_H
#define MODEL_SAVEMANAGER_H

namespace Model {
struct GameModel;
void loadSettings(GameModel &m);
void saveSettings(const GameModel &m);
}

#endif
