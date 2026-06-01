#include "EditorPageBase.h"
#include "EnginePool.h"
#include "Keys.h"
#include "SliceListPanel.h"
#include "AppSettingsBackend.h"
#include "BatchProcessDialog.h"
#include "ModelConfigHelper.h"

#include <dsfw/CommonKeys.h>
#include <dsfw/Log.h>
#include <dsfw/AppSettings.h>
#include <dsfw/AppPaths.h>
#include <dstools/ModelManager.h>
#include <dsfw/InferenceModelProvider.h>
#include <dsfw/widgets/PlayWidget.h>
#include <dsfw/widgets/ShortcutManager.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dsfw/Theme.h>
#include <dstools/Constants.h>
#include <dstools/DsKeys.h>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QCheckBox>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>

#include <QPointer>
#include <QSet>
#include <QSplitter>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace dstools {

EditorPageBase::EditorPageBase(const QString &settingsGroup, QWidget *parent)
    : QWidget(parent), m_settings(settingsGroup) {
    m_shortcutManager = new dsfw::widgets::ShortcutManager(&m_settings, this);
    m_enginePool = std::make_unique<EnginePool>(this);
}

EditorPageBase::~EditorPageBase() = default;

double EditorPageBase::audioDurationSec(const DsTextDocument &doc) {
    if (doc.audio.out > doc.audio.in)
        return usToSec(doc.audio.out - doc.audio.in);
    return 0.0;
}

// ── Standard UI helpers (P3-A3) ───────────────────────────────────────────────

QMenu *EditorPageBase::addStandardFileMenu(QMenuBar *bar, QAction *saveAction) {
    auto *fileMenu = bar->addMenu(tr("文件(&F)"));
    if (saveAction)
        fileMenu->addAction(saveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出(&X)"), this, [this]() {
        if (auto *w = window())
            w->close();
    });
    return fileMenu;
}

QMenu *EditorPageBase::addStandardEditMenu(QMenuBar *bar, QAction *undoAction, QAction *redoAction) {
    auto *editMenu = bar->addMenu(tr("编辑(&E)"));
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);
    editMenu->addSeparator();
    editMenu->addAction(prevAction());
    editMenu->addAction(nextAction());
    editMenu->addSeparator();
    auto *shortcutAction = editMenu->addAction(tr("快捷键设置..."));
    connect(shortcutAction, &QAction::triggered, this, [this]() { shortcutManager()->showEditor(this); });
    return editMenu;
}

QMenu *EditorPageBase::addStandardViewMenu(QMenuBar *bar, const QList<QAction *> &zoomActions) {
    auto *viewMenu = bar->addMenu(tr("视图(&V)"));
    if (!zoomActions.isEmpty()) {
        for (auto *act : zoomActions) {
            if (act)
                viewMenu->addAction(act);
        }
        viewMenu->addSeparator();
    }
    dsfw::Theme::instance().populateThemeMenu(viewMenu);
    return viewMenu;
}

StatusBarBuilder EditorPageBase::addStandardStatusWidgets(QWidget *parent,
                                                           QLabel **outSliceLabel,
                                                           QLabel **outDirtyLabel) {
    StatusBarBuilder builder(parent);
    auto *sliceLabel = builder.addLabel(tr("未选择切片"), 1);
    auto *dirtyLabel = builder.addLabel(tr("✅ 数据就绪"));

    builder.connect(this, &EditorPageBase::sliceChanged, sliceLabel,
                    [sliceLabel](const QString &id) { sliceLabel->setText(id.isEmpty() ? tr("未选择切片") : id); });
    builder.connect(this, &EditorPageBase::sliceChanged, dirtyLabel,
                    [this, dirtyLabel](const QString &) { updateDirtyStatusLabel(dirtyLabel); });

    if (outSliceLabel) *outSliceLabel = sliceLabel;
    if (outDirtyLabel) *outDirtyLabel = dirtyLabel;
    return builder;
}

QMenuBar *EditorPageBase::buildStandardMenuBar(QWidget *parent, QAction *saveAction, QAction *undoAction,
                                                QAction *redoAction) {
    auto *bar = new QMenuBar(parent);
    addStandardFileMenu(bar, saveAction);
    addStandardEditMenu(bar, undoAction, redoAction);
    return bar;
}

