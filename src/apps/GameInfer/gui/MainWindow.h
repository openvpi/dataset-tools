#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QSettings>
#include <QTabWidget>
#include <QTranslator>
#include <QVBoxLayout>

#include <game-infer/Game.h>

class ConfigWidget;
class MainWidget;
class QAction;
class QActionGroup;
class QEvent;
class QMenu;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void changeEvent(QEvent *event) override;

private:
    void setupCentralWidget();
    void setupStatusBar();
    void setupLanguageMenu();
    void applyLanguage(const QString &languageCode, bool persist = true);
    void retranslateUi();

    QHBoxLayout *m_mainLayout = nullptr;
    QTabWidget *m_tabWidget = nullptr;
    ConfigWidget *m_configWidget = nullptr;
    MainWidget *m_mainWidget = nullptr;

    QSettings *m_settings = nullptr;
    QTranslator m_translator;
    QString m_languageCode;
    QMenu *m_settingsMenu = nullptr;
    QMenu *m_languageMenu = nullptr;
    QActionGroup *m_languageActionGroup = nullptr;
    QAction *m_chineseAction = nullptr;
    QAction *m_englishAction = nullptr;

    std::shared_ptr<Game::Game> m_game;

    // Status bar widgets
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
};

#endif // MAINWINDOW_H
