#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QBoxLayout>
#include <QCheckBox>
#include <QFileSystemModel>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPluginLoader>
#include <QProgressBar>
#include <QPushButton>
#include <QSet>
#include <QSlider>
#include <QSplitter>
#include <QTreeWidget>

#include "Common.h"
#include "ExportDialog.h"
#include "PlayWidget.h"
#include "TextWidget.h"
#include "inc/MinLabelCfg.h"

#include "Api/IAudioDecoder.h"
#include "Api/IAudioPlayback.h"

class MainWindow final: public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    QMenu *fileMenu;
    QAction *browseAction;
    QAction *covertAction;
    QAction *exportAction;

    QMenu *editMenu;
    QAction *nextAction;
    QAction *prevAction;

    QMenu *playMenu;
    QAction *playAction;

    QMenu *helpMenu;
    QAction *aboutAppAction;
    QAction *aboutQtAction;

    bool playing;
    QString dirname;

    PlayWidget *playerWidget;
    TextWidget *textWidget;

    QTreeView *treeView;
    QFileSystemModel *fsModel;

    QProgressBar *progressBar;

    QSplitter *mainSplitter;

    QVBoxLayout *rightLayout;
    QLabel *progressLabel;
    QHBoxLayout *progressLayout;
    QWidget *rightWidget;

    QString lastFile;

    MinLabelCfg cfg;

    void openDirectory(const QString &dirName);
    void openFile(const QString &filename);
    void saveFile(const QString &filename);
    void labToJson(const QString &dirName);
    void exportAudio(ExportInfo &exportInfo);

    void reloadWindowTitle();

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    static void initStyleSheet();
    void applyConfig();

    void _q_fileMenuTriggered(QAction *action);
    void _q_editMenuTriggered(QAction *action);
    void _q_playMenuTriggered(QAction *action);
    void _q_helpMenuTriggered(QAction *action);
    void _q_updateProgress();
    void _q_treeCurrentChanged(const QModelIndex &current);
};

#endif // MAINWINDOW_H
