#include "PitchLabelerPage.h"
#include "SliceListPanel.h"
#include "ModelConfigHelper.h"
#include "ISettingsBackend.h"
#include "BatchProcessDialog.h"

#include <dstools/IEditorDataSource.h>

#include <dsfw/Log.h>

#include <DSFile.h>

#include <QCheckBox>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <QPointer>
#include <QtConcurrent>

#include <rmvpe-infer/Rmvpe.h>
#include <game-infer/Game.h>

#include <dsfw/CommonKeys.h>
#include <dsfw/IModelManager.h>
#include <dsfw/IModelProvider.h>
#include <dsfw/InferenceModelProvider.h>
#include <dsfw/ServiceLocator.h>
#include <dsfw/Theme.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dstools/DsTextTypes.h>
#include <dstools/ExecutionProvider.h>
#include <dstools/ModelManager.h>

namespace dstools {

PitchLabelerPage::PitchLabelerPage(QWidget *parent)
    : EditorPageBase("PitchLabeler", parent) {
    m_editor = new pitchlabeler::PitchEditor(this);

    setupBaseLayout(m_editor);
    setupNavigationActions();

    m_extractPitchAction = new QAction(tr("Extract Pitch"), this);
    m_extractMidiAction = new QAction(tr("Extract MIDI"), this);
    connect(m_extractPitchAction, &QAction::triggered, this, &PitchLabelerPage::onExtractPitch);
    connect(m_extractMidiAction, &QAction::triggered, this, &PitchLabelerPage::onExtractMidi);

    auto *tb = m_editor->toolbar();
    tb->addSeparator();
    tb->addAction(m_extractPitchAction);
    tb->addAction(m_extractMidiAction);
    tb->addSeparator();
    tb->addAction(m_editor->saveAction());
    tb->addSeparator();
    tb->addAction(m_editor->zoomInAction());
    tb->addAction(m_editor->zoomOutAction());

    static const dstools::SettingsKey<QString> kShortcutSave("Shortcuts/save", "Ctrl+S");
    static const dstools::SettingsKey<QString> kShortcutUndo("Shortcuts/undo", "Ctrl+Z");
    static const dstools::SettingsKey<QString> kShortcutRedo("Shortcuts/redo", "Ctrl+Y");
    static const dstools::SettingsKey<QString> kShortcutZoomIn("Shortcuts/zoomIn", "Ctrl+=");
    static const dstools::SettingsKey<QString> kShortcutZoomOut("Shortcuts/zoomOut", "Ctrl+-");
    static const dstools::SettingsKey<QString> kShortcutZoomReset("Shortcuts/zoomReset", "Ctrl+0");
    static const dstools::SettingsKey<QString> kShortcutABCompare("Shortcuts/abCompare", "Ctrl+B");
    static const dstools::SettingsKey<QString> kShortcutExtractPitch("Shortcuts/extractPitch", "F");
    static const dstools::SettingsKey<QString> kShortcutExtractMidi("Shortcuts/extractMidi", "M");
    static const dstools::SettingsKey<QString> kShortcutPlayPause("Shortcuts/playPause", "Space");

    shortcutManager()->bind(m_editor->saveAction(), kShortcutSave,
                            tr("Save"), tr("File"));
    shortcutManager()->bind(m_editor->undoAction(), kShortcutUndo,
                            tr("Undo"), tr("Edit"));
    shortcutManager()->bind(m_editor->redoAction(), kShortcutRedo,
                            tr("Redo"), tr("Edit"));
    shortcutManager()->bind(m_editor->zoomInAction(), kShortcutZoomIn,
                            tr("Zoom In"), tr("View"));
    shortcutManager()->bind(m_editor->zoomOutAction(), kShortcutZoomOut,
                            tr("Zoom Out"), tr("View"));
    shortcutManager()->bind(m_editor->zoomResetAction(), kShortcutZoomReset,
                            tr("Reset Zoom"), tr("View"));
    shortcutManager()->bind(m_editor->abCompareAction(), kShortcutABCompare,
                            tr("A/B Compare"), tr("Tools"));
    shortcutManager()->bind(m_extractPitchAction, kShortcutExtractPitch,
                            tr("Extract Pitch"), tr("Processing"));
    shortcutManager()->bind(m_extractMidiAction, kShortcutExtractMidi,
                            tr("Extract MIDI"), tr("Processing"));
    shortcutManager()->bind(m_editor->playPauseAction(), kShortcutPlayPause,
                            tr("Play/Pause"), tr("Playback"));
    shortcutManager()->applyAll();
    shortcutManager()->updateTooltips();
    shortcutManager()->setEnabled(false);

    connect(m_editor->saveAction(), &QAction::triggered,
            this, [this]() { saveCurrentSlice(); updateDirtyIndicator(); });
    connect(m_editor->undoStack(), &QUndoStack::cleanChanged,
            this, [this]() { updateDirtyIndicator(); });
}

PitchLabelerPage::~PitchLabelerPage() = default;

// ── Batch processing (P-12 template) ──────────────────────────────────────────

bool PitchLabelerPage::isBatchRunning() const {
    return m_batchRunning;
}

void PitchLabelerPage::setBatchRunning(bool running) {
    m_batchRunning = running;
}

std::shared_ptr<std::atomic<bool>> PitchLabelerPage::batchAliveToken() const {
    return m_batchAlive;
}

QString PitchLabelerPage::windowTitlePrefix() const {
    return tr("Pitch Labeling");
}

bool PitchLabelerPage::isDirty() const {
    return m_editor->undoStack() && !m_editor->undoStack()->isClean();
}

void PitchLabelerPage::onDeactivatedImpl() {
    if (m_rmvpeAlive)
        m_rmvpeAlive->store(false);
    if (m_gameAlive)
        m_gameAlive->store(false);
    m_rmvpe = nullptr;
    m_game = nullptr;
    cancelAsyncTask(m_rmvpeAlive);
    cancelAsyncTask(m_gameAlive);
}

void PitchLabelerPage::onSliceSelectedImpl(const QString &sliceId) {
    m_currentFile.reset();

    if (!source()) {
        m_editor->clear();
        return;
    }

    const QString audioPath = source()->validatedAudioPath(sliceId);

    auto result = source()->loadSlice(sliceId);
    if (result) {
        const auto &doc = result.value();
        auto file = std::make_shared<pitchlabeler::DSFile>();

        for (const auto &curve : doc.curves) {
            if (curve.name == QStringLiteral("pitch")) {
                file->f0.values = curve.values;
                file->f0.timestep = curve.timestep;
            }
        }

        for (const auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("midi")) {
                for (size_t i = 0; i + 1 < layer.boundaries.size(); i += 2) {
                    pitchlabeler::Note note;
                    note.start = layer.boundaries[i].pos;
                    note.name = layer.boundaries[i].text;
                    note.duration = layer.boundaries[i + 1].pos - layer.boundaries[i].pos;
                    file->notes.push_back(std::move(note));
                }
            }
        }

        m_currentFile = file;
        m_editor->loadDSFile(file);

        if (!audioPath.isEmpty())
            m_editor->loadAudio(audioPath, audioDurationSec(doc));
    } else if (!audioPath.isEmpty()) {
        m_currentFile = std::make_shared<pitchlabeler::DSFile>();
        m_editor->loadDSFile(m_currentFile);
        m_editor->loadAudio(audioPath, 0.0);
    }
}

