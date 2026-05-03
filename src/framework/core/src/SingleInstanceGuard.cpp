#include <dsfw/SingleInstanceGuard.h>

#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>

namespace dsfw {

SingleInstanceGuard::SingleInstanceGuard(const QString &appId, QObject *parent) : QObject(parent) {
#ifdef Q_OS_WIN
    QString user = qEnvironmentVariable("USERNAME");
#else
    QString user = qEnvironmentVariable("USER");
#endif
    m_serverName = appId + QStringLiteral("-") + user;
}

SingleInstanceGuard::~SingleInstanceGuard() = default;

bool SingleInstanceGuard::tryLock() {
    const QString lockPath =
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
            .filePath(m_serverName + QStringLiteral(".lock"));

    m_lockFile = std::make_unique<QLockFile>(lockPath);
    m_lockFile->setStaleLockTime(0); // let QLockFile detect stale locks automatically
    return m_lockFile->tryLock(100);
}

void SingleInstanceGuard::listen() {
    QLocalServer::removeServer(m_serverName);
    m_server = new QLocalServer(this);
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstanceGuard::onNewConnection);
    m_server->listen(m_serverName);
}

bool SingleInstanceGuard::sendArguments(const QStringList &arguments) {
    QLocalSocket socket;
    socket.connectToServer(m_serverName);
    if (!socket.waitForConnected(1000))
        return false;

    const QByteArray data = arguments.join(QChar('\n')).toUtf8();
    socket.write(data);
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
    return true;
}

void SingleInstanceGuard::onNewConnection() {
    while (QLocalSocket *socket = m_server->nextPendingConnection()) {
        socket->waitForReadyRead(1000);
        const QByteArray data = socket->readAll();
        socket->deleteLater();

        if (!data.isEmpty()) {
            const QStringList args = QString::fromUtf8(data).split(QChar('\n'));
            emit argumentsReceived(args);
        }
    }
}

} // namespace dsfw
