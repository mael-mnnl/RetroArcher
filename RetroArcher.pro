QT       += core gui widgets
CONFIG   += c++17

TARGET    = RetroArcher
TEMPLATE  = app

INCLUDEPATH += .

SOURCES += \
    main.cpp \
    gamewidget.cpp \
    core/utils.cpp \
    model/skillsdata.cpp \
    model/worldsdata.cpp \
    model/savemanager.cpp \
    view/assetmanager.cpp \
    view/gameview.cpp \
    view/menuview.cpp \
    controller/collisionsystem.cpp \
    controller/skillsystem.cpp \
    controller/dialogsystem.cpp \
    controller/cheatsystem.cpp \
    controller/roombuilder.cpp \
    controller/combatsystem.cpp \
    controller/playercontroller.cpp \
    controller/enemyai.cpp \
    controller/tutorialsystem.cpp

HEADERS += \
    gamewidget.h \
    core/constants.h \
    core/palette.h \
    core/utils.h \
    model/types.h \
    model/entities.h \
    model/gamemodel.h \
    model/skillsdata.h \
    model/worldsdata.h \
    model/savemanager.h \
    view/assetmanager.h \
    view/gameview.h \
    view/menuview.h \
    controller/collisionsystem.h \
    controller/skillsystem.h \
    controller/dialogsystem.h \
    controller/cheatsystem.h \
    controller/roombuilder.h \
    controller/combatsystem.h \
    controller/playercontroller.h \
    controller/enemyai.h \
    controller/tutorialsystem.h

RESOURCES += resources.qrc
