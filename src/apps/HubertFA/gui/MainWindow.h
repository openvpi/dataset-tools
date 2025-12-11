#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QBoxLayout>
#include <QButtonGroup>
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
#include <QTextCharFormat> // 添加这行
#include <QThreadPool>

#include "../util/Hfa.h"

namespace HFA {
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
        QPushButton *runHfa;

        QLineEdit *outTgEdit;

        QHBoxLayout *languageLayout;
        QButtonGroup *languageGroup;
        QHBoxLayout *nonSpeechPhLayout;

        QVBoxLayout *rightLayout;
        QHBoxLayout *outTgLayout;

        QPlainTextEdit *out;
        QProgressBar *progressBar;

        QLabel *progressLabel;
        QHBoxLayout *progressLayout;

        void addFiles(const QStringList &paths) const;
        void addFolder(const QString &path) const;

        void dragEnterEvent(QDragEnterEvent *event) override;
        void dropEvent(QDropEvent *event) override;
        void closeEvent(QCloseEvent *event) override;

    private:
        HFA *m_hfa = nullptr;

        int m_workTotal = 0;
        int m_workFinished = 0;
        int m_workError = 0;
        QStringList m_failIndex;
        QThreadPool *m_threadpool;

        QTextCharFormat m_errorFormat;

        void slot_outTgPath();

        void slot_removeListItem() const;
        void slot_clearTaskList() const;
        void slot_runHfa();

        void slot_oneFailed(const QString &filename, const QString &msg);
        void slot_oneFinished(const QString &filename, const QString &msg);
        void slot_threadFinished();

        void _q_fileMenuTriggered(const QAction *action);
        void _q_helpMenuTriggered(const QAction *action);

        void appendErrorMessage(const QString &message) const;
    };
}
#endif // MAINWINDOW_H