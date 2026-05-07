#ifndef VIEW_GAMEVIEW_H
#define VIEW_GAMEVIEW_H

#include "../model/types.h"
#include <QImage>

class QPainter;
namespace Model { struct GameModel; struct Enemy; }

namespace View {
class AssetManager;

class GameView {
public:
    explicit GameView(const AssetManager &assets);
    void render(QPainter &p, const Model::GameModel &m);

private:
    void drawRoom(QPainter &p, const Model::GameModel &m);
    void drawHUD(QPainter &p, const Model::GameModel &m);
    void drawProgressionBar(QPainter &p, const Model::GameModel &m, int x, int y, int w);
    void drawSpriteAt(QPainter &p, const QImage &sheet, int frameCount, float fps,
                      float x, float y, float scale, bool flipX, float timer);
    void drawBossInGame(QPainter &p, const Model::GameModel &m, const Model::Enemy &boss, float scaleMul);
    void drawDialogBubble(QPainter &p, const Model::GameModel &m);
    void drawTutorialOverlay(QPainter &p, const Model::GameModel &m);

    const AssetManager &m_assets;
};
}

#endif
