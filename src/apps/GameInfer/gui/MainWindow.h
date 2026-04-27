#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include <dstools/Config.h>
#include <game-infer/Game.h>

class ConfigWidget;
class MainWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupCentralWidget();
    void setupStatusBar();

    QHBoxLayout *m_mainLayout;
    QTabWidget *m_tabWidget;
    ConfigWidget *m_configWidget;
    MainWidget *m_mainWidget;

    QSettings *m_settings;
    dstools::Config m_config;

    std::shared_ptr<Game::Game> m_game;

    // Status bar widgets
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
};

#endif // MAINWINDOW_H