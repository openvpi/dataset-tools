#include "GameInferPage.h"
#include "MainWidget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMimeData>
#include <QProgressBar>

#include <dstools/ShortcutEditorWidget.h>
#include <dsfw/Theme.h>

GameInferPage::GameInferPage(dstools::AppSettings *settings, QWidget *parent)
    : QWidget(parent), m_settings(settings) {
    auto *layout = new QHBoxLayout(this);
    m_mainWidget = new MainWidget(m_settings, this);
    layout->addWidget(m_mainWidget);
    layout->setContentsMargins(0, 0, 0, 0);
}

QMenuBar *GameInferPage::createMenuBar(QWidget *parent) {
    auto *menuBar = new QMenuBar(parent);
    auto *viewMenu = menuBar->addMenu(tr("View(&V)"));
    dsfw::Theme::instance().populateThemeMenu(viewMenu);

    viewMenu->addSeparator();
    auto *shortcutAction = viewMenu->addAction(tr("Shortcut Settings..."));
    connect(shortcutAction, &QAction::triggered, this, [this]() {
        std::vector<dstools::widgets::ShortcutEntry> entries;
        dstools::widgets::ShortcutEditorWidget::showDialog(m_settings, entries, this);
    });

    return menuBar;
}

QWidget *GameInferPage::createStatusBarContent(QWidget *parent) {
    auto *progressBar = new QProgressBar(parent);
    progressBar->setVisible(false);
    progressBar->setMaximumWidth(200);
    return progressBar;
}

QString GameInferPage::windowTitle() const {
    return QStringLiteral("GameInfer");
}

bool GameInferPage::supportsDragDrop() const {
    return true;
}

void GameInferPage::handleDragEnter(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void GameInferPage::handleDrop(QDropEvent *event) {
    Q_UNUSED(event);
    // NOTE: Drag-and-drop forwarding requires a public API on MainWidget (e.g. addFiles).
}
