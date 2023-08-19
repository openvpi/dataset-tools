#include "slicer/mainwindow.h"

#include <QApplication>


//#ifdef Q_OS_WINDOWS
//#    include <Windows.h>
//#endif

int main(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
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