bool PitchLabelerPage::saveCurrentSlice() {
    if (currentSliceId().isEmpty() || !source() || !m_currentFile)
        return true;

    auto result = source()->loadSlice(currentSliceId());
    DsTextDocument doc;
    if (result)
        doc = std::move(result.value());

    CurveLayer *pitchCurve = nullptr;
    for (auto &curve : doc.curves) {
        if (curve.name == QStringLiteral("pitch")) {
            pitchCurve = &curve;
            break;
        }
    }
    if (!pitchCurve) {
        doc.curves.push_back({});
        pitchCurve = &doc.curves.back();
        pitchCurve->name = QStringLiteral("pitch");
    }
    pitchCurve->values = m_currentFile->f0.values;
    pitchCurve->timestep = m_currentFile->f0.timestep;

    IntervalLayer *midiLayer = nullptr;
    for (auto &layer : doc.layers) {
        if (layer.name == QStringLiteral("midi")) {
            midiLayer = &layer;
            break;
        }
    }
    if (!midiLayer) {
        doc.layers.push_back({});
        midiLayer = &doc.layers.back();
        midiLayer->name = QStringLiteral("midi");
        midiLayer->type = QStringLiteral("note");
    }
    midiLayer->boundaries.clear();
    int id = 1;
    for (const auto &note : m_currentFile->notes) {
        Boundary startB;
        startB.id = id++;
        startB.pos = note.start;
        startB.text = note.name;
        midiLayer->boundaries.push_back(std::move(startB));

        Boundary endB;
        endB.id = id++;
        endB.pos = note.start + note.duration;
        endB.text.clear();
        midiLayer->boundaries.push_back(std::move(endB));
    }

    if (!doc.meta.editedSteps.contains(QStringLiteral("pitch_review")))
        doc.meta.editedSteps.append(QStringLiteral("pitch_review"));

    auto saveResult = source()->saveSlice(currentSliceId(), doc);
    if (!saveResult) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                             QString::fromStdString(saveResult.error()));
        return false;
    }

    return true;
}

