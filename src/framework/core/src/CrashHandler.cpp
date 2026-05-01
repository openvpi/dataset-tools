#include <dsfw/CrashHandler.h>
#include <dsfw/Log.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>

static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS *exceptionInfo) {
    auto &handler = dsfw::CrashHandler::instance();
    QString dumpDir = handler.dumpDirectory();

    if (dumpDir.isEmpty()) {
        dumpDir = QCoreApplication::applicationDirPath() + QStringLiteral("/crash_dumps");
    }

    QDir().mkpath(dumpDir);

    QString dumpFileName = dumpDir + QStringLiteral("/crash_%1.dmp")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));

    HANDLE hFile = CreateFileW(
        reinterpret_cast<LPCWSTR>(dumpFileName.utf16()),
        GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei;
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = exceptionInfo;
        mei.ClientPointers = FALSE;

        MiniDumpWriteDump(
            GetCurrentProcess(), GetCurrentProcessId(), hFile,
            MiniDumpWithDataSegs, &mei, nullptr, nullptr);

        CloseHandle(hFile);

        DSFW_LOG_ERROR("CrashHandler", ("Crash dump written to: " + dumpFileName.toStdString()).c_str());
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

namespace dsfw {

CrashHandler &CrashHandler::instance() {
    static CrashHandler handler;
    return handler;
}

void CrashHandler::setDumpDirectory(const QString &dir) {
    m_dumpDir = dir;
}

void CrashHandler::setCrashCallback(CrashCallback callback) {
    m_callback = std::move(callback);
}

void CrashHandler::install() {
    if (m_installed)
        return;

#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(unhandledExceptionFilter);
    DSFW_LOG_INFO("CrashHandler", "Windows crash handler installed");
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    struct sigaction sa;
    sa.sa_handler = [](int sig) {
        DSFW_LOG_ERROR("CrashHandler", ("Received signal: " + std::to_string(sig)).c_str());
        _exit(128 + sig);
    };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    DSFW_LOG_INFO("CrashHandler", "Unix signal handler installed");
#endif

    m_installed = true;
}

void CrashHandler::uninstall() {
    if (!m_installed)
        return;

#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(nullptr);
#endif

    m_installed = false;
}

QString CrashHandler::dumpDirectory() const {
    return m_dumpDir;
}

} // namespace dsfw
