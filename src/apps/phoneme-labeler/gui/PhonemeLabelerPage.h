/// @file PhonemeLabelerPage.h
/// @brief PhonemeLabeler TextGrid editor main page.

#pragma once

#include <dsfw/IPageActions.h>

#include <QWidget>

#include <dsfw/AppSettings.h>
#include <dstools/ShortcutManager.h>

#include "../PhonemeLabelerKeys.h"
#include <PhonemeEditor.h>
#include "ui/FileListPanel.h"

namespace dstools {
namespace phonemelabeler {

/// @brief IPageActions page providing waveform/spectrogram/power visualization, multi-tier TextGrid editing with undo/redo, boundary binding, and playback.
class PhonemeLabelerPage : public QWidget, public dstools::labeler::IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    explicit PhonemeLabelerPage(QWidget *parent = nullptr);
    ~PhonemeLabelerPage() override;

    void setWorkingDirectory(const QString &dir) override;
    [[nodiscard]] QString workingDirectory() const override { return m_currentDir; }

    void openFile(const QString &path);

    [[nodiscard]] QMenu *spectrogramColorMenu() const { return m_editor->spectrogramColorMenu(); }
    [[nodiscard]] dstools::widgets::ShortcutManager *shortcutManager() const { return m_shortcutManager; }
    [[nodiscard]] dstools::AppSettings &settings() { return m_settings; }

    [[nodiscard]] QUndoStack *undoStack() const { return m_editor->undoStack(); }
    [[nodiscard]] QString currentFilePath() const { return m_currentFilePath; }
    [[nodiscard]] bool hasUnsavedChanges() const override;
    bool save();
    bool saveAs();

    // IPageActions
    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;
    QString windowTitle() const override;
    bool maybeSave();

    [[nodiscard]] QToolBar *toolbar() const { return m_editor->toolbar(); }

signals:
    void workingDirectoryChanged(const QString &dir);
    void fileSelected(const QString &path);
    void documentLoaded(const QString &path);
    void documentSaved(const QString &path);

private:
    dstools::AppSettings m_settings{"PhonemeLabeler"};

    // Editor widget (owns all visualization/editing)
    PhonemeEditor *m_editor = nullptr;

    // File list panel (left side)
    FileListPanel *m_fileListPanel = nullptr;

    // File state
    QString m_currentDir;
    QString m_currentFilePath;

    // Services
    dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;

    // Layout
    QSplitter *m_outerSplitter = nullptr;

    // File operations
    void onFileSelected(const QString &path);
    bool saveFile();
    bool saveFileAs();

    void applyConfig();
};

} // namespace phonemelabeler
} // namespace dstools