QMenuBar *PitchLabelerPage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);

    auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(m_editor->saveAction());
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("退出(&X)"), this, [this]() {
        if (auto *w = window()) w->close();
    });

    auto *editMenu = bar->addMenu(QStringLiteral("编辑(&E)"));
    editMenu->addAction(m_editor->undoAction());
    editMenu->addAction(m_editor->redoAction());
    editMenu->addSeparator();
    editMenu->addAction(prevAction());
    editMenu->addAction(nextAction());
    editMenu->addSeparator();
    {
        auto *shortcutAction = editMenu->addAction(QStringLiteral("快捷键设置..."));
        connect(shortcutAction, &QAction::triggered, this, [this]() {
            shortcutManager()->showEditor(this);
        });
    }

    auto *processMenu = bar->addMenu(QStringLiteral("处理(&P)"));
    processMenu->addAction(m_extractPitchAction);
    processMenu->addAction(m_extractMidiAction);
    processMenu->addAction(QStringLiteral("计算 ph_num (当前切片)"),
                           this, &PitchLabelerPage::onAddPhNum);
    processMenu->addSeparator();
    processMenu->addAction(QStringLiteral("批量提取音高 + MIDI..."),
                           this, &PitchLabelerPage::onBatchExtract);

    auto *viewMenu = bar->addMenu(QStringLiteral("视图(&V)"));
    viewMenu->addAction(m_editor->zoomInAction());
    viewMenu->addAction(m_editor->zoomOutAction());
    viewMenu->addAction(m_editor->zoomResetAction());
    viewMenu->addSeparator();
    dsfw::Theme::instance().populateThemeMenu(viewMenu);

    auto *toolsMenu = bar->addMenu(QStringLiteral("工具(&T)"));
    toolsMenu->addAction(m_editor->abCompareAction());

    return bar;
}

QWidget *PitchLabelerPage::createStatusBarContent(QWidget *parent) {
    auto *container = new QWidget(parent);
    StatusBarBuilder builder(container);

    auto *sliceLabel = builder.addLabel(QStringLiteral("未选择切片"), 1);
    auto *posLabel = builder.addLabel(QStringLiteral("00:00.000"));
    auto *zoomLabel = builder.addLabel(QStringLiteral("100%"));
    auto *noteLabel = builder.addLabel(QStringLiteral("音符: 0"));

    builder.connect(this, &PitchLabelerPage::sliceChanged, sliceLabel,
                    [sliceLabel](const QString &id) {
        sliceLabel->setText(id.isEmpty() ? QStringLiteral("未选择切片") : id);
    });
    builder.connect(m_editor, &pitchlabeler::PitchEditor::positionChanged, posLabel,
                    [posLabel](double sec) {
        int minutes = static_cast<int>(sec) / 60;
        double seconds = sec - minutes * 60;
        posLabel->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
                                          .arg(seconds, 6, 'f', 3, QChar('0')));
    });
    builder.connect(m_editor, &pitchlabeler::PitchEditor::zoomChanged, zoomLabel,
                    [zoomLabel](int percent) {
        zoomLabel->setText(QString::number(percent) + "%");
    });
    builder.connect(m_editor, &pitchlabeler::PitchEditor::noteCountChanged, noteLabel,
                    [noteLabel](int count) {
        noteLabel->setText(QStringLiteral("音符: %1").arg(count));
    });

    return container;
}

void PitchLabelerPage::ensureRmvpeEngine() {
    if (m_rmvpe && m_rmvpe->is_open())
        return;

    if (!m_modelManager) {
        m_modelManager = ServiceLocator::get<IModelManager>();
        if (m_modelManager) {
            connect(m_modelManager, &IModelManager::modelInvalidated,
                    this, &PitchLabelerPage::onModelInvalidated);
        }
    }

    if (!m_modelManager)
        return;

    auto *mm = dynamic_cast<ModelManager *>(m_modelManager);
    if (!mm)
        return;

    auto config = readModelConfig(settingsBackend(), QStringLiteral("pitch_extraction"));
    if (config.modelPath.isEmpty())
        return;

    auto result = mm->loadModel(QStringLiteral("pitch_extraction"), config, config.deviceId);
    if (!result)
        return;

    auto typeId = registerModelType("pitch_extraction");
    auto *provider = mm->provider(typeId);
    auto *rmvpeProvider = dynamic_cast<InferenceModelProvider<Rmvpe::Rmvpe> *>(provider);
    if (rmvpeProvider && rmvpeProvider->engine().is_open()) {
        m_rmvpe = &rmvpeProvider->engine();
        m_rmvpeAlive = std::make_shared<std::atomic<bool>>(true);
    }
}

