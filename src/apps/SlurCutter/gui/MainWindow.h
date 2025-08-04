#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QListWidget>
#include <QMainWindow>
#include <QSplitter>

#include "F0Widget.h"
#include "PlayWidget.h"

#include <QSettings>
#include <QStyledItemDelegate>


namespace SlurCutter {
    class CustomDelegate final : public QStyledItemDelegate {
    public:
        explicit CustomDelegate(const QSet<QString> &editedFiles, QObject *parent = nullptr);

        void setEditedFiles(const QSet<QString> &editedFiles);

    private:
        QSet<QString> m_editedFiles;

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    };

    class MainWindow final : public QMainWindow {
        Q_OBJECT
    public:
        explicit MainWindow(QWidget *parent = nullptr);
        ~MainWindow() override;

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
        QSet<QString> editedFiles;

        PlayWidget *playerWidget;
        F0Widget *f0Widget;

        CustomDelegate *m_delegate;
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
        QSettings *cfg{};

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
        static void initStyleSheet();
        void applyConfig();

        void _q_fileMenuTriggered(const QAction *action);
        void _q_editMenuTriggered(const QAction *action);
        void _q_playMenuTriggered(const QAction *action) const;
        void _q_helpMenuTriggered(const QAction *action);
        void _q_treeCurrentChanged(const QModelIndex &current);
        void _q_sentenceChanged(int currentRow);
    };
}
#endif // MAINWINDOW_H
