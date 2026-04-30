#include <dsfw/AsyncTask.h>

namespace dstools {

    AsyncTask::AsyncTask(QString identifier, QObject *parent)
        : QObject(parent), m_identifier(std::move(identifier)) {
    }

    void AsyncTask::run() {
        QString msg;
        if (execute(msg)) {
            Q_EMIT succeeded(m_identifier, msg);
        } else {
            Q_EMIT failed(m_identifier, msg);
        }
    }

    const QString &AsyncTask::identifier() const {
        return m_identifier;
    }

} // namespace dstools
