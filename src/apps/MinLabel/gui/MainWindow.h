#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileSystemModel>
#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>

#include "Common.h"
#include <dstools/AppSettings.h>
#include <dstools/PlayWidget.h>
#include <dstools/ShortcutManager.h>
#include <dstools/FileStatusDelegate.h>
#include <dstools/FileProgressTracker.h>
#include "TextWidget.h"

namespace Minlabel {
    class MainWindow final : public QMainWindow {
        Q_OBJECT
    public:
        explicit MainWindow(QWidget *parent = nullptr);
        ~MainWindow() override;

    protected:
        QMenu *fileMenu;
        QAction *browseAction;
        QAction *convertAction;
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

        dstools::widgets::PlayWidget *playerWidget;
        TextWidget *textWidget;

        QTreeView *treeView;
        QFileSystemModel *fsModel;

        dstools::widgets::FileProgressTracker *m_fileProgress;

        QSplitter *mainSplitter;

        QVBoxLayout *rightLayout;
        QWidget *rightWidget;

        QString lastFile;

        dstools::AppSettings m_settings;
        dstools::widgets::ShortcutManager *m_shortcutManager;

        void openDirectory(const QString &dirName) const;
        void openFile(const QString &filename) const;
        bool saveFile(const QString &filename);
        // Returns false on failure but does not exit (BUG-001 fix)
        void labToJson(const QString &dirName);
        void exportAudio(const ExportInfo &exportInfo);

        void reloadWindowTitle();

        void dragEnterEvent(QDragEnterEvent *event) override;
        void dropEvent(QDropEvent *event) override;
        void closeEvent(QCloseEvent *event) override;

    private:
        void applyConfig();

        void _q_fileMenuTriggered(const QAction *action);
        void _q_editMenuTriggered(const QAction *action) const;
        void _q_playMenuTriggered(const QAction *action) const;
        void _q_helpMenuTriggered(const QAction *action);
        void _q_updateProgress() const;
        void _q_treeCurrentChanged(const QModelIndex &current);
    };
}
#endif // MAINWINDOW_H
