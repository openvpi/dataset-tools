#include "EditorPageBase.h"
#include "SliceListPanel.h"
#include "ISettingsBackend.h"
#include "BatchProcessDialog.h"

#include <dsfw/CommonKeys.h>
#include <dsfw/Log.h>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <QSplitter>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace dstools {

static const dstools::SettingsKey<QString> kSplitterState("Layout/splitterState", "");

EditorPageBase::EditorPageBase(const QString &settingsGroup, QWidget *parent)
    : QWidget(parent), m_settings(settingsGroup) {
    m_shortcutManager = new dstools::widgets::ShortcutManager(&m_settings, this);
}

EditorPageBase::~EditorPageBase() = default;

double EditorPageBase::audioDurationSec(const DsTextDocument &doc) {
    if (doc.audio.out > doc.audio.in)
        return usToSec(doc.audio.out - doc.audio.in);
    return 0.0;
}

// ── Setup helpers ─────────────────────────────────────────────────────────────

void EditorPageBase::setupBaseLayout(QWidget *editorWidget) {
    m_sliceList = new SliceListPanel(this);
    m_sliceList->setMinimumWidth(160);
    m_sliceList->setMaximumWidth(280);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_sliceList);
    m_splitter->addWidget(editorWidget);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);

    connect(m_sliceList, &SliceListPanel::sliceSelected,
            this, &EditorPageBase::onSliceSelected);
}

void EditorPageBase::setupNavigationActions() {
    m_prevAction = new QAction(tr("Previous Slice"), this);
    m_nextAction = new QAction(tr("Next Slice"), this);

    m_shortcutManager->bind(m_prevAction, dsfw::CommonKeys::NavigationPrev,
                            tr("Previous Slice"), tr("Navigation"));
    m_shortcutManager->bind(m_nextAction, dsfw::CommonKeys::NavigationNext,
                            tr("Next Slice"), tr("Navigation"));

    connect(m_prevAction, &QAction::triggered, this, [this]() {
        m_sliceList->selectPrev();
    });
    connect(m_nextAction, &QAction::triggered, this, [this]() {
        m_sliceList->selectNext();
    });
}

// ── Data source ───────────────────────────────────────────────────────────────

void EditorPageBase::setDataSource(IEditorDataSource *source, ISettingsBackend *settingsBackend) {
    m_source = source;
    m_settingsBackend = settingsBackend;
    if (m_sliceList)
        m_sliceList->setDataSource(source);
}

// ── Slice selection ───────────────────────────────────────────────────────────

void EditorPageBase::onSliceSelected(const QString &sliceId) {
    autoSaveCurrentSlice();

    QString prevSlice = m_currentSliceId;
    m_currentSliceId = sliceId;
    DSFW_LOG_DEBUG("ui", ("Slice selected: " + sliceId.toStdString()).c_str());
    if (m_sliceList)
        m_sliceList->saveSelection(m_settings);

    if (!m_source)
        return;

    onSliceSelectedImpl(sliceId);
    emit sliceChanged(sliceId);
}

// ── IPageActions ──────────────────────────────────────────────────────────────

QString EditorPageBase::windowTitle() const {
    QString title = windowTitlePrefix();
    if (!m_currentSliceId.isEmpty())
        title += QStringLiteral(" — ") + m_currentSliceId;
    return title;
}

bool EditorPageBase::hasUnsavedChanges() const {
    return isDirty();
}

