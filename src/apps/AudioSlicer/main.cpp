#include "slicer/mainwindow.h"

#include <QApplication>
#include <QMessageBox>


#ifdef Q_OS_WINDOWS
#    include <Windows.h>
#endif

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationName("Audio Slicer");
    a.setApplicationDisplayName("Audio Slicer");


#if defined(Q_OS_WIN)
    QFont font("Microsoft Yahei UI");
    font.setPointSize(9);
    a.setFont(font);
#endif

    // Set library loading info
// #ifdef Q_OS_MAC
//     qApp->addLibraryPath(qApp->applicationDirPath() + "/../Frameworks/ChorusKit/plugins");
// #else
//     qApp->addLibraryPath(qApp->applicationDirPath() + "/../lib/ChorusKit/plugins");
// #endif

    MainWindow w;
    w.show();

    return a.exec();
}
