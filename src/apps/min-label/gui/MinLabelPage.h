/// @file MinLabelPage.h
/// @brief MinLabel audio labeling application main page.

#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

#include <QFileSystemModel>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include "Common.h"
#include "MinLabelEditor.h"

#include <MinLabelService.h>
#include <dsfw/AppSettings.h>
#include <dstools/FileStatusDelegate.h>
#include <dstools/ShortcutManager.h>

namespace Minlabel {

    /// @brief IPageActions and IPageLifecycle page providing audio browsing, G2P label editing, file navigation, and dataset export.
    class MinLabelPage : public QWidget,
                         public dstools::labeler::IPageActions,
                         public dstools::labeler::IPageLifecycle {
        Q_OBJECT
        Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

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
        void setWorkingDirectory(const QString &dir) override;
        QString workingDirectory() const override;

        // State
        bool hasUnsavedChanges() const override;
        bool save();

        // IPageActions
        QMenuBar *createMenuBar(QWidget *parent) override;
        QWidget *createStatusBarContent(QWidget *parent) override;
        QString windowTitle() const override;
        bool supportsDragDrop() const override;
        void handleDragEnter(QDragEnterEvent *event) override;
        void handleDrop(QDropEvent *event) override;

        // Settings access for MainWindow
        dstools::AppSettings &settings();
        dstools::widgets::ShortcutManager *shortcutManager() const;

        // IPageLifecycle
        bool onDeactivating() override;
        void onShutdown() override;

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
        MinLabelEditor *m_editor = nullptr;
        QTreeView *treeView = nullptr;
        QFileSystemModel *fsModel = nullptr;
        QSplitter *mainSplitter = nullptr;

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

        void applyConfig();

        void _q_fileMenuTriggered(const QAction *action);
        void _q_editMenuTriggered(const QAction *action) const;
        void _q_playMenuTriggered(const QAction *action) const;
        void _q_helpMenuTriggered(const QAction *action);
        void _q_updateProgress() const;
        void _q_treeCurrentChanged(const QModelIndex &current);
    };

} // namespace Minlabel
