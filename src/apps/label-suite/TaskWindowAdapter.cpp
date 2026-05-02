#include "TaskWindowAdapter.h"

#include "commands/DiscardSliceCommand.h"

#include <dsfw/PipelineRunner.h>
#include <QMenu>

namespace dstools::labeler {

TaskWindowAdapter::TaskWindowAdapter(dstools::widgets::TaskWindow *page, QWidget *parent)
    : QWidget(parent), m_page(page) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_page->setParent(this);
    layout->addWidget(m_page);

    setupSliceContextMenu();
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

void TaskWindowAdapter::setupSliceContextMenu() {
    auto *listWidget = m_page->taskList();
    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(listWidget, &QListWidget::customContextMenuRequested,
                     this, [this, listWidget](const QPoint &pos) {
                         QListWidgetItem *item = listWidget->itemAt(pos);
                         if (!item)
                             return;

                         auto *menu = new QMenu(this);
                         const bool isDiscarded = item->data(Qt::UserRole + 2).toBool();

                         if (isDiscarded) {
                             auto *restoreAction = menu->addAction(tr("Restore"));
                             QObject::connect(restoreAction, &QAction::triggered, this, [this, listWidget, item]() {
                                 int row = listWidget->row(item);
                                 m_undoStack.push(new DiscardSliceCommand(listWidget, row,
                                     DiscardSliceCommand::State::Restore));
                             });
                         } else {
                             auto *discardAction = menu->addAction(tr("Discard"));
                             QObject::connect(discardAction, &QAction::triggered, this, [this, listWidget, item]() {
                                 int row = listWidget->row(item);
                                 m_undoStack.push(new DiscardSliceCommand(listWidget, row,
                                     DiscardSliceCommand::State::Discard));
                             });
                         }

                         menu->exec(listWidget->mapToGlobal(pos));
                         menu->deleteLater();
                     });
}

}
