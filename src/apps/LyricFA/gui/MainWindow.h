#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPluginLoader>
#include <QProgressBar>
#include <QPushButton>

#include "../util/Asr.h"
#include "../util/MatchLyric.h"

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

    QLabel *progressLabel;
    QHBoxLayout *progressLayout;

    void addFiles(const QStringList &paths) const;
    void addFolder(const QString &path) const;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    LyricFA::Asr *m_asr = nullptr;
    MatchLyric *m_match = nullptr;

    static void initStyleSheet();

    void slot_labPath();
    void slot_jsonPath();
    void slot_lyricPath();

    void slot_removeListItem() const;
    void slot_clearTaskList() const;
    void slot_runAsr() const;
    void slot_matchLyric() const;

    void _q_fileMenuTriggered(const QAction *action);
    void _q_helpMenuTriggered(const QAction *action);
};

#endif // MAINWINDOW_H