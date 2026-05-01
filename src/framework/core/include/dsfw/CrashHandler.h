#pragma once

#include <QString>

#include <functional>

namespace dsfw {

class CrashHandler {
public:
    using CrashCallback = std::function<void(const QString &dumpPath)>;

    static CrashHandler &instance();

    void setDumpDirectory(const QString &dir);
    void setCrashCallback(CrashCallback callback);
    void install();
    void uninstall();

    QString dumpDirectory() const;

private:
    CrashHandler() = default;
    ~CrashHandler() = default;
    CrashHandler(const CrashHandler &) = delete;
    CrashHandler &operator=(const CrashHandler &) = delete;

    QString m_dumpDir;
    CrashCallback m_callback;
    bool m_installed = false;
};

} // namespace dsfw
