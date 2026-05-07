#ifndef CONTROLLER_DIALOGSYSTEM_H
#define CONTROLLER_DIALOGSYSTEM_H

class QString;
class QColor;
namespace Model { struct GameModel; }

namespace Controller { namespace Dialog {
void schedule(Model::GameModel &m, const QString &speaker, const QString &text, float dur, QColor color);
void triggerBoss(Model::GameModel &m, int subType, int stage);
}}

#endif
