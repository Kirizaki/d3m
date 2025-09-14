#include <QApplication>
#include "gui/main_window.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(1000, 700);
    w.show();
    return app.exec();
}