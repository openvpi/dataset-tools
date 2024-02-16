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
#include <QStringList>

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

protected:
    // events
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void showEvent(QShowEvent *event) override;

#ifdef Q_OS_WIN
protected:
    bool nativeEvent(const QByteArray &eventType, void *message,
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        qintptr
#else
        long
#endif
        *result) override;
#endif

public slots:
    void slot_browseOutputDir();
    void slot_addAudioFiles();
    void slot_addFolder();
    void slot_removeListItem();
    void slot_clearAudioList();
    void slot_slicingModeChanged(int index);
    void slot_about();
    void slot_aboutQt();
    void slot_start();
    void slot_saveLogs();
    void slot_oneFinished(const QString &filename, int listIndex);
    void slot_oneInfo(const QString &infomsg);
    void slot_oneError(const QString &errmsg);
    void slot_oneFailed(const QString &errmsg, int listIndex);
    void slot_threadFinished();
    void slot_updateFilenameExample(int value);

private:
    Ui::MainWindow *ui;

    bool m_processing;
    bool m_windowLoaded;
    int m_workTotal;
    int m_workFinished;
    int m_workError;
    QStringList m_failIndex;
    QThreadPool *m_threadpool;

    void warningProcessNotFinished();
    void setProcessing(bool processing);
    void logMessage(const QString &txt);
    void addSingleAudioFile(const QString &fullPath);
    void initStylesMenu();

#ifdef Q_OS_WIN
private:
    ITaskbarList3* m_pTaskbarList3;
    HWND m_hwnd;
#endif
};


#endif //AUDIO_SLICER_MAINWINDOW_H