void PitchLabelerPage::ensureGameEngine() {
    if (m_game && m_game->isOpen())
        return;

    if (!m_modelManager) {
        m_modelManager = ServiceLocator::get<IModelManager>();
        if (m_modelManager) {
            connect(m_modelManager, &IModelManager::modelInvalidated,
                    this, &PitchLabelerPage::onModelInvalidated);
        }
    }

    if (!m_modelManager)
        return;

    auto *mm = dynamic_cast<ModelManager *>(m_modelManager);
    if (!mm)
        return;

    auto config = readModelConfig(settingsBackend(), QStringLiteral("midi_transcription"));
    if (config.modelPath.isEmpty())
        return;

    auto result = mm->loadModel(QStringLiteral("midi_transcription"), config, config.deviceId);
    if (!result)
        return;

    auto typeId = registerModelType("midi_transcription");
    auto *provider = mm->provider(typeId);
    auto *gameProvider = dynamic_cast<InferenceModelProvider<Game::Game> *>(provider);
    if (gameProvider && gameProvider->engine().isOpen()) {
        m_game = &gameProvider->engine();
        m_gameAlive = std::make_shared<std::atomic<bool>>(true);
    }
}

void PitchLabelerPage::onModelInvalidated(const QString &taskKey) {
    if (taskKey == QStringLiteral("pitch_extraction")) {
        if (m_rmvpeAlive)
            m_rmvpeAlive->store(false);
        m_rmvpe = nullptr;
        cancelAsyncTask(m_rmvpeAlive);
        DSFW_LOG_WARN("infer", "Pitch extraction task cancelled: model invalidated");
    } else if (taskKey == QStringLiteral("midi_transcription")) {
        if (m_gameAlive)
            m_gameAlive->store(false);
        m_game = nullptr;
        cancelAsyncTask(m_gameAlive);
        DSFW_LOG_WARN("infer", "MIDI transcription task cancelled: model invalidated");
    }
}

void PitchLabelerPage::ensureRmvpeEngineAsync(std::function<void()> onReady) {
    if (m_rmvpe && m_rmvpe->is_open()) {
        if (onReady) onReady();
        return;
    }
    QPointer<PitchLabelerPage> guard(this);
    QTimer::singleShot(0, this, [this, guard, onReady = std::move(onReady)]() {
        if (!guard) return;
        ensureRmvpeEngine();
        if (m_rmvpe && m_rmvpe->is_open() && onReady)
            onReady();
    });
}

void PitchLabelerPage::ensureGameEngineAsync(std::function<void()> onReady) {
    if (m_game && m_game->isOpen()) {
        if (onReady) onReady();
        return;
    }
    QPointer<PitchLabelerPage> guard(this);
    QTimer::singleShot(0, this, [this, guard, onReady = std::move(onReady)]() {
        if (!guard) return;
        ensureGameEngine();
        if (m_game && m_game->isOpen() && onReady)
            onReady();
    });
}

void PitchLabelerPage::onAutoInfer() {
    updateProgress();

    if (settingsBackend()) {
        auto settingsData = settingsBackend()->load();
        auto preload = settingsData["preload"].toObject();

        auto pitchPreload = preload["pitch_extraction"].toObject();
        if (pitchPreload["enabled"].toBool(false))
            ensureRmvpeEngineAsync();

        auto midiPreload = preload["midi_transcription"].toObject();
        if (midiPreload["enabled"].toBool(false))
            ensureGameEngineAsync();
    }

    // Auto-run inference for dirty layers
    if (!source() || currentSliceId().isEmpty())
        return;

    auto dirty = source()->dirtyLayers(currentSliceId());

    bool needPitch = dirty.contains(QStringLiteral("pitch"));
    bool needMidi = dirty.contains(QStringLiteral("midi"));

    if (needPitch) {
        source()->clearDirtyLayers(currentSliceId(), {QStringLiteral("pitch")});
        ensureRmvpeEngineAsync([this]() {
            if (m_rmvpe && m_rmvpe->is_open() && !m_inferRunning) {
                dsfw::widgets::ToastNotification::show(
                    this, dsfw::widgets::ToastType::Info,
                    QStringLiteral("自动提取音高..."), 3000);
                runPitchExtraction(currentSliceId());
            }
        });
    }

    if (needMidi) {
        source()->clearDirtyLayers(currentSliceId(), {QStringLiteral("midi")});
        ensureGameEngineAsync([this]() {
            if (m_game && m_game->isOpen() && !m_inferRunning) {
                dsfw::widgets::ToastNotification::show(
                    this, dsfw::widgets::ToastType::Info,
                    QStringLiteral("自动转写 MIDI..."), 3000);
                runMidiTranscription(currentSliceId());
            }
        });
    }
}

void PitchLabelerPage::onExtractPitch() {
    if (currentSliceId().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提取音高"),
                                 QStringLiteral("请先选择一个切片。"));
        return;
    }

    ensureRmvpeEngine();
    if (!m_rmvpe || !m_rmvpe->is_open()) {
        QMessageBox::warning(this, QStringLiteral("提取音高"),
                             QStringLiteral("RMVPE 模型未加载。请在设置中配置模型路径。"));
        return;
    }

    if (m_inferRunning) {
        QMessageBox::information(this, QStringLiteral("提取音高"),
                                 QStringLiteral("推理正在运行中，请稍候。"));
        return;
    }

    runPitchExtraction(currentSliceId());
}

