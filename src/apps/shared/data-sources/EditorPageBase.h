#pragma once

#include "dstools/ModelManager.h"
#include <dstools/Result.h>


#include <dsfw/AppSettings.h>
#include <dsfw/IPageActions.h>
#include <dsfw/Log.h>
#include <dsfw/IPageLifecycle.h>
#include <dstools/DsTextTypes.h>
#include <dstools/IEditorDataSource.h>
#include <dstools/ShortcutManager.h>

#include <QWidget>
#include <QString>
#include <QSet>
#include <QTimer>
#include <QPointer>
#include <QtConcurrent>
#include <atomic>
#include <functional>
#include <map>
#include <memory>

class QAction;
class QMenuBar;
class QSplitter;
class QLabel;
class QHBoxLayout;

template<typename EngineType>
struct EngineTraits;

namespace dstools {

class ISettingsBackend;
class SliceListPanel;
class BatchProcessDialog;
class IModelManager;

struct BatchSliceResult {
    enum Status { Processed, Skipped, Error };
    Status status = Error;
    QString logMessage;
};

class EngineAliveToken {
public:
    explicit EngineAliveToken(bool startValid = false) {
        if (startValid)
            m_token = std::make_shared<std::atomic<bool>>(true);
    }

    std::shared_ptr<std::atomic<bool>> token() const { return m_token; }

    bool isValid() const { return m_token && *m_token; }
    explicit operator bool() const { return isValid(); }

    void create() {
        m_token = std::make_shared<std::atomic<bool>>(true);
    }

    void invalidate() {
        if (m_token)
            m_token->store(false);
        m_token.reset();
    }

private:
    std::shared_ptr<std::atomic<bool>> m_token;
};

struct BatchConfig {
    QString dialogTitle;
    bool defaultSkipExisting = true;
    QString skipExistingLabel;
};

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

    IModelManager *ensureModelManager();

    // ── Engine token management ──

    /// Get or create an alive token for the given engine key.
    EngineAliveToken &aliveToken(const QString &key) { return m_aliveTokens[key]; }

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

    // ── Batch processing template (P-12) ──

    virtual bool hasExistingResult(const QString &sliceId) const;

    virtual bool prepareSliceInput(const QString &sliceId, QString &skipReason);

    virtual BatchSliceResult processSlice(const QString &sliceId);

    virtual void applyBatchResult(const QString &sliceId, const BatchSliceResult &result);

    virtual bool isBatchRunning() const = 0;
    virtual void setBatchRunning(bool running) = 0;

    virtual std::shared_ptr<std::atomic<bool>> batchAliveToken() const = 0;