StatusBarBuilder EditorPageBase::buildStandardStatusBar(QWidget *container) {
    return addStandardStatusWidgets(container, nullptr, nullptr);
}

dsfw::widgets::PipelineStatusBar *EditorPageBase::createPipelineStatusBar() {
    auto *bar = new dsfw::widgets::PipelineStatusBar();
    setupPipelineSteps(bar);
    return bar;
}

void EditorPageBase::setupPipelineSteps(dsfw::widgets::PipelineStatusBar *bar) {
    Q_UNUSED(bar)
}

void EditorPageBase::runStandardAutoInferPreload() {
    updateProgress();
}

void EditorPageBase::updateProgress() {
}

QJsonObject EditorPageBase::preloadConfig() const {
    if (!m_settingsBackend)
        return {};
    auto settingsData = m_settingsBackend->load();
    return settingsData["preload"].toObject();
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

    m_pageBanner = new dsfw::widgets::PageBanner(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_pageBanner);
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

void EditorPageBase::setDataSource(IEditorDataSource *source, AppSettingsBackend *settingsBackend) {
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
    ++m_activationGeneration;

    if (m_shortcutManager)
        m_shortcutManager->setEnabled(true);

    if (m_sliceList)
        m_sliceList->refresh();

    // Restore splitter state
    auto state = m_settings.get(dstools::settings::kSplitterState);
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
        m_settings.set(dstools::settings::kSplitterState, QString::fromLatin1(m_splitter->saveState().toBase64()));

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

// ── PageBanner ────────────────────────────────────────────────────────────────

void EditorPageBase::showBanner(dsfw::widgets::BannerType type, const QString &message,
                                const QStringList &buttonLabels) {
    if (m_pageBanner) {
        m_pageBanner->show(type, message, buttonLabels);
        QObject::connect(m_pageBanner, &dsfw::widgets::PageBanner::closeRequested,
                         this, [this]() { m_pageBanner->close(); },
                         Qt::SingleShotConnection);
        if (!buttonLabels.isEmpty()) {
            QObject::connect(m_pageBanner, &dsfw::widgets::PageBanner::buttonClicked,
                             this, [this](int) { m_pageBanner->close(); },
                             Qt::SingleShotConnection);
        }
    }
}

void EditorPageBase::hideBanner() {
    if (m_pageBanner)
        m_pageBanner->hide();
}

// ── Auto-infer ────────────────────────────────────────────────────────────────

static bool isAutoStep(const QString &stepName) {
    static const QSet<QString> manualSteps = {QString::fromUtf8(dstools::keys::steps::minLabel)};
    return !manualSteps.contains(stepName);
}

// ── Template method: onAutoInfer ─────────────────────────────────────────────

void EditorPageBase::onAutoInfer() {
    runStandardAutoInferPreload();

    onAutoInferPreloadEngines();

    if (!m_source || m_currentSliceId.isEmpty())
        return;

    auto dirty = m_source->dirtyLayers(m_currentSliceId);
    if (dirty.isEmpty())
        return;

    bool hasManualDirty = false;
    for (const auto &layer : dirty) {
        if (!isAutoStep(layer)) {
            hasManualDirty = true;
            break;
        }
    }

    if (hasManualDirty && m_pageBanner) {
        m_pageBanner->show(dsfw::widgets::BannerType::Warning,
                           tr("检测到待更新的标注数据，请检查并手动修正。"),
                           {tr("知道了")});
        QObject::connect(m_pageBanner, &dsfw::widgets::PageBanner::buttonClicked,
                         this, [this](int) { hideBanner(); });
    }

    onAutoInferProcessDirty(dirty);
}

void EditorPageBase::onAutoInferPreloadEngines() {}

void EditorPageBase::onAutoInferProcessDirty(const QStringList &dirty) {
    Q_UNUSED(dirty);
}

void EditorPageBase::showAutoInferProgress(const QString &stepName) {
    if (m_pageBanner)
        m_pageBanner->show(dsfw::widgets::BannerType::Info,
                           tr("正在执行: %1...").arg(stepName));
}

void EditorPageBase::showAutoInferComplete(const QString &stepName) {
    if (m_pageBanner)
        m_pageBanner->show(dsfw::widgets::BannerType::Success,
                           tr("%1 已完成").arg(stepName),
                           {tr("知道了")});
    if (m_pageBanner)
        QObject::connect(m_pageBanner, &dsfw::widgets::PageBanner::buttonClicked,
                         this, [this](int) { hideBanner(); });
}

int EditorPageBase::showAutoInferFailed(const QString &stepName, const QString &error) {
    if (m_pageBanner) {
        m_pageBanner->show(dsfw::widgets::BannerType::Error,
                           tr("%1 失败: %2").arg(stepName, error));
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("自动处理失败"));
    msgBox.setText(tr("步骤 '%1' 执行失败。").arg(stepName));
    msgBox.setInformativeText(error.isEmpty() ? tr("未知错误") : error);
    msgBox.setIcon(QMessageBox::Critical);
    auto *retryBtn = msgBox.addButton(tr("重试"), QMessageBox::AcceptRole);
    auto *skipBtn = msgBox.addButton(tr("跳过"), QMessageBox::RejectRole);
    msgBox.setEscapeButton(skipBtn);
    msgBox.exec();

    if (msgBox.clickedButton() == retryBtn)
        return 0;
    if (msgBox.clickedButton() == skipBtn)
        return 1;
    return -1;
}

// ── Dirty status bar ──────────────────────────────────────────────────────────

void EditorPageBase::markLayersModified(const QStringList &layers) {
    if (!m_source || m_currentSliceId.isEmpty() || layers.isEmpty())
        return;
    m_source->addDirtyLayers(m_currentSliceId, layers);
}

void EditorPageBase::addEditedStep(const QString &step) {
    if (!m_source || m_currentSliceId.isEmpty() || step.isEmpty())
        return;
    m_source->addEditedStep(m_currentSliceId, step);
}

void EditorPageBase::setLayerManuallyEdited(const QString &layer, bool edited) {
    if (!m_source || m_currentSliceId.isEmpty() || layer.isEmpty())
        return;
    m_source->setLayerManuallyEdited(m_currentSliceId, layer, edited);
}

QString EditorPageBase::computeDirtyStatusText(const QString &sliceId) const {
    if (!m_source || sliceId.isEmpty())
        return tr("✅ 数据就绪");

    auto dirtyLayers = m_source->dirtyLayers(sliceId);
    if (dirtyLayers.isEmpty())
        return tr("✅ 数据就绪");

    QStringList dirtyNames;
    for (const auto &layer : dirtyLayers) {
        if (layer == QLatin1String(dstools::keys::layers::grapheme)) dirtyNames.append(tr("歌词"));
        else if (layer == QLatin1String(dstools::keys::layers::phoneme)) dirtyNames.append(tr("音素"));
        else if (layer == QLatin1String(dstools::keys::layers::phNum)) dirtyNames.append(tr("发音数"));
        else if (layer == QLatin1String(dstools::keys::layers::midi)) dirtyNames.append(QStringLiteral("MIDI"));
        else if (layer == QLatin1String(dstools::keys::layers::pitch)) dirtyNames.append(QStringLiteral("F0"));
        else dirtyNames.append(layer);
    }

    return tr("⏳ %1 层待更新: %2").arg(dirtyLayers.size()).arg(dirtyNames.join(QStringLiteral(", ")));
}

void EditorPageBase::updateDirtyStatusLabel(QLabel *label) {
    if (!label)
        return;
    QString text = computeDirtyStatusText(m_currentSliceId);
    label->setText(text);
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
        if (ok) {
            updateDirtyIndicator();
        } else {
            QMessageBox::warning(this, tr("Save Failed"),
                                 tr("Save failed. Please check disk space or permissions and try again."));
            DSFW_LOG_ERROR("editor", ("Manual save failed for slice " + m_currentSliceId.toStdString()).c_str());
        }
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
    if (ok) {
        updateDirtyIndicator();
    } else {
        QMessageBox::warning(this, tr("Auto-Save Failed"),
                             tr("Auto-save failed. Please check disk space or permissions and try again."));
        DSFW_LOG_ERROR("editor", ("Auto-save failed for slice " + m_currentSliceId.toStdString()).c_str());
    }
    return ok;
}

void EditorPageBase::updateDirtyIndicator() {
    if (m_sliceList && !m_currentSliceId.isEmpty())
        m_sliceList->setSliceDirty(m_currentSliceId, isDirty());
}

void EditorPageBase::updateProgress(const QStringList &layerNames) {
    if (!source())
        return;
    const auto ids = source()->sliceIds();
    int total = ids.size();
    int completed = 0;
    for (const auto &id : ids) {
        auto result = source()->loadSlice(id);
        if (result) {
            for (const auto &layer : result.value().layers) {
                if (layerNames.contains(layer.name) && !layer.boundaries.empty()) {
                    ++completed;
                    break;
                }
            }
            sliceListPanel()->setSliceLoadError(id, QString());
        } else {
            sliceListPanel()->setSliceLoadError(id, QString::fromStdString(result.error()));
        }
    }
    sliceListPanel()->setProgress(completed, total);
}

void EditorPageBase::startAutoSaveTimer() {
    if (!m_autoSaveTimer) {
        m_autoSaveTimer = new QTimer(this);
        connect(m_autoSaveTimer, &QTimer::timeout, this, [this]() {
            if (isDirty())
                autoSaveCurrentSlice();
        });
    }

    dstools::AppSettings appSettings("Editor");
    appSettings.reload();

    if (appSettings.get(dstools::settings::kAutoSaveEnabled)) {
        int intervalMs = appSettings.get(dstools::settings::kAutoSaveIntervalMs);
        m_autoSaveTimer->start(intervalMs);
    } else {
        m_autoSaveTimer->stop();
    }
}

void EditorPageBase::stopAutoSaveTimer() {
    if (m_autoSaveTimer)
        m_autoSaveTimer->stop();
}

std::tuple<ModelManager *, ModelTypeId>
    EditorPageBase::loadModelForTask(const QString &taskKey, const QString &modelTypeName) {
        auto config = readModelConfig(settingsBackend(), taskKey);
        if (config.modelPath.isEmpty())
            return {nullptr, ModelTypeId{}};

        auto *mgr = ensureModelManager();
        if (!mgr)
            return {nullptr, ModelTypeId{}};

        auto result = mgr->loadModel(taskKey, config, config.deviceId);
        if (!result)
            return {nullptr, ModelTypeId{}};

        auto typeId = registerModelType(
            (modelTypeName.isEmpty() ? taskKey : modelTypeName).toStdString());
        return {mgr, typeId};
    }

    void EditorPageBase::loadEngineAsync(const QString &taskKey,
                                      std::function<bool()> loadFunc,
                                      std::function<void(bool)> onReady) {
    if (m_loadingEngines.contains(taskKey))
        return;

    m_loadingEngines.insert(taskKey);
    QPointer<EditorPageBase> guard(this);
    auto alive = aliveToken(taskKey).token();

    (void) QtConcurrent::run([guard, alive, taskKey, loadFunc = std::move(loadFunc),
                              onReady = std::move(onReady)]() {
        if (alive && !*alive)
            return;

        bool success = false;
        std::string errorMsg;
        try {
            if (loadFunc)
                success = loadFunc();
        } catch (const std::exception &e) {
            errorMsg = e.what();
            DSFW_LOG_ERROR("infer", ("Engine load exception: " + taskKey.toStdString() + " - " + errorMsg).c_str());
            success = false;
        }

        QMetaObject::invokeMethod(guard.data(), [guard, alive, taskKey, success,
                                                  onReady = std::move(onReady)]() {
            if (!guard || (alive && !*alive))
                return;
            guard->m_loadingEngines.remove(taskKey);
            if (onReady)
                onReady(success);
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

ModelManager *EditorPageBase::ensureModelManager() {
    if (m_modelManager)
        return m_modelManager;

    m_modelManager = &ModelManager::instance();
    return m_modelManager;
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

                    if (!guard) break;

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
                ++errors;
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

bool EditorPageBase::applyAndReload(const QString &sliceId, const DsTextDocument &doc) {
    if (!source())
        return false;
    auto saveResult = source()->saveSlice(sliceId, doc);
    if (!saveResult) {
        DSFW_LOG_ERROR("editor", ("Failed to save slice in applyAndReload: " + sliceId.toStdString() + " - " + saveResult.error()).c_str());
    }
    if (sliceId == currentSliceId())
        onSliceSelectedImpl(sliceId);
    return true;
}

} // namespace dstools