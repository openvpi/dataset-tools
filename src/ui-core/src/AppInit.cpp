#include <dstools/AppInit.h>

#include <dsfw/ModelManager.h>
#include <dsfw/ServiceLocator.h>

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
        QMessageBox::warning(nullptr, title, msg, QApplication::tr("Confirm"));
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
#ifdef HAS_QBREAKPAD
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

    // 4. Ensure config directory exists
    QString configDirPath = app.applicationDirPath() + "/config";
    QDir configDir(configDirPath);
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }

    // 5. Register core services with ServiceLocator
    if (!ServiceLocator::get<ModelManager>()) {
        auto *modelMgr = new ModelManager(&app);
        ServiceLocator::set<ModelManager>(modelMgr);
    }

    // 6. Call registered post-init hooks
    for (const auto &hook : hooks()) {
        hook(app);
    }

    return true;
}

} // namespace dstools
