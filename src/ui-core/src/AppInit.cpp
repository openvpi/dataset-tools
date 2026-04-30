#include <dstools/AppInit.h>

#include <QApplication>
#include <QDir>
#include <QFont>
#include <QMessageBox>

#ifdef Q_OS_WIN
#include <ShlObj.h>
#include <Windows.h>
#endif

#ifdef HAS_QBREAKPAD
#include <QBreakpadHandler.h>
#endif

#ifdef HAS_CPP_PINYIN
#include <cpp-pinyin/G2pglobal.h>
#include <filesystem>
#endif

namespace dstools {

bool AppInit::init(QApplication &app, bool initPinyin, bool initCrashHandler) {
    // 1. Root privilege check — from original MinLabel/SlurCutter main.cpp
#ifdef Q_OS_WIN
    if (IsUserAnAdmin() && !app.arguments().contains("--allow-root")) {
        QString title = app.applicationName();
        QString msg = QString("You're trying to start %1 as the %2, which may cause "
                              "security problem and isn't recommended.")
                          .arg(app.applicationName(), "Administrator");
        MessageBoxW(nullptr, msg.toStdWString().data(), title.toStdWString().data(),
                    MB_OK | MB_TOPMOST | MB_SETFOREGROUND | MB_ICONWARNING);
        return false;
    }
#elif defined(Q_OS_LINUX)
    if (geteuid() == 0 && !app.arguments().contains("--allow-root")) {
        QString msg = QString("You're trying to start %1 as root, which may cause "
                              "security problem and isn't recommended.")
                          .arg(app.applicationName());
        fputs(qPrintable(msg), stdout);
        return false;
    }
#else
    // macOS: show GUI warning
    if (geteuid() == 0 && !app.arguments().contains("--allow-root")) {
        QString title = app.applicationName();
        QString msg = QString("You're trying to start %1 as root, which may cause "
                              "security problem and isn't recommended.")
                          .arg(app.applicationName());
        QMessageBox::warning(nullptr, title, msg, QApplication::tr("Confirm"));
        return false;
    }
#endif

    // 2. Font setup — from original various main.cpp Windows font logic
#ifdef Q_OS_WIN
    // Use Win32 API to retrieve system font metrics (from LyricFA/HubertFA pattern)
    NONCLIENTMETRICSW metrics = {sizeof(NONCLIENTMETRICSW)};
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &metrics, 0)) {
        QString fontFace = QString::fromWCharArray(metrics.lfMessageFont.lfFaceName);
        qreal fontPointSize = 0.0;
        if (HDC hDC = GetDC(nullptr)) {
            int dpiY = GetDeviceCaps(hDC, LOGPIXELSY);
            fontPointSize = -72.0 * metrics.lfMessageFont.lfHeight / dpiY;
            ReleaseDC(nullptr, hDC);
        }
        QFont font(fontFace);
        if (fontPointSize > 0) {
            font.setPointSizeF(fontPointSize);
        }
        app.setFont(font);
    } else {
        // Fallback to Microsoft YaHei (original MinLabel/SlurCutter pattern)
        QFont f("Microsoft YaHei");
        f.setPointSize(9);
        app.setFont(f);
    }
#endif

    // 3. Crash handler (optional)
    if (initCrashHandler) {
#ifdef HAS_QBREAKPAD
        // QBreakpad initialization — from original MinLabel main.cpp
        static QBreakpadHandler *handler = nullptr;
        if (!handler) {
            handler = new QBreakpadHandler();
            handler->setDumpPath(app.applicationDirPath() + "/dumps");
#ifdef Q_OS_WIN
            QBreakpadHandler::UniqueExtraHandler = []() {
                MessageBoxW(nullptr, L"Crash!!!", L"QBreakpad", MB_OK | MB_ICONERROR);
            };
#endif
        }
#endif
    }

    // 4. cpp-pinyin dictionary (optional)
    if (initPinyin) {
#ifdef HAS_CPP_PINYIN
        std::filesystem::path dictPath =
#ifdef Q_OS_MAC
            std::filesystem::path(app.applicationDirPath().toStdString()) / ".." / "Resources" / "dict";
#else
            std::filesystem::path(app.applicationDirPath().toStdWString()) / "dict";
#endif
        Pinyin::setDictionaryPath(dictPath.make_preferred().string());
#endif
    }

    // 5. Ensure config directory exists
    QString configDirPath = app.applicationDirPath() + "/config";
    QDir configDir(configDirPath);
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }
    return true;
}

} // namespace dstools
