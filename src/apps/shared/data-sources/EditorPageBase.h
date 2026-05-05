#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/AppSettings.h>
#include <dstools/ShortcutManager.h>
#include <dstools/IEditorDataSource.h>

#include <QWidget>
#include <QString>

class QAction;
class QMenuBar;
class QSplitter;

namespace dstools {

class ISettingsBackend;
class SliceListPanel;

/// @brief Base class for shared editor pages (MinLabel, Phoneme, Pitch).
///
/// Extracts common boilerplate: SliceListPanel + QSplitter layout,
/// setDataSource, navigation actions, lifecycle (splitter save/restore,
/// ensureSelection), and slice change bookkeeping.
///
/// Subclasses implement domain-specific logic via virtual hooks.
class EditorPageBase : public QWidget,
                       public labeler::IPageActions,
                       public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit EditorPageBase(const QString &settingsGroup, QWidget *parent = nullptr);
    ~EditorPageBase() override;

    void setDataSource(IEditorDataSource *source, ISettingsBackend *settingsBackend);

    IEditorDataSource *source() const { return m_source; }
    ISettingsBackend *settingsBackend() const { return m_settingsBackend; }
    const QString &currentSliceId() const { return m_currentSliceId; }

    // IPageActions — default implementations
    QString windowTitle() const override;
    bool hasUnsavedChanges() const override;
    bool supportsDragDrop() const override { return true; }
    void handleDragEnter(QDragEnterEvent *event) override;
    void handleDrop(QDropEvent *event) override;

    // IPageLifecycle — common implementations
    void onActivated() override;
    bool onDeactivating() override;
    void onDeactivated() override;
    void onShutdown() override;

    dstools::widgets::ShortcutManager *shortcutManager() const { return m_shortcutManager; }

signals:
    void sliceChanged(const QString &sliceId);

protected:
    // ── Accessors for subclass use ──
    SliceListPanel *sliceListPanel() const { return m_sliceList; }
    QSplitter *splitter() const { return m_splitter; }
    dstools::AppSettings &settings() { return m_settings; }
    const dstools::AppSettings &settings() const { return m_settings; }

    QAction *prevAction() const { return m_prevAction; }
    QAction *nextAction() const { return m_nextAction; }

    void setCurrentSliceId(const QString &id) { m_currentSliceId = id; }

    // ── Setup helpers (call from subclass constructor) ──

    /// Set up the standard SliceListPanel + QSplitter + editor layout.
    /// @param editorWidget The main editor widget to place in the splitter's right pane.
    void setupBaseLayout(QWidget *editorWidget);

    /// Create prev/next navigation actions and bind them to the shortcut manager.
    void setupNavigationActions();

    // ── Subclass must implement ──

    /// Return the title prefix (e.g. "歌词标注", "音素标注", "音高标注").
    virtual QString windowTitlePrefix() const = 0;

    /// Check if the current slice has unsaved changes.
    virtual bool isDirty() const = 0;

    /// Save the current slice data. Return true on success.
    virtual bool saveCurrentSlice() = 0;

    /// Handle a slice being selected (load data into editor).
    virtual void onSliceSelectedImpl(const QString &sliceId) = 0;

    // ── Optional hooks ──

    /// Called during onActivated() after common setup, for auto-inference etc.
    virtual void onAutoInfer() {}

    /// Save additional splitters (e.g. PhonemeEditor's internal splitter).
    virtual void saveExtraSplitters() {}

    /// Restore additional splitters.
    virtual void restoreExtraSplitters() {}

    /// Called during onDeactivated() for cleanup (e.g. release engine references).
    virtual void onDeactivatedImpl() {}

    /// Called during onShutdown() for final save (e.g. shortcut persistence).
    virtual void onShutdownImpl() {}

    // ── Common utility ──

    /// Show Save/Discard/Cancel dialog and act accordingly.
    /// Uses isDirty() to decide whether to show dialog.
    bool maybeSave();

private:
    SliceListPanel *m_sliceList = nullptr;
    QSplitter *m_splitter = nullptr;
    IEditorDataSource *m_source = nullptr;
    ISettingsBackend *m_settingsBackend = nullptr;
    QString m_currentSliceId;

    dstools::AppSettings m_settings;
    dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;

    QAction *m_prevAction = nullptr;
    QAction *m_nextAction = nullptr;

    void onSliceSelected(const QString &sliceId);
};

} // namespace dstools
