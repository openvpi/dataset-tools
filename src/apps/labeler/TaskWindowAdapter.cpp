#include "TaskWindowAdapter.h"

#include <dsfw/PipelineRunner.h>

namespace dstools::labeler {

TaskWindowAdapter::TaskWindowAdapter(dstools::widgets::TaskWindow *page, QWidget *parent)
    : QWidget(parent), m_page(page) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_page->setParent(this);
    layout->addWidget(m_page);
}

TaskWindowAdapter::~TaskWindowAdapter() = default;

dstools::widgets::TaskWindow *TaskWindowAdapter::innerPage() const {
    return m_page;
}

bool TaskWindowAdapter::hasUnsavedChanges() const {
    return false;
}

void TaskWindowAdapter::setWorkingDirectory(const QString &dir) {
    m_workingDir = dir;
}

QString TaskWindowAdapter::workingDirectory() const {
    return m_workingDir;
}

void TaskWindowAdapter::onActivated() {
}

bool TaskWindowAdapter::onDeactivating() {
    return true;
}

void TaskWindowAdapter::onDeactivated() {
}

void TaskWindowAdapter::onWorkingDirectoryChanged(const QString &newDir) {
    m_workingDir = newDir;
}

void TaskWindowAdapter::onShutdown() {
}

void TaskWindowAdapter::connectPipelineRunner(dstools::PipelineRunner *runner) {
    QObject::connect(runner, &dstools::PipelineRunner::progress,
                     this, [this](int, int item, int total, const QString &) {
                         if (total > 0)
                             m_page->setProgressValue(item * 100 / total);
                     });
    QObject::connect(runner, &dstools::PipelineRunner::stepCompleted,
                     this, [this](int, const QString &name) {
                         m_page->appendLog(QStringLiteral("Step completed: %1").arg(name));
                     });
    QObject::connect(runner, &dstools::PipelineRunner::itemDiscarded,
                     this, [this](const QString &itemId, const QString &reason) {
                         m_page->slot_oneFailed(itemId, reason);
                     });
}

}
