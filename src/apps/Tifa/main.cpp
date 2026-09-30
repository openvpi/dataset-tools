#include "gui/MainWindow.h"
#include "util/PathUtils.h"

#include <filesystem>

#include <QApplication>
#include <QMessageBox>

#include <cpp-pinyin/G2pglobal.h>

#ifdef Q_OS_WINDOWS
#    include <ShlObj.h>
#    include <Windows.h>
#else
#    include <unistd.h>

#    include <cstdio>
#endif

bool isUserRoot() {
#ifdef Q_OS_WINDOWS
    return IsUserAnAdmin();
#else
    return geteuid() == 0;
#endif
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    if (isUserRoot() && !QApplication::arguments().contains("--allow-root")) {
        QString title = QApplication::applicationName();
        QString msg = QString("You're trying to start %1 as the %2, which may cause "
                              "security problem and isn't recommended.")
                          .arg(QApplication::applicationName(), "Administrator");
#ifdef Q_OS_WINDOWS
        MessageBoxW(nullptr, msg.toStdWString().data(), title.toStdWString().data(),
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

    // Mandarin and Cantonese dictionaries of the phonetic front end. The other
    // dictionaries of TIFA are read from the folder selected in the window.
#ifdef Q_OS_MAC
    std::filesystem::path dictDir =
        Tifa::toPath(QApplication::applicationDirPath()) / ".." / "Resources" / "dict";
#else
    std::filesystem::path dictDir = Tifa::toPath(QApplication::applicationDirPath()) / "dict";
#endif
    Pinyin::setDictionaryPath(dictDir.make_preferred());

    Tifa::MainWindow w;
    w.show();

    return QApplication::exec();
}