void PitchLabelerPage::onExtractMidi() {
    if (currentSliceId().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提取 MIDI"),
                                 QStringLiteral("请先选择一个切片。"));
        return;
    }

    ensureGameEngine();
    if (!m_game || !m_game->isOpen()) {
        QMessageBox::warning(this, QStringLiteral("提取 MIDI"),
                             QStringLiteral("GAME 模型未加载。请在设置中配置模型路径。"));
        return;
    }

    if (m_inferRunning) {
        QMessageBox::information(this, QStringLiteral("提取 MIDI"),
                                 QStringLiteral("推理正在运行中，请稍候。"));
        return;
    }

    runMidiTranscription(currentSliceId());
}

void PitchLabelerPage::onAddPhNum() {
    if (currentSliceId().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("计算 ph_num"),
                                 QStringLiteral("请先选择一个切片。"));
        return;
    }
    runAddPhNum(currentSliceId());
}

void PitchLabelerPage::runPitchExtraction(const QString &sliceId) {
    if (!source())
        return;

    QString audioPath = source()->validatedAudioPath(sliceId);
    if (audioPath.isEmpty()) {
        if (source()->audioPath(sliceId).isEmpty())
            QMessageBox::warning(this, QStringLiteral("提取音高"),
                                 QStringLiteral("当前切片没有音频文件。"));
        return;
    }

    m_inferRunning = true;
    DSFW_LOG_INFO("infer", ("Pitch extraction started: " + sliceId.toStdString()).c_str());
    auto *rmvpe = m_rmvpe;
    auto rmvpeAlive = m_rmvpeAlive;
    QPointer<PitchLabelerPage> guard(this);

    (void) QtConcurrent::run([rmvpe, rmvpeAlive, audioPath, sliceId, guard]() {
        if (!rmvpeAlive || !*rmvpeAlive)
            return;
        if (!rmvpe)
            return;
        std::vector<Rmvpe::RmvpeRes> results;
        dstools::Result<void> result = Err("Not executed");

        try {
            result = rmvpe->get_f0(audioPath.toStdWString(), 0.03f, results, nullptr);
        } catch (const std::exception &e) {
            DSFW_LOG_ERROR("infer", ("Pitch extraction exception: " + sliceId.toStdString() + " - " + e.what()).c_str());
            result = Err(std::string("Exception: ") + e.what());
        }

        if (!guard)
            return;

        QMetaObject::invokeMethod(guard.data(), [guard, sliceId, result = std::move(result),
                                          results = std::move(results)]() {
            if (!guard)
                return;
            guard->m_inferRunning = false;
            if (result && !results.empty()) {
                const auto &res = results[0];
                std::vector<int32_t> f0Mhz(res.f0.size());
                for (size_t i = 0; i < res.f0.size(); ++i) {
                    f0Mhz[i] = static_cast<int32_t>(res.f0[i] * 1000.0f);
                }
                float timestep = res.f0.size() > 1 && res.offset >= 0
                                     ? (1.0f / 16000.0f)
                                     : 0.005f;
                guard->applyPitchResult(sliceId, f0Mhz, timestep);
                DSFW_LOG_INFO("infer", ("Pitch extraction completed: " + sliceId.toStdString()
                              + " - " + std::to_string(f0Mhz.size()) + " frames").c_str());
                dsfw::widgets::ToastNotification::show(
                    guard.data(), dsfw::widgets::ToastType::Info,
                    QStringLiteral("音高提取完成"), 3000);
            } else {
                QMessageBox::warning(guard.data(), QStringLiteral("提取音高"),
                                     QStringLiteral("音高提取失败。"));
            }
        }, Qt::QueuedConnection);
    });
}

