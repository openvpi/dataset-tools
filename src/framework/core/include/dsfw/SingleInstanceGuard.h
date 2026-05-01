#pragma once

/// @file SingleInstanceGuard.h
/// @brief Single-instance application guard using QLockFile + QLocalServer.

#include <QObject>
#include <QString>
#include <QStringList>

class QLockFile;
class QLocalServer;

namespace dsfw {

/// @brief Ensures only one instance of the application runs at a time.
///
/// Uses QLockFile for process locking and QLocalServer/QLocalSocket for
/// inter-process communication. If a second instance starts, it sends its
/// command-line arguments to the first instance and exits.
///
/// Server name includes the username to avoid conflicts on shared machines.
///
/// @code
/// SingleInstanceGuard guard("MyApp");
/// if (!guard.tryLock()) {
///     guard.sendArguments(app.arguments());
///     return 0;
/// }
/// guard.listen();
/// QObject::connect(&guard, &SingleInstanceGuard::argumentsReceived,
///     [](const QStringList &args) { /* handle file open etc. */ });
/// @endcode
class SingleInstanceGuard : public QObject {
    Q_OBJECT
public:
    /// @brief Construct the guard for the given application identifier.
    /// @param appId Unique application identifier (e.g. "PitchLabeler").
    /// @param parent Optional QObject parent.
    explicit SingleInstanceGuard(const QString &appId, QObject *parent = nullptr);
    ~SingleInstanceGuard() override;

    /// @brief Try to acquire the lock. Returns true if this is the first instance.
    bool tryLock();

    /// @brief Start listening for messages from other instances.
    /// Call after tryLock() returns true.
    void listen();

    /// @brief Send arguments to the running instance.
    /// Call when tryLock() returns false.
    /// @param arguments Command-line arguments to send.
    /// @return True if arguments were sent successfully.
    bool sendArguments(const QStringList &arguments);

signals:
    /// @brief Emitted when another instance sends its arguments.
    /// @param arguments The command-line arguments from the second instance.
    void argumentsReceived(const QStringList &arguments);

private:
    void onNewConnection();

    QString m_serverName;
    QLockFile *m_lockFile = nullptr;
    QLocalServer *m_server = nullptr;
};

} // namespace dsfw
