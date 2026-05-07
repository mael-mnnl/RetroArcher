#ifndef VIEW_MENUVIEW_H
#define VIEW_MENUVIEW_H

#include <QImage>

class QPainter;
namespace Model { struct GameModel; }

namespace View {
class AssetManager;

class MenuView {
public:
    explicit MenuView(const AssetManager &assets);

    void drawMenu        (QPainter &p, const Model::GameModel &m);
    void drawSkinSelect  (QPainter &p, const Model::GameModel &m);
    void drawDiscoveries (QPainter &p, const Model::GameModel &m);
    void drawLore        (QPainter &p, const Model::GameModel &m);
    void drawSkillSelect (QPainter &p, const Model::GameModel &m);
    void drawEndScreen   (QPainter &p, const Model::GameModel &m, bool win);

private:
    void drawSpriteAt(QPainter &p, const QImage &sheet, int frameCount, float fps,
                      float x, float y, float scale, bool flipX, float timer);
    const AssetManager &m_assets;
};
}

#endif
