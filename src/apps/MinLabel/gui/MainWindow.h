#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include <QSettings>
#include <QSplitter>

#include "Common.h"
#include "PlayWidget.h"
#include "TextWidget.h"

class MainWindow final : public QMainWindow {
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

    QSettings *cfg{};

    void openDirectory(const QString &dirName) const;
    void openFile(const QString &filename) const;
    void saveFile(const QString &filename);
    void labToJson(const QString &dirName);
    void exportAudio(const ExportInfo &exportInfo);

    void reloadWindowTitle();

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    static void initStyleSheet();
    void applyConfig();

    void _q_fileMenuTriggered(const QAction *action);
    void _q_editMenuTriggered(const QAction *action) const;
    void _q_playMenuTriggered(const QAction *action) const;
    void _q_helpMenuTriggered(const QAction *action);
    void _q_updateProgress() const;
    void _q_treeCurrentChanged(const QModelIndex &current);
};

#endif // MAINWINDOW_H