void PitchLabelerPage::runMidiTranscription(const QString &sliceId) {
    if (!source())
        return;

    QString audioPath = source()->validatedAudioPath(sliceId);
    if (audioPath.isEmpty()) {
        if (source()->audioPath(sliceId).isEmpty())
            QMessageBox::warning(this, QStringLiteral("提取 MIDI"),
                                 QStringLiteral("当前切片没有音频文件。"));
        return;
    }

    m_inferRunning = true;
    DSFW_LOG_INFO("infer", ("MIDI transcription started: " + sliceId.toStdString()).c_str());
    auto *game = m_game;
    auto gameAlive = m_gameAlive;
    QPointer<PitchLabelerPage> guard(this);

    (void) QtConcurrent::run([game, gameAlive, audioPath, sliceId, guard]() {
        if (!gameAlive || !*gameAlive)
            return;
        if (!game)
            return;
        std::vector<Game::GameNote> notes;
        dstools::Result<void> result = Err("Not executed");

        try {
            result = game->getNotes(audioPath.toStdWString(), notes, nullptr);
        } catch (const std::exception &e) {
            DSFW_LOG_ERROR("infer", ("MIDI transcription exception: " + sliceId.toStdString() + " - " + e.what()).c_str());
            result = Err(std::string("Exception: ") + e.what());
        }

        if (!guard)
            return;

        QMetaObject::invokeMethod(guard.data(), [guard, sliceId, result = std::move(result),
                                          notes = std::move(notes)]() {
            if (!guard)
                return;
            guard->m_inferRunning = false;
            if (result) {
                guard->applyMidiResult(sliceId, notes);
                DSFW_LOG_INFO("infer", ("MIDI transcription completed: " + sliceId.toStdString()
                              + " - " + std::to_string(notes.size()) + " notes").c_str());
                dsfw::widgets::ToastNotification::show(
                    guard.data(), dsfw::widgets::ToastType::Info,
                    QStringLiteral("MIDI 转录完成"), 3000);
            } else {
                QMessageBox::warning(guard.data(), QStringLiteral("提取 MIDI"),
                                     QStringLiteral("MIDI 转录失败。"));
            }
        }, Qt::QueuedConnection);
    });
}

void PitchLabelerPage::runAddPhNum(const QString &sliceId) {
    if (!source())
        return;

    auto result = source()->loadSlice(sliceId);
    if (!result) {
        QMessageBox::warning(this, QStringLiteral("计算 ph_num"),
                             QStringLiteral("无法加载切片数据。"));
        return;
    }

    DsTextDocument doc = std::move(result.value());

    QStringList phSeq;
    for (const auto &layer : doc.layers) {
        if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive)) {
            for (const auto &b : layer.boundaries) {
                if (!b.text.isEmpty())
                    phSeq << b.text;
            }
        }
    }

    if (phSeq.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("计算 ph_num"),
                                 QStringLiteral("当前切片没有音素数据，请先进行强制对齐。"));
        return;
    }

    QString phNumStr;
    QString error;
    if (m_phNumCalc.calculate(phSeq.join(' '), phNumStr, error)) {
        IntervalLayer *phNumLayer = nullptr;
        for (auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("ph_num")) {
                phNumLayer = &layer;
                break;
            }
        }
        if (!phNumLayer) {
            doc.layers.push_back({});
            phNumLayer = &doc.layers.back();
            phNumLayer->name = QStringLiteral("ph_num");
            phNumLayer->type = QStringLiteral("attribute");
        }

        phNumLayer->boundaries.clear();
        const auto parts = phNumStr.split(QChar(' '), Qt::SkipEmptyParts);
        int id = 1;
        for (const auto &part : parts) {
            Boundary b;
            b.id = id++;
            b.text = part;
            phNumLayer->boundaries.push_back(std::move(b));
        }

        (void) source()->saveSlice(sliceId, doc);

        dsfw::widgets::ToastNotification::show(
            this, dsfw::widgets::ToastType::Info,
            QStringLiteral("ph_num 计算完成"), 3000);
    } else {
        QMessageBox::warning(this, QStringLiteral("计算 ph_num"),
                             QStringLiteral("ph_num 计算失败: %1").arg(error));
    }
}

void PitchLabelerPage::applyPitchResult(const QString &sliceId,
                                            const std::vector<int32_t> &f0,
                                            float timestep) {
    if (!source())
        return;

    auto result = source()->loadSlice(sliceId);
    DsTextDocument doc;
    if (result)
        doc = std::move(result.value());

    CurveLayer *pitchCurve = nullptr;
    for (auto &curve : doc.curves) {
        if (curve.name == QStringLiteral("pitch")) {
            pitchCurve = &curve;
            break;
        }
    }
    if (!pitchCurve) {
        doc.curves.push_back({});
        pitchCurve = &doc.curves.back();
        pitchCurve->name = QStringLiteral("pitch");
    }
    pitchCurve->values = f0;
    pitchCurve->timestep = timestep;

    (void) source()->saveSlice(sliceId, doc);

    if (sliceId == currentSliceId()) {
        if (!m_currentFile)
            m_currentFile = std::make_shared<pitchlabeler::DSFile>();
        m_currentFile->f0.values = f0;
        m_currentFile->f0.timestep = timestep;
        m_editor->loadDSFile(m_currentFile);
    }
}

