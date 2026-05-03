#include "EditorPageBase.h"
#include "SliceListPanel.h"
#include "ISettingsBackend.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMenuBar>
#include <QMimeData>
#include <QUrl>

namespace dstools {

EditorPageBase::EditorPageBase(QWidget *parent)
    : QWidget(parent) {
}

EditorPageBase::~EditorPageBase() = default;

void EditorPageBase::setDataSource(IEditorDataSource *source, ISettingsBackend *settingsBackend) {
    m_source = source;
    m_settingsBackend = settingsBackend;
}

QMenuBar *EditorPageBase::createMenuBar(QWidget *parent) {
    auto *menuBar = new QMenuBar(parent);

    auto *navMenu = menuBar->addMenu(QStringLiteral("&Navigation"));
    m_prevAction = navMenu->addAction(QStringLiteral("Previous Slice"));
    m_nextAction = navMenu->addAction(QStringLiteral("Next Slice"));
    m_playAction = navMenu->addAction(QStringLiteral("Play Audio"));

    m_prevAction->setShortcut(Qt::Key_Up);
    m_nextAction->setShortcut(Qt::Key_Down);
    m_playAction->setShortcut(Qt::Key_Space);

    return menuBar;
}

QWidget *EditorPageBase::createStatusBarContent(QWidget *parent) {
    Q_UNUSED(parent)
    return nullptr;
}

bool EditorPageBase::hasUnsavedChanges() const {
    return m_dirty;
}

void EditorPageBase::handleDragEnter(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void EditorPageBase::handleDrop(QDropEvent *event) {
    Q_UNUSED(event)
}

void EditorPageBase::onActivated() {}

bool EditorPageBase::onDeactivating() {
    return maybeSave();
}

void EditorPageBase::onShutdown() {}

dstools::widgets::ShortcutManager *EditorPageBase::shortcutManager() const {
    return m_shortcutManager;
}

bool EditorPageBase::maybeSave() {
    if (!m_dirty)
        return true;
    return saveCurrentSlice();
}

} // namespace dstools
