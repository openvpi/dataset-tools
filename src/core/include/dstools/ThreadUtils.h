#pragma once
#include <QMetaObject>
#include <QCoreApplication>
#include <functional>

namespace dstools {

/// Execute a callback on the main thread (safely operate UI from worker threads).
/// Replaces direct QMessageBox calls from worker threads.
inline void invokeOnMain(std::function<void()> fn) {
    QMetaObject::invokeMethod(
        qApp, std::move(fn), Qt::QueuedConnection);
}

} // namespace dstools
