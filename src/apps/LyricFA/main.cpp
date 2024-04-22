#include "gui/MainWindow.h"
#include <QApplication>

using namespace LyricFA;
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    MainWindow w;
    w.show();
    return QApplication::exec();
}
