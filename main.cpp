#include "FishingVoyage.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    FishingVoyage window;
    window.show();
    return app.exec();
}
