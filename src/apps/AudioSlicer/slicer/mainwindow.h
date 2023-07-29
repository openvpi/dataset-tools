#ifndef AUDIO_SLICER_MAINWINDOW_H
#define AUDIO_SLICER_MAINWINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QThreadPool>
#include <QEvent>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#ifdef Q_OS_WIN
#include <ShlObj.h>
#endif


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() override;

    // events
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

public slots:
    void slot_browseOutputDir();
    void slot_addAudioFiles();
    void slot_addFolder();
    void slot_clearAudioList();
    void slot_about();
    void slot_aboutQt();
    void slot_start();
    void slot_saveLogs();
    void slot_oneFinished(const QString &filename);
    void slot_oneInfo(const QString &infomsg);
    void slot_oneError(const QString &errmsg);
    void slot_threadFinished();

private:
    Ui::MainWindow *ui;

    bool m_processing;
    int m_workTotal;
    int m_workFinished;
    int m_workError;
    QThreadPool *m_threadpool;

    void warningProcessNotFinished();
    void setProcessing(bool processing);
    void logMessage(const QString &txt);
    void addSingleAudioFile(const QString &fullPath);

#ifdef Q_OS_WIN
    private:
    ITaskbarList3* m_pTaskbarList3;
    HWND m_hwnd;
#endif
};


#endif //AUDIO_SLICER_MAINWINDOW_H
