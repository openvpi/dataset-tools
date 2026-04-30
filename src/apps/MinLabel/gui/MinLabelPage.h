#pragma once

#include <QFileSystemModel>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include "Common.h"
#include "TextWidget.h"

#include <dsfw/AppSettings.h>
#include <dstools/FileProgressTracker.h>
#include <dstools/FileStatusDelegate.h>
#include <dstools/PlayWidget.h>
#include <dstools/ShortcutManager.h>

namespace Minlabel {

    class MinLabelPage : public QWidget {
        Q_OBJECT
    public:
        explicit MinLabelPage(QWidget *parent = nullptr);
        ~MinLabelPage() override;

        // Actions for menus
        QAction *browseAction() const;
        QAction *convertAction() const;
        QAction *exportAction() const;
        QAction *nextAction() const;
        QAction *prevAction() const;
        QAction *playAction() const;

        // Working directory
        void setWorkingDirectory(const QString &dir);
        QString workingDirectory() const;

        // State
        bool hasUnsavedChanges() const;
        bool save();

        // Settings access for MainWindow
        dstools::AppSettings &settings();
        dstools::widgets::ShortcutManager *shortcutManager() const;

    signals:
        void workingDirectoryChanged(const QString &dir);

    private:
        // Actions
        QAction *m_browseAction = nullptr;
        QAction *m_convertAction = nullptr;
        QAction *m_exportAction = nullptr;
        QAction *m_nextAction = nullptr;
        QAction *m_prevAction = nullptr;
        QAction *m_playAction = nullptr;

        // Widgets
        dstools::widgets::PlayWidget *playerWidget = nullptr;
        TextWidget *textWidget = nullptr;
        QTreeView *treeView = nullptr;
        QFileSystemModel *fsModel = nullptr;
        dstools::widgets::FileProgressTracker *m_fileProgress = nullptr;
        QSplitter *mainSplitter = nullptr;
        QWidget *rightWidget = nullptr;
        QVBoxLayout *rightLayout = nullptr;

        // State
        QString dirname;
        QString lastFile;
        bool playing = false;
        bool m_isDirty = false;

        // Services
        dstools::AppSettings m_settings;
        dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;

        // Methods
        void openDirectory(const QString &dirName) const;
        void openFile(const QString &filename) const;
        bool saveFile(const QString &filename);
        void labToJson(const QString &dirName);
        void exportAudio(const ExportInfo &exportInfo);

        static QString removeTone(const QString &labContent);

        void applyConfig();

        void _q_fileMenuTriggered(const QAction *action);
        void _q_editMenuTriggered(const QAction *action) const;
        void _q_playMenuTriggered(const QAction *action) const;
        void _q_helpMenuTriggered(const QAction *action);
        void _q_updateProgress() const;
        void _q_treeCurrentChanged(const QModelIndex &current);
    };

} // namespace Minlabel
