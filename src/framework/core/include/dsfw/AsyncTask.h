#ifndef DSFW_ASYNCTASK_H
#define DSFW_ASYNCTASK_H

#include <QObject>
#include <QRunnable>
#include <QString>

namespace dstools {

    class AsyncTask : public QObject, public QRunnable {
        Q_OBJECT
    public:
        explicit AsyncTask(QString identifier, QObject *parent = nullptr);
        ~AsyncTask() override = default;

        void run() override final;

        const QString &identifier() const;

    protected:
        virtual bool execute(QString &msg) = 0;

    signals:
        void succeeded(const QString &identifier, const QString &msg);
        void failed(const QString &identifier, const QString &msg);

    private:
        QString m_identifier;
    };

} // namespace dstools

#endif // DSFW_ASYNCTASK_H
