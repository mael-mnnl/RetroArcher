#ifndef CONTROLLER_SCORESYSTEM_H
#define CONTROLLER_SCORESYSTEM_H
class QString;
class QColor;
namespace Model { struct GameModel; }
namespace Controller { namespace Score {
void onKill(Model::GameModel &m);
void popup(Model::GameModel &m, float x, float y, const QString &text, const QColor &col);
void update(Model::GameModel &m, float dt);
void addToLeaderboard(Model::GameModel &m);
}}
#endif
