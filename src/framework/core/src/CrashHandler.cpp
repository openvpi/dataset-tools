#include <dsfw/CrashHandler.h>
#include <dsfw/AppPaths.h>
#include <dsfw/Log.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>

static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS *exceptionInfo) {
    auto &handler = dsfw::CrashHandler::instance();
    QString dumpDir = handler.dumpDirectory();

    if (dumpDir.isEmpty()) {
        dumpDir = dsfw::AppPaths::dumpDir();
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

        DSFW_LOG_ERROR("app", ("Crash dump written to: " + dumpFileName.toStdString()).c_str());
    }

    handler.invokeCrashCallback(dumpFileName);

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
#include <csignal>
#include <unistd.h>

static void unixSignalHandler(int sig) {
    auto &handler = dsfw::CrashHandler::instance();
    QString dumpDir = handler.dumpDirectory();

    if (dumpDir.isEmpty()) {
        dumpDir = dsfw::AppPaths::dumpDir();
    }

    QDir().mkpath(dumpDir);

    QString crashFile = dumpDir + QStringLiteral("/crash_%1.txt")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));

    QFile file(crashFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << "Signal: " << sig << "\n";
        stream << "Time: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
        file.close();
    } else {
        const char msg[] = "CrashHandler: failed to write crash log\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }

    handler.invokeCrashCallback(crashFile);

    _exit(128 + sig);
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
    DSFW_LOG_INFO("app", "Windows crash handler installed");
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    struct sigaction sa;
    sa.sa_handler = unixSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    DSFW_LOG_INFO("app", "Unix signal handler installed");
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

void CrashHandler::invokeCrashCallback(const QString &dumpPath) {
    if (m_callback)
        m_callback(dumpPath);
}

} // namespace dsfw