    void runBatchProcess(const BatchConfig &config,
                         const QStringList &sliceIds,
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
    std::tuple<ModelManager *, ModelTypeId>
    loadModelForTask(const QString &taskKey, const QString &modelTypeName = {});

    /// Load an inference engine asynchronously in a background thread.
    /// @param taskKey Unique key for this loading task (prevents duplicate loads).
    /// @param loadFunc Blocking function that loads the engine. Returns true on success.
    ///        Called on a background thread — must not touch UI.
    /// @param onReady Callback invoked on the main thread after successful load.
    void loadEngineAsync(const QString &taskKey,
                         std::function<bool()> loadFunc,
                         std::function<void()> onReady);

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
    template<typename T = void>
    void runAsyncTask(const QString &taskKey, const QString &sliceId,
                      std::function<Result<T>(const std::shared_ptr<std::atomic<bool>>&)> backgroundWork,
                      std::function<void(const QString&, const Result<T>&)> onComplete) {
        auto alive = aliveToken(taskKey).token();
        QPointer<EditorPageBase> guard(this);

        (void) QtConcurrent::run([alive, sliceId, guard,
                                  backgroundWork = std::move(backgroundWork),
                                  onComplete = std::move(onComplete)]() {
            if (!alive || !*alive)
                return;

            Result<T> result = Err(std::string("Not executed"));
            try {
                if (backgroundWork)
                    result = backgroundWork(alive);
            } catch (const std::exception &e) {
                DSFW_LOG_ERROR("infer", ("Async task exception: " + sliceId.toStdString() +
                                         " - " + e.what()).c_str());
                result = Err(std::string("Exception: ") + e.what());
            }

            if (!guard)
                return;

            QMetaObject::invokeMethod(guard.data(), [guard, sliceId, result = std::move(result),
                                                      onComplete = std::move(onComplete)]() {
                if (!guard)
                    return;
                if (onComplete)
                    onComplete(sliceId, result);
            }, Qt::QueuedConnection);
        });
    }

    // ── Template-based engine loading (P-12) ──

    /// Called when a model is invalidated (override to clear engine pointer).
    virtual void onEngineInvalidated(const QString &taskKey) { Q_UNUSED(taskKey) }

    /// Ensure an inference engine is loaded and ready.
    /// Uses EngineTraits<EngineType> for type-specific behavior.
    template<typename EngineType>
    void ensureEngine(EngineType *&enginePtr, const QString &taskKey) {
        using Traits = EngineTraits<EngineType>;

        if (Traits::isOpen(enginePtr))
            return;

        auto *mgr = ensureModelManager();
        if (!mgr)
            return;

        auto &token = aliveToken(taskKey);
        if (!token.isValid()) {
            connect(mgr, &IModelManager::modelInvalidated,
                    this, &EditorPageBase::onEngineInvalidated);
        }

        auto [mm, typeId] = loadModelForTask(taskKey);
        if (!mm || !typeId.isValid())
            return;

        auto *provider = mm->provider(typeId);
        auto *typedProvider = dynamic_cast<typename Traits::ProviderType *>(provider);
        if (typedProvider && Traits::isOpen(&typedProvider->engine())) {
            enginePtr = &typedProvider->engine();
            token.create();
        }
    }

    /// Async version: defers ensureEngine to next event-loop iteration.
    template<typename EngineType>
    void ensureEngineAsync(EngineType *&enginePtr, const QString &taskKey,
                           std::function<void()> onReady = {}) {
        using Traits = EngineTraits<EngineType>;

        if (Traits::isOpen(enginePtr)) {
            if (onReady)
                onReady();
            return;
        }

        QPointer<EditorPageBase> guard(this);
        QTimer::singleShot(0, this, [this, guard, onReady = std::move(onReady),
                                     taskKey, &enginePtr]() {
            if (!guard)
                return;
            ensureEngine(enginePtr, taskKey);
            if (Traits::isOpen(enginePtr) && onReady)
                onReady();
        });
    }

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

    /// Start the auto-save timer (called from onActivated).
    void startAutoSaveTimer();

    /// Stop the auto-save timer (called from onDeactivated).
    void stopAutoSaveTimer();

    // ── Status bar builder ──

    /// Helper for building status bar content in createStatusBarContent().
    /// Ensures all signal connections use the target widget as context,
    /// so connections are automatically broken when the widget is destroyed
    /// during page switches (AppShell::rebuildStatusBar calls deleteLater).
    class StatusBarBuilder {
    public:
        explicit StatusBarBuilder(QWidget *container);

        QLabel *addLabel(const QString &text, int stretch = 0);

        template<typename Sender, typename Signal, typename Widget, typename Func>
        QMetaObject::Connection connect(Sender *sender, Signal signal,
                                         Widget *widget, Func &&func) {
            return QObject::connect(sender, signal, widget, std::forward<Func>(func));
        }

        QWidget *container() const { return m_container; }

    private:
        QWidget *m_container;
        QHBoxLayout *m_layout;
    };

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

    QSet<QString> m_loadingEngines;
    IModelManager *m_modelManager = nullptr;
    std::map<QString, EngineAliveToken> m_aliveTokens;

    QTimer *m_autoSaveTimer = nullptr;

    void onSliceSelected(const QString &sliceId);
};

} // namespace dstools
