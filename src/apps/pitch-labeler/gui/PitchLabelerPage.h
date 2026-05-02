#pragma once

#include <dsfw/IPageActions.h>

#include <QSplitter>
#include <QWidget>

#include <dsfw/AppSettings.h>
#include <dstools/ShortcutManager.h>

#include <PitchEditor.h>

#include <memory>
#include <vector>

namespace dstools {
namespace pitchlabeler {

class DSFile;
class PitchFileService;

namespace ui {
class FileListPanel;
}

/// Reusable page widget containing all PitchLabeler UI and logic.
/// MainWindow becomes a thin shell providing menus, status bar, and window chrome.
class PitchLabelerPage : public QWidget, public dstools::labeler::IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    explicit PitchLabelerPage(QWidget *parent = nullptr);
    ~PitchLabelerPage() override;

    void setWorkingDirectory(const QString &dir) override;
    void closeDirectory();
    [[nodiscard]] QString workingDirectory() const override { return m_workingDirectory; }

    [[nodiscard]] bool hasUnsavedChanges() const override;
    bool save();
    bool saveAll();

    // IPageActions
    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;
    QString windowTitle() const override;

    void openDirectory();

    // Access for MainWindow shortcut binding
    [[nodiscard]] QAction *saveAction() const { return m_editor->saveAction(); }
    [[nodiscard]] QAction *saveAllAction() const { return m_editor->saveAllAction(); }
    [[nodiscard]] QAction *undoAction() const { return m_editor->undoAction(); }
    [[nodiscard]] QAction *redoAction() const { return m_editor->redoAction(); }
    [[nodiscard]] QAction *zoomInAction() const { return m_editor->zoomInAction(); }
    [[nodiscard]] QAction *zoomOutAction() const { return m_editor->zoomOutAction(); }
    [[nodiscard]] QAction *zoomResetAction() const { return m_editor->zoomResetAction(); }
    [[nodiscard]] QAction *abCompareAction() const { return m_editor->abCompareAction(); }

    [[nodiscard]] dstools::widgets::ShortcutManager *shortcutManager() const { return m_shortcutManager; }
    [[nodiscard]] dstools::AppSettings &settings() { return m_settings; }

    // Config persistence
    void saveConfig();

signals:
    void workingDirectoryChanged(const QString &dir);
    void fileSelected(const QString &path);
    void modificationChanged(bool modified);
    void fileLoaded(const QString &path);
    void fileSaved(const QString &path);
    void fileStatusChanged(const QString &fileName);

private:
    // Settings
    dstools::AppSettings m_settings{"DiffSingerPitchLabeler"};

    // Editor widget (owns PianoRoll, PropertyPanel, toolbar, playback)
    PitchEditor *m_editor = nullptr;

    // File management
    QString m_workingDirectory;
    PitchFileService *m_fileService = nullptr;
    std::shared_ptr<DSFile> m_currentFile;
    QString m_currentFilePath;

    // Services
    dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;

    // Layout
    QSplitter *m_outerSplitter = nullptr;
    ui::FileListPanel *m_fileListPanel = nullptr;

    // A/B comparison state (kept at page level since it needs originalF0)
    std::vector<int32_t> m_originalF0;

    // Shortcuts
    QList<QShortcut *> m_windowShortcuts;

    // Helpers
    void applyConfig();
    void rebuildWindowShortcuts();

    // File operations
    void onFileLoaded(const QString &path, std::shared_ptr<DSFile> file);
    void loadFile(const QString &path);
    bool saveFile();
    bool saveAllFiles();

    // File navigation
    void onNextFile();
    void onPrevFile();

    // Status
    void updateStatusInfo();
};

} // namespace pitchlabeler
} // namespace dstools
