#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPluginLoader>
#include <QProgressBar>
#include <QPushButton>
#include <QThreadPool>

#include "../util/Fbl.h"

#include <QDoubleSpinBox>

namespace FBL {
    class MainWindow final : public QMainWindow {
        Q_OBJECT
    public:
        explicit MainWindow(QWidget *parent = nullptr);
        ~MainWindow() override;

    protected:
        QMenu *fileMenu;
        QAction *addFileAction;
        QAction *addFolderAction;

        QMenu *helpMenu;
        QAction *aboutAppAction;
        QAction *aboutQtAction;

        QWidget *mainWidget;
        QListWidget *taskList;
        QVBoxLayout *mainLayout;
        QHBoxLayout *listLayout;
        QHBoxLayout *btnLayout;

        QPushButton *remove;
        QPushButton *clear;
        QPushButton *runFbl;

        QPlainTextEdit *out;
        QProgressBar *progressBar;

        QVBoxLayout *rightLayout;
        QHBoxLayout *rawTgLayout;
        QHBoxLayout *outTgLayout;

        QHBoxLayout *apThreshLayout;
        QHBoxLayout *apDurLayout;
        QHBoxLayout *spDurLayout;

        QLineEdit *rawTgEdit;
        QLineEdit *outTgEdit;

        QDoubleSpinBox *ap_threshold;
        QDoubleSpinBox *ap_dur;
        QDoubleSpinBox *sp_dur;

        QCheckBox *pinyinBox;

        QLabel *progressLabel;
        QHBoxLayout *progressLayout;

        void addFiles(const QStringList &paths) const;
        void addFolder(const QString &path) const;

        void dragEnterEvent(QDragEnterEvent *event) override;
        void dropEvent(QDropEvent *event) override;
        void closeEvent(QCloseEvent *event) override;

    private:
        FBL *m_fbl = nullptr;

        int m_workTotal = 0;
        int m_workFinished = 0;
        int m_workError = 0;
        QStringList m_failIndex;
        QThreadPool *m_threadpool;

        static void initStyleSheet();

        void slot_rawTgPath();
        void slot_outTgPath();
        void slot_lyricPath();

        void slot_removeListItem() const;
        void slot_clearTaskList() const;
        void slot_runFbl();

        void slot_oneFailed(const QString &filename, const QString &msg);
        void slot_oneFinished(const QString &filename, const QString &msg);
        void slot_threadFinished();

        void _q_fileMenuTriggered(const QAction *action);
        void _q_helpMenuTriggered(const QAction *action);
    };
}
#endif // MAINWINDOW_H
