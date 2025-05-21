#include "gui/MainWindow.h"

#include <QApplication>

#ifdef Q_OS_WINDOWS
#    include <ShlObj.h>
#    include <Windows.h>
#endif

bool isUserRoot() {
#ifdef Q_OS_WINDOWS
    return IsUserAnAdmin();
#else
    return geteuid() == 0;
#endif
}

int main(int argc, char *argv[]) {
    // QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    // QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);

    if (isUserRoot() && !a.arguments().contains("--allow-root")) {
        QString title = qApp->applicationName();
        QString msg = QString("You're trying to start %1 as the %2, which may cause "
                              "security problem and isn't recommended.")
                          .arg(qApp->applicationName(), "Administrator");
#ifdef Q_OS_WINDOWS
        MessageBoxW(0, msg.toStdWString().data(), title.toStdWString().data(),
                    MB_OK | MB_TOPMOST | MB_SETFOREGROUND | MB_ICONWARNING);
#elif defined(Q_OS_LINUX)
        fputs(qPrintable(msg), stdout);
#else
        QMessageBox::warning(nullptr, title, msg, QApplication::tr("Confirm"));
#endif
        return 0;
    }

#ifdef Q_OS_WINDOWS
    QFont f("Microsoft YaHei");
    f.setPointSize(9);
    a.setFont(f);
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
