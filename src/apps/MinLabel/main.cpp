#include "gui/MainWindow.h"

#include "QMSystem.h"

#include <QApplication>

#include <cpp-pinyin/G2pglobal.h>

#ifdef Q_OS_WINDOWS
#    include <Windows.h>
#endif

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    if (QMOs::isUserRoot() && !QApplication::arguments().contains("--allow-root")) {
        QString title = QApplication::applicationName();
        QString msg = QString("You're trying to start %1 as the %2, which may cause "
                              "security problem and isn't recommended.")
                          .arg(QApplication::applicationName(), "Administrator");
#ifdef Q_OS_WINDOWS
        ::MessageBoxW(nullptr, msg.toStdWString().data(), title.toStdWString().data(),
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
    QApplication::setFont(f);
#endif

#ifdef Q_OS_MAC
    Pinyin::setDictionaryPath(QApplication::applicationDirPath().toUtf8().toStdString() + "/../Resources/dict");
#else
    Pinyin::setDictionaryPath(QApplication::applicationDirPath().toUtf8().toStdString() + "/dict");
#endif

    MainWindow w;
    w.show();

    return QApplication::exec();
}
