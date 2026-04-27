#include "MainWindow.h"
#include "MainWidget.h"

#include <QApplication>
#include <QCloseEvent>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_settings("GameInfer"),
      m_statusLabel(nullptr), m_progressBar(nullptr) {

    setWindowTitle("GameInfer - \u97f3\u9891\u8f6cMIDI\u5de5\u5177");
    resize(800, 450);

    setupCentralWidget();
    setupStatusBar();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // AppSettings writes immediately; no manual sync needed.
    event->accept();
}

void MainWindow::setupCentralWidget() {
    m_mainLayout = new QHBoxLayout();
    m_mainWidget = new MainWidget(&m_settings, this);

    m_mainLayout->addWidget(m_mainWidget);
    setCentralWidget(new QWidget());
    centralWidget()->setLayout(m_mainLayout);
}

void MainWindow::setupStatusBar() {
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(200);

    statusBar()->addWidget(m_progressBar);
}
