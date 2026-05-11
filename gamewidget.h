#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include "model/gamemodel.h"
#include "view/assetmanager.h"
#include "view/gameview.h"
#include "view/menuview.h"
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>

class GameWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

private slots:
    void tick();

private:
    void updateGame(float dt);
    void checkDoorTransition();
    void onBossDefeated();
    void launchSelection();

    View::AssetManager m_assets;
    Model::GameModel   m_model;
    View::GameView     m_gameView;
    View::MenuView     m_menuView;

    QTimer        m_timer;
    QElapsedTimer m_elapsed;
    qint64        m_lastTickNs = 0;

    // UI hovers
    int m_classHover         = 0;
    int m_relicHover         = 0;
    int m_shopHover          = 0;
    int m_forgeHover         = 0;
    int m_blessingHover      = 0;
    int m_blackMarketHover   = 0;
};

#endif
