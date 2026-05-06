#include <dsfw/LogNotifier.h>

#include <QCoreApplication>
#include <QMetaObject>

namespace dstools {

LogNotifier &LogNotifier::instance() {
    static LogNotifier s_instance;
    return s_instance;
}

LogNotifier::LogNotifier(QObject *parent)
    : QObject(parent)
{
    // Ensure the notifier lives on the main thread.
    // Created via static local → first call should be from main().
    if (QCoreApplication::instance() && thread() != QCoreApplication::instance()->thread()) {
        moveToThread(QCoreApplication::instance()->thread());
    }
}

LogSink LogNotifier::sink() {
    return [](const LogEntry &entry) {
        // Deliver always on the main thread via queued connection.
        auto *target = &LogNotifier::instance();
        QMetaObject::invokeMethod(target, [target, entry]() {
            emit target->entryAdded(entry);
        }, Qt::QueuedConnection);
    };
}

} // namespace dstools
