#include <dstools/AppInit.h>

#include <dstools/ModelManager.h>
#include <dsfw/AppPaths.h>
#include <dsfw/CrashHandler.h>
#include <dsfw/FileLogSink.h>
#include <dsfw/IModelManager.h>
#include <dsfw/Log.h>
#include <dsfw/ServiceLocator.h>
#include <dsfw/TranslationManager.h>

#include <QApplication>
#include <QDir>
#include <QFont>
#include <QMessageBox>

#ifdef Q_OS_WIN
#include <ShlObj.h>
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace dstools {

std::vector<AppInit::InitHook> &AppInit::hooks() {
    static std::vector<InitHook> s_hooks;
    return s_hooks;
}

void AppInit::registerPostInitHook(InitHook hook) {
    hooks().push_back(std::move(hook));
}

bool AppInit::init(QApplication &app, bool initCrashHandler) {
    // 0. Required by QWindowKit (frameless window) — must be set before any widget is created
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    // 1. Root privilege check
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
    if (geteuid() == 0 && !app.arguments().contains("--allow-root")) {
        QString title = app.applicationName();
        QString msg = QString("You're trying to start %1 as root, which may cause "
                              "security problem and isn't recommended.")
                          .arg(app.applicationName());
        QMessageBox::warning(nullptr, title, msg, QMessageBox::Ok);
        return false;
    }
#endif

    // 2. Font setup
#ifdef Q_OS_WIN
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
        QFont f("Microsoft YaHei");
        f.setPointSize(9);
        app.setFont(f);
    }
#endif

    // 3. Crash handler (optional)
    if (initCrashHandler) {
        dsfw::CrashHandler::instance().setDumpDirectory(dsfw::AppPaths::dumpDir());
        dsfw::CrashHandler::instance().install();
    }

    // 4. Migrate legacy paths and ensure config directory exists
    dsfw::AppPaths::migrateFromLegacyPaths();
    dsfw::AppPaths::configDir();

    // 4.5 Register file log sink and clean old logs
    QString logDir = dsfw::AppPaths::logDir();
    Logger::instance().addSink(dsfw::createFileLogSink(logDir, app.applicationName()));
    dsfw::cleanOldLogFiles(logDir);

    // 5. Register core services with ServiceLocator
    if (!ServiceLocator::get<IModelManager>()) {
        auto *modelMgr = new ModelManager(&app);
        ServiceLocator::set<IModelManager>(modelMgr);
    }

    // 6. Load translations (follows system locale; apps can customize via CommonKeys::Language)
    dsfw::TranslationManager::install();

    // 7. Call registered post-init hooks
    for (const auto &hook : hooks()) {
        hook(app);
    }

    return true;
}

} // namespace dstools
