#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHBoxLayout>
#include <QMainWindow>
#include <QProgressBar>
#include <QVBoxLayout>

#include <dsfw/AppSettings.h>

class MainWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupCentralWidget();
    void setupMenuBar();
    void setupStatusBar();

    QHBoxLayout *m_mainLayout;
    MainWidget *m_mainWidget;

    dstools::AppSettings m_settings;

    // Status bar widgets
    QProgressBar *m_progressBar;
};

#endif // MAINWINDOW_H