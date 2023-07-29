#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QBoxLayout>
#include <QFileSystemModel>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPluginLoader>
#include <QPushButton>
#include <QSet>
#include <QSlider>
#include <QSplitter>
#include <QTreeWidget>
#include <QListWidget>

#include "PlayWidget.h"
#include "F0Widget.h"

#include "Api/IAudioDecoder.h"
#include "Api/IAudioPlayback.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    QMenu *fileMenu;
    QAction *browseAction;

    QMenu *editMenu;
    QAction *nextAction;
    QAction *prevAction;

    QMenu *playMenu;
    QAction *playAction;

    QMenu *helpMenu;
    QAction *aboutAppAction;
    QAction *aboutQtAction;

    int notifyTimerId;
    bool playing;
    QString dirname;

    PlayWidget *playerWidget;
    F0Widget *f0Widget;

    QTreeView *treeView;
    QFileSystemModel *fsModel;
    QListWidget *sentenceWidget;

    QSplitter *mainSplitter;
    QSplitter *fsSentencesSplitter;

    QVBoxLayout *rightLayout;
    QWidget *rightWidget;

    QString lastFile;
    bool fileSwitchDirection = true; // true = next, false = prev

    // Cached DS file content
    // We only store DS's first layer of objects, anything else is parsed on the fly
    QVector<QJsonObject> dsContent;
    int currentRow = -1;

    // Cached application configuration
    SlurCutterCfg cfg;

    void openDirectory(const QString &dirname);
    void openFile(const QString &filename);
    bool saveFile(const QString &filename);

    void pullEditedMidi();
    void switchFile(bool next);
    void switchSentence(bool next);

    void loadDsContent(const QString &content);
    Q_SLOT void reloadDsSentenceRequested();

    void reloadWindowTitle();

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void initStyleSheet();

    void _q_fileMenuTriggered(QAction *action);
    void _q_editMenuTriggered(QAction *action);
    void _q_playMenuTriggered(QAction *action);
    void _q_helpMenuTriggered(QAction *action);
    void _q_treeCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
    void _q_sentenceChanged(int currentRow);
};

#endif // MAINWINDOW_H
