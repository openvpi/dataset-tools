#pragma once

#include "StatusBarBuilder.h"

#include <QPointer>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <QJsonObject>
#include <QMenu>
#include <QtConcurrent>
#include <atomic>
#include <tuple>
#include <dsfw/AppSettings.h>
#include <dsfw/IModelProvider.h>
#include <dstools/ModelManager.h>
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/Log.h>
#include <dsfw/widgets/PageBanner.h>
#include <dsfw/widgets/PipelineStatusBar.h>
#include <dstools/DsTextTypes.h>
#include <dstools/IEditorDataSource.h>
#include <dstools/Result.h>
#include <dsfw/widgets/ShortcutManager.h>
#include "IEnginePoolHost.h"
#include <functional>
#include <map>
#include <memory>

class QAction;
class QMenuBar;
class QSplitter;
class QLabel;
class QHBoxLayout;

namespace dstools {

    class AppSettingsBackend;
    class SliceListPanel;
    class BatchProcessDialog;
    class ModelManager;
    class EnginePool;

    struct BatchSliceResult {
        enum Status { Processed, Skipped, Error };
        Status status = Error;
        QString logMessage;
        QString error;
    };

    struct BatchConfig {
        QString dialogTitle;
        bool defaultSkipExisting = true;
        QString skipExistingLabel;
    };

