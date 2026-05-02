#include "PipelinePage.h"
#include "PipelineKeys.h"
#include "slicer/SlicerPage.h"
#include "lyricfa/LyricFAPage.h"
#include "hubertfa/HubertFAPage.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QTabWidget>

#include <dstools/ShortcutEditorWidget.h>
#include <dsfw/Theme.h>

PipelinePage::PipelinePage(dstools::AppSettings *settings, QWidget *parent)
    : QWidget(parent), m_settings(settings) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget(this);

    m_slicerPage = new SlicerPage(this);
    m_lyricFAPage = new LyricFAPage(this);
    m_hubertFAPage = new HubertFAPage(this);

    m_tabWidget->addTab(m_slicerPage, tr("AudioSlicer"));
    m_tabWidget->addTab(m_lyricFAPage, tr("LyricFA"));
    m_tabWidget->addTab(m_hubertFAPage, tr("HubertFA"));

    layout->addWidget(m_tabWidget);

    // Restore last tab
    int lastTab = m_settings->get(PipelineKeys::LastTab);
    if (lastTab >= 0 && lastTab < m_tabWidget->count())
        m_tabWidget->setCurrentIndex(lastTab);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        m_settings->set(PipelineKeys::LastTab, index);
    });
}

QMenuBar *PipelinePage::createMenuBar(QWidget *parent) {
    auto *menuBar = new QMenuBar(parent);

    auto *fileMenu = menuBar->addMenu(tr("File(&F)"));

    auto *shortcutAction = fileMenu->addAction(tr("Shortcut Settings..."));
    connect(shortcutAction, &QAction::triggered, this, [this]() {
        std::vector<dstools::widgets::ShortcutEntry> entries;
        dstools::widgets::ShortcutEditorWidget::showDialog(m_settings, entries, this);
    });

    fileMenu->addSeparator();
    fileMenu->addAction(tr("Quit"), QKeySequence::Quit, qApp, &QApplication::quit);

    auto *viewMenu = menuBar->addMenu(tr("View(&V)"));
    dsfw::Theme::instance().populateThemeMenu(viewMenu);

    auto *helpMenu = menuBar->addMenu(tr("Help(&H)"));
    helpMenu->addAction(tr("About %1").arg(qApp->applicationName()), this, [this]() {
        QMessageBox::information(this, qApp->applicationName(),
            QString("%1 %2, Copyright OpenVPI.").arg(qApp->applicationName(), APP_VERSION));
    });
    helpMenu->addAction(tr("About Qt"), this, [this]() {
        QMessageBox::aboutQt(this);
    });

    return menuBar;
}

QWidget *PipelinePage::createStatusBarContent(QWidget *parent) {
    Q_UNUSED(parent);
    return nullptr;
}

QString PipelinePage::windowTitle() const {
    return QStringLiteral("Dataset Pipeline");
}
