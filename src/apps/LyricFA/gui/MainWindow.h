#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QCheckBox>
#include <QPluginLoader>
#include <QProgressBar>
#include <QPushButton>
#include <QThreadPool>

#include "../util/Asr.h"
#include "../util/MatchLyric.h"

namespace LyricFA {
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
        QPushButton *runAsr;
        QPushButton *matchLyric;

        QPlainTextEdit *out;
        QProgressBar *progressBar;

        QVBoxLayout *rightLayout;
        QHBoxLayout *labLayout;
        QHBoxLayout *jsonLayout;
        QHBoxLayout *lyricLayout;

        QLineEdit *labEdit;
        QLineEdit *jsonEdit;
        QLineEdit *lyricEdit;

        QCheckBox *pinyinBox;

        QLabel *progressLabel;
        QHBoxLayout *progressLayout;

        void addFiles(const QStringList &paths) const;
        void addFolder(const QString &path) const;

        void dragEnterEvent(QDragEnterEvent *event) override;
        void dropEvent(QDropEvent *event) override;
        void closeEvent(QCloseEvent *event) override;

    private:
        Asr *m_asr = nullptr;
        QSharedPointer<IKg2p::MandarinG2p> m_mandarin = nullptr;
        MatchLyric *m_match = nullptr;

        int m_workTotal = 0;
        int m_workFinished = 0;
        int m_workError = 0;
        QStringList m_failIndex;
        QThreadPool *m_threadpool;

        static void initStyleSheet();

        void slot_labPath();
        void slot_jsonPath();
        void slot_lyricPath();

        void slot_removeListItem() const;
        void slot_clearTaskList() const;
        void slot_runAsr();
        void slot_matchLyric();

        void slot_oneFailed(const QString &filename, const QString &msg);
        void slot_oneFinished(const QString &filename, const QString &msg);
        void slot_threadFinished();

        void _q_fileMenuTriggered(const QAction *action);
        void _q_helpMenuTriggered(const QAction *action);
    };
}
#endif // MAINWINDOW_H
