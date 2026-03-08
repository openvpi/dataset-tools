#include "MainWindow.h"
#include "MainWidget.h"

#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStatusBar>

#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_settings(nullptr), m_statusLabel(nullptr), m_progressBar(nullptr) {

    setWindowTitle("GameInfer - 音频转MIDI工具");
    resize(800, 450);

    // Setup settings
    const QString configDirPath = QApplication::applicationDirPath() + "/config";
    if (const QDir configDir(configDirPath); !configDir.exists()) {
        if (!configDir.mkpath(".")) {
            QMessageBox::critical(this, QApplication::applicationName(),
                                  "Failed to create config directory: " + configDir.absolutePath());
            return;
        }
    }

    m_settings = new QSettings(configDirPath + "/GameInfer.ini", QSettings::IniFormat);

    setupCentralWidget();
    setupStatusBar();
}

MainWindow::~MainWindow() {
    delete m_settings;
}

void MainWindow::setupCentralWidget() {
    m_mainLayout = new QHBoxLayout();
    m_mainWidget = new MainWidget(m_settings, this);

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