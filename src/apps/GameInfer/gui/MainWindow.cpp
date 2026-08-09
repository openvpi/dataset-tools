#include "MainWindow.h"
#include "MainWidget.h"

#include <QApplication>
#include <QAction>
#include <QActionGroup>
#include <QDir>
#include <QEvent>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_settings(nullptr), m_statusLabel(nullptr), m_progressBar(nullptr) {
    resize(1100, 700);
    setMinimumSize(800, 560);

    // Setup settings
    const QString configDirPath = QApplication::applicationDirPath() + "/config";
    if (const QDir configDir(configDirPath); !configDir.exists()) {
        if (!configDir.mkpath(".")) {
            QMessageBox::critical(this, QApplication::applicationName(),
                                  tr("Failed to create the configuration directory: %1").arg(configDir.absolutePath()));
            return;
        }
    }

    m_settings = new QSettings(configDirPath + "/GameInfer.ini", QSettings::IniFormat);

    QString languageCode = m_settings->value("General/uiLanguage").toString();
    if (languageCode.isEmpty()) {
        languageCode = QLocale::system().language() == QLocale::Chinese ? "zh_CN" : "en";
    }
    applyLanguage(languageCode, false);

    setupCentralWidget();
    setupStatusBar();
    setupLanguageMenu();
    retranslateUi();
}

MainWindow::~MainWindow() {
    delete m_mainWidget;
    m_mainWidget = nullptr;
    delete m_settings;
    m_settings = nullptr;
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

void MainWindow::setupLanguageMenu() {
    m_settingsMenu = menuBar()->addMenu(QString());
    m_languageMenu = m_settingsMenu->addMenu(QString());
    m_languageActionGroup = new QActionGroup(this);
    m_languageActionGroup->setExclusive(true);

    m_chineseAction = m_languageMenu->addAction(QString());
    m_chineseAction->setCheckable(true);
    m_chineseAction->setData("zh_CN");
    m_languageActionGroup->addAction(m_chineseAction);

    m_englishAction = m_languageMenu->addAction(QString());
    m_englishAction->setCheckable(true);
    m_englishAction->setData("en");
    m_languageActionGroup->addAction(m_englishAction);

    connect(m_languageActionGroup, &QActionGroup::triggered, this,
            [this](const QAction *action) { applyLanguage(action->data().toString()); });
}

void MainWindow::applyLanguage(const QString &languageCode, const bool persist) {
    const QString normalized = languageCode.startsWith("zh", Qt::CaseInsensitive) ? "zh_CN" : "en";
    QApplication::removeTranslator(&m_translator);
    if (normalized == "zh_CN") {
        if (!m_translator.load(":/i18n/GameInfer_zh_CN.qm")) {
            std::cerr << "Failed to load embedded Chinese translation." << std::endl;
        } else {
            QApplication::installTranslator(&m_translator);
        }
    }
    m_languageCode = normalized;
    if (persist && m_settings != nullptr) {
        m_settings->setValue("General/uiLanguage", m_languageCode);
    }
    if (m_chineseAction != nullptr) {
        m_chineseAction->setChecked(m_languageCode == "zh_CN");
        m_englishAction->setChecked(m_languageCode == "en");
        retranslateUi();
    }
}

void MainWindow::retranslateUi() {
    setWindowTitle(tr("GameInfer - Audio to MIDI"));
    if (m_settingsMenu != nullptr) {
        m_settingsMenu->setTitle(tr("Settings"));
        m_languageMenu->setTitle(tr("Interface language"));
        m_chineseAction->setText(tr("Simplified Chinese"));
        m_englishAction->setText(tr("English"));
        m_chineseAction->setChecked(m_languageCode == "zh_CN");
        m_englishAction->setChecked(m_languageCode == "en");
    }
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}