    class EditorPageBase : public QWidget, public IEnginePoolHost, public labeler::IPageActions, public labeler::IPageLifecycle {
        Q_OBJECT
        Q_INTERFACES(dstools::IEnginePoolHost dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

    public:
        explicit EditorPageBase(const QString &settingsGroup, QWidget *parent = nullptr);
        ~EditorPageBase() override;

        void setDataSource(IEditorDataSource *source, AppSettingsBackend *settingsBackend);

        EnginePool *enginePool() const {
            return m_enginePool.get();
        }

        QObject *asQObject() override {
            return this;
        }

        IEditorDataSource *source() const {
            return m_source;
        }
        AppSettingsBackend *settingsBackend() const {
            return m_settingsBackend;
        }
        const QString &currentSliceId() const {
            return m_currentSliceId;
        }

        // IPageActions — default implementations
        QString windowTitle() const override;
        bool hasUnsavedChanges() const override;
        bool supportsDragDrop() const override {
            return true;
        }
        void handleDragEnter(QDragEnterEvent *event) override;
        void handleDrop(QDropEvent *event) override;

        // IPageLifecycle — common implementations
        void onActivated() override;
        bool onDeactivating() override;
        void onDeactivated() override;
        void onShutdown() override;

        dsfw::widgets::ShortcutManager *shortcutManager() const {
            return m_shortcutManager;
        }

    signals:
        void sliceChanged(const QString &sliceId);

    protected:
        // ── Accessors for subclass use ──
        SliceListPanel *sliceListPanel() const {
            return m_sliceList;
        }
        QSplitter *splitter() const {
            return m_splitter;
        }
        dstools::AppSettings &settings() {
            return m_settings;
        }
        const dstools::AppSettings &settings() const {
            return m_settings;
        }

        QAction *prevAction() const {
            return m_prevAction;
        }
        QAction *nextAction() const {
            return m_nextAction;
        }

        void setCurrentSliceId(const QString &id) {
            m_currentSliceId = id;
        }

        ModelManager *ensureModelManager() override;

        // ── Engine token management ──

        /// Get or create an alive token for the given engine key.
        EngineAliveToken &aliveToken(const QString &key) override {
            return m_aliveTokens[key];
        }

        /// Look up an alive token without creating (const version).
        /// Returns a default (invalid) token if not found.
        const EngineAliveToken &aliveToken(const QString &key) const {
            static const EngineAliveToken kEmpty;
            auto it = m_aliveTokens.find(key);
            return it != m_aliveTokens.end() ? it->second : kEmpty;
        }

        // ── Setup helpers (call from subclass constructor) ──

        /// Set up the standard SliceListPanel + QSplitter + editor layout.
        /// @param editorWidget The main editor widget to place in the splitter's right pane.
        void setupBaseLayout(QWidget *editorWidget);

        /// Create prev/next navigation actions and bind them to the shortcut manager.
        void setupNavigationActions();

        // ── Standard UI helpers (P3-A3) ──

        /// Add standard File menu (saveAction + separator + exit).
        /// @return File menu for subclasses to add page-specific actions.
        QMenu *addStandardFileMenu(QMenuBar *bar, QAction *saveAction);

        /// Add standard Edit menu (undo, redo, navigation, shortcut settings).
        /// @return Edit menu for subclasses to add page-specific actions.
        QMenu *addStandardEditMenu(QMenuBar *bar, QAction *undoAction, QAction *redoAction);

        /// Add standard View menu (zoom actions + theme).
        /// @param zoomActions  {zoomIn, zoomOut, zoomReset} — pass {} to skip zoom.
        /// @return View menu for subclasses to add page-specific actions.
        QMenu *addStandardViewMenu(QMenuBar *bar, const QList<QAction *> &zoomActions = {});

        /// Create standard status bar widgets (slice label + dirty label).
        /// @return The StatusBarBuilder for adding page-specific widgets.
        /// @param outSliceLabel  Set to the created slice label.
        /// @param outDirtyLabel  Set to the created dirty status label.
        StatusBarBuilder addStandardStatusWidgets(QWidget *parent,
                                                   QLabel **outSliceLabel,
                                                   QLabel **outDirtyLabel);

        /// Build a standard menu bar with File and Edit menus.
        /// Returns the bar so subclasses can add page-specific menus (Process, View, Tools, etc.).
        QMenuBar *buildStandardMenuBar(QWidget *parent, QAction *saveAction, QAction *undoAction, QAction *redoAction);

        /// Build standard status bar content with slice/dirty labels.
        /// @param container The status bar container widget (page-owned, will get layout set).
        /// @return StatusBarBuilder for adding page-specific labels (position, zoom, etc.).
        StatusBarBuilder buildStandardStatusBar(QWidget *container);

        /// Create and return a pipeline status bar. Caller owns the widget.
        /// Subclasses should call setupPipelineStatusBar() to configure steps.
        dsfw::widgets::PipelineStatusBar *createPipelineStatusBar();

        /// Configure pipeline steps on the status bar.
        /// @param bar The status bar to configure.
        virtual void setupPipelineSteps(dsfw::widgets::PipelineStatusBar *bar);

        /// Standard auto-infer preload phase: updateProgress() + preload engine config.
        /// Called by onAutoInfer() template method before the manual-dirty check.
        void runStandardAutoInferPreload();

        /// Get preload engine config from settings (convenience accessor).
        QJsonObject preloadConfig() const;

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
        /// Template method: preload -> guard check -> dirty check -> manual-dirty banner -> process dirty.
        /// Subclasses override onAutoInferPreloadEngines() and/or onAutoInferProcessDirty().
        virtual void onAutoInfer();

        /// Override to preload engines before dirty layer processing (e.g. acquire pitch, midi, FA engines).
        virtual void onAutoInferPreloadEngines();

        /// Override to handle page-specific dirty layers (e.g. run pitch extraction, MIDI transcription, FA).
        virtual void onAutoInferProcessDirty(const QStringList &dirty);

        /// Show progress notification during auto-infer step execution.
        /// @param stepName Human-readable step name (e.g. "Pitch Extraction").
        void showAutoInferProgress(const QString &stepName);

        /// Show completion notification after auto-infer step succeeds.
        /// @param stepName Human-readable step name.
        void showAutoInferComplete(const QString &stepName);

        /// Show failure notification with retry/skip options.
        /// @param stepName Human-readable step name.
        /// @param error Error description.
        /// @return User choice: 0=retry, 1=skip, -1=dismissed.
        int showAutoInferFailed(const QString &stepName, const QString &error);

        /// Convenience: show a PageBanner at the top of the editor.
        void showBanner(dsfw::widgets::BannerType type, const QString &message, const QStringList &buttonLabels = {});
        void hideBanner();

        /// Propagate dirty downstream layers after saving modified layers.
        /// Calls source()->addDirtyLayers() which triggers propagateDirty + persist.
        void markLayersModified(const QStringList &layers);

        /// Record a pipeline step as manually edited (for export validation).
        /// Calls source()->addEditedStep().
        void addEditedStep(const QString &step);

        /// Mark a layer as manually edited to protect from auto-recompute.
        /// Calls source()->setLayerManuallyEdited().
        void setLayerManuallyEdited(const QString &layer, bool edited = true);

        /// Compute dirty status text for status bar display.
        /// Format: "✅ 数据就绪" or "⏳ 2 层待更新: phoneme, ph_num"
        QString computeDirtyStatusText(const QString &sliceId) const;

        /// Update a QLabel with dirty status text for the current slice.
        /// @param label  Label to update (if null, no-op).
        void updateDirtyStatusLabel(QLabel *label);

        /// Save additional splitters (e.g. PhonemeEditor's internal splitter).
        virtual void saveExtraSplitters() {
        }

        /// Restore additional splitters.
        virtual void restoreExtraSplitters() {
        }

        /// Called during onDeactivated() for cleanup (e.g. release engine references).
        virtual void onDeactivatedImpl() {
        }

        /// Called during onShutdown() for final save (e.g. shortcut persistence).
        virtual void onShutdownImpl() {
        }

        // ── Batch processing template (P-12) ──

        [[nodiscard]] virtual bool hasExistingResult(const QString &sliceId) const;
        [[nodiscard]] virtual bool prepareSliceInput(const QString &sliceId, QString &skipReason);
        virtual BatchSliceResult processSlice(const QString &sliceId) = 0;
        [[nodiscard]] virtual void applyBatchResult(const QString &sliceId, const BatchSliceResult &result);

        bool isBatchRunning() const {
            return m_batchRunning;
        }
        void setBatchRunning(bool running) {
            if (m_batchRunning.exchange(running) != running)
                onBatchRunningChanged(running);
        }

        std::shared_ptr<std::atomic<bool>> batchAliveToken() const {
            return aliveToken(m_batchTaskKey).token();
        }

        void setBatchTaskKey(const QString &key) {
            m_batchTaskKey = key;
        }

        virtual void onBatchRunningChanged(bool running) {
            Q_UNUSED(running);
        }

        void runBatchProcess(const BatchConfig &config, const QStringList &sliceIds,
                             std::function<void(BatchProcessDialog *)> addExtraParams = {});

        // ── Common result application ──

        /// Save a slice document and reload the editor if it matches the current slice.
        /// Handles saveSlice() + conditional reload + updateProgress() in one call.
        /// @return true on success.
        bool applyAndReload(const QString &sliceId, const DsTextDocument &doc);

        // ── Async engine loading ──

        /// Load a model via ModelManager and register its type.
        /// Handles the common pattern: readConfig → loadModel → registerOrGetModelType.
        /// @return (ModelManager*, typeId) on success, (nullptr, 0) on failure.
        ///         Caller owns the lifetime of the returned ModelManager* (same as ensureModelManager).
        std::tuple<ModelManager *, ModelTypeId> loadModelForTask(const QString &taskKey,
                                                                 const QString &modelTypeName = {}) override;

        /// Load an inference engine asynchronously in a background thread.
        /// @param taskKey Unique key for this loading task (prevents duplicate loads).
        /// @param loadFunc Blocking function that loads the engine. Returns true on success.
        ///        Called on a background thread — must not touch UI.
        /// @param onReady Callback invoked on the main thread after successful load.
        void loadEngineAsync(const QString &taskKey, std::function<bool()> loadFunc, std::function<void(bool)> onReady);

        /// Check if an async engine load is in progress for the given task key.
        bool isEngineLoading(const QString &taskKey) const;

        /// Cancel an async task by invalidating its alive token.
        /// Sequence: store(false) → clear engine pointer → reset token.
        /// This ordering ensures the background thread sees false before
        /// the engine pointer is nulled, closing the TOCTOU gap (P-09).
        static void cancelAsyncTask(std::shared_ptr<std::atomic<bool>> &aliveToken);

        // ── Async inference task runner ──

        /// Run an async inference task with alive-token + QPointer safety.
        /// Handles QtConcurrent::run, try/catch, alive-token check, and
        /// invokeMethod callback to the main thread.
        /// @tparam T Result type (default void).
        /// @param taskKey  Alive-token key.
        /// @param sliceId  Current slice ID (for logging, etc.).
        /// @param backgroundWork  Blocking work on worker thread.
        ///        Receives shared_ptr<atomic<bool>> for cancellation check.
        ///        Should return Result<T>.
        /// @param onComplete  Called on main thread with the result.
        template <typename T = void>
        void runAsyncTask(const QString &taskKey, const QString &sliceId,
                          std::function<Result<T>(const std::shared_ptr<std::atomic<bool>> &)> backgroundWork,
                          std::function<void(const QString &, const Result<T> &)> onComplete) {
            auto alive = aliveToken(taskKey).token();
            QPointer<EditorPageBase> guard(this);

            (void) QtConcurrent::run([alive, sliceId, guard, backgroundWork = std::move(backgroundWork),
                                      onComplete = std::move(onComplete)]() {
                if (!alive || !*alive)
                    return;

                Result<T> result = Err<T>(std::string("Not executed"));
                try {
                    if (backgroundWork)
                        result = backgroundWork(alive);
                } catch (const std::exception &e) {
                    DSFW_LOG_ERROR("infer",
                                   ("Async task exception: " + sliceId.toStdString() + " - " + e.what()).c_str());
                    result = Err<T>(std::string("Exception: ") + e.what());
                }

                if (!guard)
                    return;

                QMetaObject::invokeMethod(
                    guard.data(),
                    [guard, sliceId, result = std::move(result), onComplete = std::move(onComplete)]() {
                        if (!guard)
                            return;
                        if (onComplete)
                            onComplete(sliceId, result);
                    },
                    Qt::QueuedConnection);
            });
        }

        // ── Template-based engine loading (P-12) ──

        /// Called when a model is invalidated (override to clear engine pointer).
        void onEngineInvalidated(const QString &taskKey) override {
            Q_UNUSED(taskKey)
        }

    public:
        // ── Common utility ──

        /// Compute audio duration in seconds from a DsTextDocument.
        /// Returns 0.0 if the document has no valid audio range.
        static double audioDurationSec(const DsTextDocument &doc);

        /// Show Save/Discard/Cancel dialog and act accordingly.
        /// Uses isDirty() to decide whether to show dialog.
        bool maybeSave();

        /// Auto-save the current slice if dirty (no dialog).
        /// Returns true if save succeeded or was not needed.
        bool autoSaveCurrentSlice();

        /// Update SliceListPanel dirty indicator for the current slice.
        void updateDirtyIndicator();

        /// Update SliceListPanel progress based on layer completion.
        /// Counts slices where any of @p layerNames has non-empty boundaries.
        void updateProgress(const QStringList &layerNames);

        /// No-arg overload; subclasses provide layer names.
        virtual void updateProgress();

        /// Start the auto-save timer (called from onActivated).
        void startAutoSaveTimer();

        /// Stop the auto-save timer (called from onDeactivated).
        void stopAutoSaveTimer();

        // ── Status bar builder ──

        /// Helper for building status bar content in createStatusBarContent().
        /// Ensures all signal connections use the target widget as context,
        /// so connections are automatically broken when the widget is destroyed
        /// during page switches (AppShell::rebuildStatusBar calls deleteLater).
        /// @see StatusBarBuilder

    private:
        SliceListPanel *m_sliceList = nullptr;
        QSplitter *m_splitter = nullptr;
        IEditorDataSource *m_source = nullptr;
        AppSettingsBackend *m_settingsBackend = nullptr;
        QString m_currentSliceId;

        dstools::AppSettings m_settings;
        dsfw::widgets::ShortcutManager *m_shortcutManager = nullptr;

        QAction *m_prevAction = nullptr;
        QAction *m_nextAction = nullptr;

        std::unique_ptr<EnginePool> m_enginePool;
        QSet<QString> m_loadingEngines;
        ModelManager *m_modelManager = nullptr;
        std::map<QString, EngineAliveToken> m_aliveTokens;
        std::atomic<bool> m_batchRunning{false};
        QString m_batchTaskKey;

        QTimer *m_autoSaveTimer = nullptr;

        dsfw::widgets::PageBanner *m_pageBanner = nullptr;

        int m_activationGeneration = 0;

        void onSliceSelected(const QString &sliceId);
    };

} // namespace dstools
