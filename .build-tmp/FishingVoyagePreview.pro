QT += core gui widgets

CONFIG += release c++17
CONFIG -= debug debug_and_release
TEMPLATE = app
TARGET = FishingVoyagePreview
DESTDIR = ../build-preview

QMAKE_CXXFLAGS_RELEASE = -O0 -DNDEBUG
QMAKE_LFLAGS_RELEASE += -s

OBJECTS_DIR = obj
MOC_DIR = moc
RCC_DIR = rcc
UI_DIR = ui

SOURCES += \
    ../BackpackDialog.cpp \
    ../boss.cpp \
    ../Enemy.cpp \
    ../EncyclopediaDialog.cpp \
    ../Filemanager.cpp \
    ../Fish.cpp \
    ../GameUiDialog.cpp \
    ../GameManager.cpp \
    ../InventorySystem.cpp \
    ../Item.cpp \
    ../obstacle.cpp \
    ../player.cpp \
    ../Shopdialog.cpp \
    ../wavesystem.cpp \
    ../Weapon.cpp \
    ../weathersystem.cpp \
    ../GameWindow.cpp \
    ../main.cpp

HEADERS += \
    ../BackpackDialog.h \
    ../Boss.h \
    ../Enemy.h \
    ../EncyclopediaDialog.h \
    ../FileManager.h \
    ../Fish.h \
    ../GameConfig.h \
    ../GameUiDialog.h \
    ../GameManager.h \
    ../InventorySystem.h \
    ../Item.h \
    ../ItemFactory.h \
    ../Obstacle.h \
    ../Player.h \
    ../Shopdialog.h \
    ../WaveSystem.h \
    ../Weapon.h \
    ../WeatherSystem.h \
    ../GameWindow.h

# Generated explicitly before qmake: GNU make on Windows cannot stat several
# UTF-8 resource paths even though Qt's rcc can read them correctly.
SOURCES += rcc/qrc_FishingVoyage.cpp
