#include "PipelineWindow.h"
#include "slicer/SlicerPage.h"
#include "lyricfa/LyricFAPage.h"
#include "hubertfa/HubertFAPage.h"

#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

PipelineWindow::PipelineWindow(QWidget *parent)
    : QMainWindow(parent), m_config("DatasetPipeline") {
    setWindowTitle("Dataset Pipeline");
    resize(1200, 800);
    setupUI();
    setupMenuBar();

    // Restore last tab
    int lastTab = m_config.getInt("General/lastTab", 0);
    if (lastTab >= 0 && lastTab < m_tabWidget->count())
        m_tabWidget->setCurrentIndex(lastTab);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        m_config.setInt("General/lastTab", index);
    });

    statusBar()->showMessage(tr("Ready"));
}

PipelineWindow::~PipelineWindow() {
    m_config.sync();
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
    fileMenu->addAction(tr("Quit"), qApp, &QApplication::quit, QKeySequence::Quit);

    auto *helpMenu = menuBar()->addMenu(tr("Help(&H)"));
    helpMenu->addAction(tr("About %1").arg(qApp->applicationName()), this, [this]() {
        QMessageBox::information(this, qApp->applicationName(),
            QString("%1 %2, Copyright OpenVPI.").arg(qApp->applicationName(), APP_VERSION));
    });
    helpMenu->addAction(tr("About Qt"), this, [this]() {
        QMessageBox::aboutQt(this);
    });
}
