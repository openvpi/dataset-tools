#include "TaskWindowAdapter.h"

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
    Q_UNUSED(newDir);
}

void TaskWindowAdapter::onShutdown() {
}

}