void PitchLabelerPage::applyMidiResult(const QString &sliceId,
                                           const std::vector<Game::GameNote> &notes) {
    if (!source())
        return;

    auto result = source()->loadSlice(sliceId);
    DsTextDocument doc;
    if (result)
        doc = std::move(result.value());

    IntervalLayer *midiLayer = nullptr;
    for (auto &layer : doc.layers) {
        if (layer.name == QStringLiteral("midi")) {
            midiLayer = &layer;
            break;
        }
    }
    if (!midiLayer) {
        doc.layers.push_back({});
        midiLayer = &doc.layers.back();
        midiLayer->name = QStringLiteral("midi");
        midiLayer->type = QStringLiteral("note");
    }

    midiLayer->boundaries.clear();
    int id = 1;
    for (const auto &note : notes) {
        Boundary startB;
        startB.id = id++;
        startB.pos = static_cast<int64_t>(note.onset * 1000000.0);
        int midiNote = static_cast<int>(note.pitch + 0.5);
        int octave = midiNote / 12 - 1;
        int pc = midiNote % 12;
        static const char *pcNames[] = {"C", "C#", "D", "D#", "E", "F",
                                         "F#", "G", "G#", "A", "A#", "B"};
        startB.text = QStringLiteral("%1%2").arg(pcNames[pc]).arg(octave);
        midiLayer->boundaries.push_back(std::move(startB));

        Boundary endB;
        endB.id = id++;
        endB.pos = static_cast<int64_t>((note.onset + note.duration) * 1000000.0);
        endB.text.clear();
        midiLayer->boundaries.push_back(std::move(endB));
    }

    (void) source()->saveSlice(sliceId, doc);

    if (sliceId == currentSliceId()) {
        if (!m_currentFile)
            m_currentFile = std::make_shared<pitchlabeler::DSFile>();
        m_currentFile->notes.clear();
        for (const auto &note : notes) {
            pitchlabeler::Note n;
            n.start = static_cast<int64_t>(note.onset * 1000000.0);
            n.duration = static_cast<int64_t>(note.duration * 1000000.0);
            int midiNote = static_cast<int>(note.pitch + 0.5);
            int octave = midiNote / 12 - 1;
            int pc = midiNote % 12;
            static const char *pcNames[] = {"C", "C#", "D", "D#", "E", "F",
                                             "F#", "G", "G#", "A", "A#", "B"};
            n.name = QStringLiteral("%1%2").arg(pcNames[pc]).arg(octave);
            m_currentFile->notes.push_back(std::move(n));
        }
        m_editor->loadDSFile(m_currentFile);
    }
}

