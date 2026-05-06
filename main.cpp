#include <QApplication>
#include <QCoreApplication>
#include "gamewidget.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("RetroArcher");
    QCoreApplication::setApplicationName("RetroArcher");

    QApplication app(argc, argv);
    GameWidget w;
    w.setWindowTitle("Retro Archer");
    w.show();
    return app.exec();
}
