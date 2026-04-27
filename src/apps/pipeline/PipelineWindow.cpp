#include "PipelineWindow.h"
#include "PipelineKeys.h"
#include "slicer/SlicerPage.h"
#include "lyricfa/LyricFAPage.h"
#include "hubertfa/HubertFAPage.h"

#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

#include <dstools/ShortcutEditorWidget.h>
#include <dstools/Theme.h>

PipelineWindow::PipelineWindow(QWidget *parent)
    : QMainWindow(parent), m_settings("DatasetPipeline") {
    setWindowTitle("Dataset Pipeline");
    resize(1200, 800);
    setupUI();
    setupMenuBar();

    // Restore last tab
    int lastTab = m_settings.get(PipelineKeys::LastTab);
    if (lastTab >= 0 && lastTab < m_tabWidget->count())
        m_tabWidget->setCurrentIndex(lastTab);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        m_settings.set(PipelineKeys::LastTab, index);
    });

    statusBar()->showMessage(tr("Ready"));
}

void PipelineWindow::setupUI() {
    m_tabWidget = new QTabWidget(this);

    m_slicerPage = new SlicerPage(this);
    m_lyricFAPage = new LyricFAPage(this);
    m_hubertFAPage = new HubertFAPage(this);

    m_tabWidget->addTab(m_slicerPage, tr("AudioSlicer"));
    m_tabWidget->addTab(m_lyricFAPage, tr("LyricFA"));
    m_tabWidget->addTab(m_hubertFAPage, tr("HubertFA"));

    setCentralWidget(m_tabWidget);
}

void PipelineWindow::setupMenuBar() {
    auto *fileMenu = menuBar()->addMenu(tr("File(&F)"));

    auto *shortcutAction = fileMenu->addAction(tr("Shortcut Settings..."));
    connect(shortcutAction, &QAction::triggered, this, [this]() {
        std::vector<dstools::widgets::ShortcutEntry> entries;
        dstools::widgets::ShortcutEditorWidget::showDialog(&m_settings, entries, this);
    });

    fileMenu->addSeparator();
    fileMenu->addAction(tr("Quit"), qApp, &QApplication::quit, QKeySequence::Quit);

    auto *viewMenu = menuBar()->addMenu(tr("View(&V)"));
    dstools::Theme::instance().populateThemeMenu(viewMenu);

    auto *helpMenu = menuBar()->addMenu(tr("Help(&H)"));
    helpMenu->addAction(tr("About %1").arg(qApp->applicationName()), this, [this]() {
        QMessageBox::information(this, qApp->applicationName(),
            QString("%1 %2, Copyright OpenVPI.").arg(qApp->applicationName(), APP_VERSION));
    });
    helpMenu->addAction(tr("About Qt"), this, [this]() {
        QMessageBox::aboutQt(this);
    });
}