void PitchLabelerPage::onBatchExtract() {
    if (!source()) {
        QMessageBox::warning(this, QStringLiteral("批量提取"),
                             QStringLiteral("请先打开工程。"));
        return;
    }

    ensureRmvpeEngine();
    ensureGameEngine();

    bool hasRmvpe = m_rmvpe && m_rmvpe->is_open();
    bool hasGame = m_game && m_game->isOpen();

    if (!hasRmvpe && !hasGame) {
        QMessageBox::warning(this, QStringLiteral("批量提取"),
                             QStringLiteral("RMVPE 和 GAME 模型均未加载。请在设置中配置模型路径。"));
        return;
    }

    if (m_inferRunning) {
        QMessageBox::information(this, QStringLiteral("批量提取"),
                                 QStringLiteral("推理正在运行中，请稍候。"));
        return;
    }

    const auto ids = source()->sliceIds();
    if (ids.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("批量提取"),
                                 QStringLiteral("没有可处理的切片。"));
        return;
    }

    auto *dlg = new BatchProcessDialog(QStringLiteral("批量提取音高 + MIDI"), this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    auto *extractPitch = new QCheckBox(QStringLiteral("提取音高 (RMVPE)"), dlg);
    extractPitch->setChecked(hasRmvpe);
    extractPitch->setEnabled(hasRmvpe);
    dlg->addParamWidget(extractPitch);

    auto *extractMidi = new QCheckBox(QStringLiteral("提取 MIDI (GAME)"), dlg);
    extractMidi->setChecked(hasGame);
    extractMidi->setEnabled(hasGame);
    dlg->addParamWidget(extractMidi);

    auto *skipExisting = new QCheckBox(QStringLiteral("跳过已有结果的切片"), dlg);
    skipExisting->setChecked(true);
    dlg->addParamWidget(skipExisting);

    dlg->appendLog(QStringLiteral("总切片数: %1").arg(ids.size()));

    auto *rmvpe = m_rmvpe;
    auto *game = m_game;
    auto rmvpeAlive = m_rmvpeAlive;
    auto gameAlive = m_gameAlive;
    auto *src = source();
    QPointer<PitchLabelerPage> guard(this);

    connect(dlg, &BatchProcessDialog::started, this,
            [dlg, rmvpe, game, rmvpeAlive, gameAlive, src, ids, guard, extractPitch, extractMidi, skipExisting]() {
        if (!guard) {
            dlg->finish(0, 0, 0);
            return;
        }
        guard->m_inferRunning = true;
        bool doPitch = extractPitch->isChecked();
        bool doMidi = extractMidi->isChecked();
        bool skip = skipExisting->isChecked();

        (void) QtConcurrent::run([dlg, rmvpe, game, rmvpeAlive, gameAlive, src, ids, guard, doPitch, doMidi, skip]() {
            int processed = 0;
            int skipped = 0;
            int errors = 0;
            int idx = 0;
            try {
            for (const auto &sliceId : ids) {
                if (dlg->isCancelled())
                    break;
                if ((doPitch && (!rmvpeAlive || !*rmvpeAlive)) ||
                    (doMidi && (!gameAlive || !*gameAlive)))
                    break;

                QMetaObject::invokeMethod(dlg, [dlg, idx, total = ids.size()]() {
                    dlg->setProgress(idx, total);
                }, Qt::QueuedConnection);

                QString audioPath = src->validatedAudioPath(sliceId);
                if (audioPath.isEmpty()) {
                    ++skipped;
                    QMetaObject::invokeMethod(dlg, [dlg, sliceId]() {
                        dlg->appendLog(QStringLiteral("[SKIP] %1 (missing audio)").arg(sliceId));
                    }, Qt::QueuedConnection);
                    ++idx;
                    continue;
                }

                if (skip) {
                    auto result = src->loadSlice(sliceId);
                    if (result) {
                        bool hasData = false;
                        for (const auto &curve : result.value().curves) {
                            if (curve.name == QStringLiteral("pitch") && !curve.values.empty()) {
                                hasData = true;
                                break;
                            }
                        }
                        if (!hasData) {
                            for (const auto &layer : result.value().layers) {
                                if (layer.name == QStringLiteral("midi") && !layer.boundaries.empty()) {
                                    hasData = true;
                                    break;
                                }
                            }
                        }
                        if (hasData) {
                            ++skipped;
                            QMetaObject::invokeMethod(dlg, [dlg, sliceId]() {
                                dlg->appendLog(QStringLiteral("[SKIP] %1 (existing data)").arg(sliceId));
                            }, Qt::QueuedConnection);
                            ++idx;
                            continue;
                        }
                    }
                }

                if (doPitch && rmvpe) {
                    std::vector<Rmvpe::RmvpeRes> results;
                    auto result = rmvpe->get_f0(audioPath.toStdWString(), 0.03f, results, nullptr);
                    if (result && !results.empty()) {
                        const auto &res = results[0];
                        std::vector<int32_t> f0Mhz(res.f0.size());
                        for (size_t i = 0; i < res.f0.size(); ++i) {
                            f0Mhz[i] = static_cast<int32_t>(res.f0[i] * 1000.0f);
                        }
                        float timestep = 0.005f;
                        QMetaObject::invokeMethod(guard.data(), [guard, sliceId, f0Mhz, timestep]() {
                            if (guard)
                                guard->applyPitchResult(sliceId, f0Mhz, timestep);
                        }, Qt::QueuedConnection);
                    }
                }

                if (doMidi && game) {
                    std::vector<Game::GameNote> notes;
                    auto result = game->getNotes(audioPath.toStdWString(), notes, nullptr);
                    if (result) {
                        QMetaObject::invokeMethod(guard.data(), [guard, sliceId, notes = std::move(notes)]() {
                            if (guard)
                                guard->applyMidiResult(sliceId, notes);
                        }, Qt::QueuedConnection);
                    }
                }

                QMetaObject::invokeMethod(dlg, [dlg, sliceId]() {
                    dlg->appendLog(QStringLiteral("[OK] %1").arg(sliceId));
                }, Qt::QueuedConnection);
                ++processed;
                ++idx;
            }
            } catch (const std::exception &e) {
                DSFW_LOG_ERROR("infer", ("Batch pitch/MIDI exception: " + std::string(e.what())).c_str());
                QMetaObject::invokeMethod(dlg, [dlg, eMsg = std::string(e.what())]() {
                    dlg->appendLog(QStringLiteral("[ERROR] Exception: %1").arg(QString::fromStdString(eMsg)));
                }, Qt::QueuedConnection);
            }
            QMetaObject::invokeMethod(dlg, [dlg, processed, skipped, errors]() {
                dlg->finish(processed, skipped, errors);
            }, Qt::QueuedConnection);
            QMetaObject::invokeMethod(guard.data(), [guard]() {
                if (guard)
                    guard->m_inferRunning = false;
            }, Qt::QueuedConnection);
        });
    });

    connect(dlg, &BatchProcessDialog::cancelled, this, [guard]() {
        if (guard)
            guard->m_inferRunning = false;
    });

    dlg->show();
}

void PitchLabelerPage::updateProgress() {
    if (!source()) return;
    const auto ids = source()->sliceIds();
    int total = ids.size();
    int completed = 0;
    for (const auto &id : ids) {
        auto result = source()->loadSlice(id);
        if (result) {
            for (const auto &layer : result.value().layers) {
                if (layer.name == QStringLiteral("pitch") && !layer.boundaries.empty()) {
                    ++completed;
                    break;
                }
            }
        }
    }
    sliceListPanel()->setProgress(completed, total);
}

} // namespace dstools