void EditorPageBase::handleDragEnter(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void EditorPageBase::handleDrop(QDropEvent *event) {
    Q_UNUSED(event)
}

// ── IPageLifecycle ────────────────────────────────────────────────────────────

void EditorPageBase::onActivated() {
    if (m_shortcutManager)
        m_shortcutManager->setEnabled(true);

    if (m_sliceList)
        m_sliceList->refresh();

    // Restore splitter state
    auto state = m_settings.get(kSplitterState);
    if (!state.isEmpty() && m_splitter)
        m_splitter->restoreState(QByteArray::fromBase64(state.toUtf8()));

    restoreExtraSplitters();

    if (m_currentSliceId.isEmpty() && m_sliceList)
        m_sliceList->ensureSelection(m_settings);

    startAutoSaveTimer();

    onAutoInfer();
}

bool EditorPageBase::onDeactivating() {
    // Save splitter state
    if (m_splitter)
        m_settings.set(kSplitterState, QString::fromLatin1(m_splitter->saveState().toBase64()));

    saveExtraSplitters();

    return maybeSave();
}

void EditorPageBase::onDeactivated() {
    stopAutoSaveTimer();

    if (m_shortcutManager)
        m_shortcutManager->setEnabled(false);

    onDeactivatedImpl();
}

void EditorPageBase::onShutdown() {
    if (m_shortcutManager)
        m_shortcutManager->saveAll();

    onShutdownImpl();
}

// ── Utility ───────────────────────────────────────────────────────────────────

bool EditorPageBase::maybeSave() {
    if (!isDirty())
        return true;

    auto ret = QMessageBox::question(
        this, tr("Unsaved Changes"),
        tr("Current slice has been modified. Save changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save) {
        bool ok = saveCurrentSlice();
        if (ok) updateDirtyIndicator();
        return ok;
    }
    if (ret == QMessageBox::Discard)
        return true;
    return false;
}

bool EditorPageBase::autoSaveCurrentSlice() {
    if (!isDirty())
        return true;

    bool ok = saveCurrentSlice();
    if (ok) updateDirtyIndicator();
    return ok;
}

void EditorPageBase::updateDirtyIndicator() {
    if (m_sliceList && !m_currentSliceId.isEmpty())
        m_sliceList->setSliceDirty(m_currentSliceId, isDirty());
}

void EditorPageBase::startAutoSaveTimer() {
    if (!m_autoSaveTimer) {
        m_autoSaveTimer = new QTimer(this);
        connect(m_autoSaveTimer, &QTimer::timeout, this, [this]() {
            if (isDirty())
                autoSaveCurrentSlice();
        });
    }

    static const dstools::SettingsKey<bool> kAutoSaveEnabled("General/autoSaveEnabled", true);
    static const dstools::SettingsKey<int> kAutoSaveIntervalMs("General/autoSaveIntervalMs", 30000);
    dstools::AppSettings appSettings("Editor");
    appSettings.reload();

    if (appSettings.get(kAutoSaveEnabled)) {
        int intervalMs = appSettings.get(kAutoSaveIntervalMs);
        m_autoSaveTimer->start(intervalMs);
    } else {
        m_autoSaveTimer->stop();
    }
}

void EditorPageBase::stopAutoSaveTimer() {
    if (m_autoSaveTimer)
        m_autoSaveTimer->stop();
}

void EditorPageBase::loadEngineAsync(const QString &taskKey,
                                      std::function<bool()> loadFunc,
                                      std::function<void()> onReady) {
    if (m_loadingEngines.contains(taskKey))
        return; // Already loading

    m_loadingEngines.insert(taskKey);
    QPointer<EditorPageBase> guard(this);

    (void) QtConcurrent::run([guard, taskKey, loadFunc = std::move(loadFunc),
                              onReady = std::move(onReady)]() {
        bool success = false;
        try {
            if (loadFunc)
                success = loadFunc();
        } catch (const std::exception &e) {
            DSFW_LOG_ERROR("infer", ("Engine load exception: " + taskKey.toStdString() + " - " + e.what()).c_str());
            success = false;
        }

        QMetaObject::invokeMethod(guard.data(), [guard, taskKey, success,
                                                  onReady = std::move(onReady)]() {
            if (!guard)
                return;
            guard->m_loadingEngines.remove(taskKey);
            if (success && onReady)
                onReady();
        }, Qt::QueuedConnection);
    });
}

bool EditorPageBase::isEngineLoading(const QString &taskKey) const {
    return m_loadingEngines.contains(taskKey);
}

void EditorPageBase::cancelAsyncTask(std::shared_ptr<std::atomic<bool>> &aliveToken) {
    if (aliveToken) {
        aliveToken->store(false);
    }
    aliveToken.reset();
}

// ── StatusBarBuilder ──────────────────────────────────────────────────────────

EditorPageBase::StatusBarBuilder::StatusBarBuilder(QWidget *container)
    : m_container(container) {
    m_layout = new QHBoxLayout(container);
    m_layout->setContentsMargins(0, 0, 0, 0);
}

QLabel *EditorPageBase::StatusBarBuilder::addLabel(const QString &text, int stretch) {
    auto *label = new QLabel(text, m_container);
    m_layout->addWidget(label, stretch);
    return label;
}

bool EditorPageBase::hasExistingResult(const QString &sliceId) const {
    Q_UNUSED(sliceId)
    return false;
}

bool EditorPageBase::prepareSliceInput(const QString &sliceId, QString &skipReason) {
    Q_UNUSED(sliceId)
    Q_UNUSED(skipReason)
    return true;
}

BatchSliceResult EditorPageBase::processSlice(const QString &sliceId) {
    Q_UNUSED(sliceId)
    return {BatchSliceResult::Error, "processSlice not implemented"};
}

void EditorPageBase::applyBatchResult(const QString &sliceId, const BatchSliceResult &result) {
    Q_UNUSED(sliceId)
    Q_UNUSED(result)
}

void EditorPageBase::runBatchProcess(
    const BatchConfig &config,
    const QStringList &sliceIds,
    std::function<void(BatchProcessDialog *)> addExtraParams)
{
    auto *dlg = new BatchProcessDialog(config.dialogTitle, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    auto *skipExisting = new QCheckBox(config.skipExistingLabel, dlg);
    skipExisting->setChecked(config.defaultSkipExisting);
    dlg->addParamWidget(skipExisting);

    if (addExtraParams)
        addExtraParams(dlg);

    dlg->appendLog(tr("Total slices: %1").arg(sliceIds.size()));

    auto aliveToken = batchAliveToken();
    auto *src = source();
    QPointer<EditorPageBase> guard(this);

    connect(dlg, &BatchProcessDialog::started, this,
        [dlg, src, ids = sliceIds, guard, skipExisting, aliveToken]() {
        if (!guard) {
            dlg->finish(0, 0, 0);
            return;
        }
        guard->setBatchRunning(true);
        bool skip = skipExisting->isChecked();

        (void) QtConcurrent::run([dlg, src, ids, guard, skip, aliveToken]() {
            int processed = 0, skipped = 0, errors = 0;
            int idx = 0;
            try {
                for (const auto &sliceId : ids) {
                    if (dlg->isCancelled()) break;
                    if (aliveToken && !*aliveToken) break;

                    QMetaObject::invokeMethod(dlg,
                        [dlg, idx, total = ids.size()]() { dlg->setProgress(idx, total); },
                        Qt::QueuedConnection);

                    QString audioPath = src->validatedAudioPath(sliceId);
                    if (audioPath.isEmpty()) {
                        ++skipped;
                        QMetaObject::invokeMethod(dlg,
                            [dlg, sliceId]() { dlg->appendLog(tr("[SKIP] %1 (missing audio)").arg(sliceId)); },
                            Qt::QueuedConnection);
                        ++idx;
                        continue;
                    }

                    if (skip && guard->hasExistingResult(sliceId)) {
                        ++skipped;
                        QMetaObject::invokeMethod(dlg,
                            [dlg, sliceId]() { dlg->appendLog(tr("[SKIP] %1 (existing result)").arg(sliceId)); },
                            Qt::QueuedConnection);
                        ++idx;
                        continue;
                    }

                    QString skipReason;
                    if (!guard->prepareSliceInput(sliceId, skipReason)) {
                        ++skipped;
                        QMetaObject::invokeMethod(dlg,
                            [dlg, sliceId, skipReason]() { dlg->appendLog(tr("[SKIP] %1 (%2)").arg(sliceId, skipReason)); },
                            Qt::QueuedConnection);
                        ++idx;
                        continue;
                    }

                    auto result = guard->processSlice(sliceId);

                    if (result.status == BatchSliceResult::Processed) {
                        QMetaObject::invokeMethod(guard.data(),
                            [guard, sliceId, result]() { guard->applyBatchResult(sliceId, result); },
                            Qt::QueuedConnection);
                        ++processed;
                    } else if (result.status == BatchSliceResult::Skipped) {
                        ++skipped;
                    } else {
                        ++errors;
                    }
                    QMetaObject::invokeMethod(dlg,
                        [dlg, msg = result.logMessage]() { dlg->appendLog(msg); },
                        Qt::QueuedConnection);

                    ++idx;
                }
            } catch (const std::exception &e) {
                QMetaObject::invokeMethod(dlg,
                    [dlg, eMsg = std::string(e.what())]() {
                        dlg->appendLog(tr("[ERROR] Exception: %1").arg(QString::fromStdString(eMsg)));
                    }, Qt::QueuedConnection);
            }

            QMetaObject::invokeMethod(dlg,
                [dlg, processed, skipped, errors]() { dlg->finish(processed, skipped, errors); },
                Qt::QueuedConnection);
            QMetaObject::invokeMethod(guard.data(),
                [guard]() { if (guard) guard->setBatchRunning(false); },
                Qt::QueuedConnection);
        });
    });

    connect(dlg, &BatchProcessDialog::cancelled, this, [guard, aliveToken]() {
        if (aliveToken) aliveToken->store(false);
        if (guard) guard->setBatchRunning(false);
    });

    dlg->show();
}

} // namespace dstools
