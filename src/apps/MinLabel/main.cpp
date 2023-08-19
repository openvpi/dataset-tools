#include "gui/MainWindow.h"

#include "QMSystem.h"

#include <QApplication>
#include <QMessageBox>

#include "g2pglobal.h"

#ifdef Q_OS_WINDOWS
#    include <Windows.h>
#endif

int main(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);

    if (QMOs::isUserRoot() && !a.arguments().contains("--allow-root")) {
        QString title = qApp->applicationName();
        QString msg = QString("You're trying to start %1 as the %2, which may cause "
                              "security problem and isn't recommended.")
                          .arg(qApp->applicationName(), "Administrator");
#ifdef Q_OS_WINDOWS
        ::MessageBoxW(0, msg.toStdWString().data(), title.toStdWString().data(),
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

// Initialize g2p
// #ifdef Q_OS_MAC
//     IKg2p::setDictionaryPath(qApp->applicationDirPath() + "/../Resources/ChorusKit/g2p/dict");
// #else
//     IKg2p::setDictionaryPath(qApp->applicationDirPath() + "/../share/ChorusKit/g2p/dict");
// #endif
#ifdef Q_OS_MAC
    IKg2p::setDictionaryPath(qApp->applicationDirPath() + "/../Resources/dict");
#else
    IKg2p::setDictionaryPath(qApp->applicationDirPath() + "/dict");
#endif

    MainWindow w;
    w.show();

    return a.exec();
}
