#include "PitchLabelerPage.h"
#include "BatchProcessDialog.h"
#include "ISettingsBackend.h"
#include "ModelConfigHelper.h"
#include "SliceListPanel.h"

#include <dstools/IEditorDataSource.h>

#include <dsfw/Log.h>

#include <DSFile.h>
#include <rmvpe-infer/Rmvpe.h>
#include <game-infer/Game.h>

#include <QCheckBox>
#include <QJsonObject>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointer>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent>

#include <dstools/PitchUtils.h>
#include <game-infer/Game.h>
#include <rmvpe-infer/Rmvpe.h>

#include <dsfw/CommonKeys.h>
#include <dsfw/IModelManager.h>
#include <dsfw/IModelProvider.h>
#include <dsfw/InferenceModelProvider.h>
#include <dsfw/Theme.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dstools/DsTextTypes.h>
#include <dstools/ExecutionProvider.h>
#include <dstools/ModelManager.h>

template<>
struct EngineTraits<Rmvpe::Rmvpe> {
    using ProviderType = dstools::InferenceModelProvider<Rmvpe::Rmvpe>;

    static bool isOpen(const Rmvpe::Rmvpe *engine) {
        return engine && engine->is_open();
    }
};

template<>
struct EngineTraits<Game::Game> {
    using ProviderType = dstools::InferenceModelProvider<Game::Game>;

    static bool isOpen(const Game::Game *engine) {
        return engine && engine->isOpen();
    }
};

namespace dstools {

    PitchLabelerPage::PitchLabelerPage(QWidget *parent) : EditorPageBase("PitchLabeler", parent) {
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

        shortcutManager()->bind(m_editor->saveAction(), kShortcutSave, tr("Save"), tr("File"));
        shortcutManager()->bind(m_editor->undoAction(), kShortcutUndo, tr("Undo"), tr("Edit"));
        shortcutManager()->bind(m_editor->redoAction(), kShortcutRedo, tr("Redo"), tr("Edit"));
        shortcutManager()->bind(m_editor->zoomInAction(), kShortcutZoomIn, tr("Zoom In"), tr("View"));
        shortcutManager()->bind(m_editor->zoomOutAction(), kShortcutZoomOut, tr("Zoom Out"), tr("View"));
        shortcutManager()->bind(m_editor->zoomResetAction(), kShortcutZoomReset, tr("Reset Zoom"), tr("View"));
        shortcutManager()->bind(m_editor->abCompareAction(), kShortcutABCompare, tr("A/B Compare"), tr("Tools"));
        shortcutManager()->bind(m_extractPitchAction, kShortcutExtractPitch, tr("Extract Pitch"), tr("Processing"));
        shortcutManager()->bind(m_extractMidiAction, kShortcutExtractMidi, tr("Extract MIDI"), tr("Processing"));
        shortcutManager()->bind(m_editor->playPauseAction(), kShortcutPlayPause, tr("Play/Pause"), tr("Playback"));
        shortcutManager()->applyAll();
        shortcutManager()->updateTooltips();
        shortcutManager()->setEnabled(false);

        connect(m_editor->saveAction(), &QAction::triggered, this, [this]() {
            saveCurrentSlice();
            updateDirtyIndicator();
        });
        connect(m_editor->undoStack(), &QUndoStack::cleanChanged, this, [this]() { updateDirtyIndicator(); });
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
        m_rmvpeAlive.invalidate();
        m_gameAlive.invalidate();
        m_rmvpe = nullptr;
        m_game = nullptr;
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

            if (!audioPath.isEmpty())
                file->setFilePath(audioPath);

            m_currentFile = file;
            m_editor->loadDSFile(file);

            if (!audioPath.isEmpty())
                m_editor->loadAudio(audioPath, audioDurationSec(doc));
        } else if (!audioPath.isEmpty()) {
            m_currentFile = std::make_shared<pitchlabeler::DSFile>();
            m_currentFile->setFilePath(audioPath);
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
            QMessageBox::warning(this, QStringLiteral("保存失败"), QString::fromStdString(saveResult.error()));
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
            if (auto *w = window())
                w->close();
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
            connect(shortcutAction, &QAction::triggered, this, [this]() { shortcutManager()->showEditor(this); });
        }

        auto *processMenu = bar->addMenu(QStringLiteral("处理(&P)"));
        processMenu->addAction(m_extractPitchAction);
        processMenu->addAction(m_extractMidiAction);
        processMenu->addAction(QStringLiteral("计算 ph_num (当前切片)"), this, &PitchLabelerPage::onAddPhNum);
        processMenu->addSeparator();
        processMenu->addAction(QStringLiteral("批量提取音高 + MIDI..."), this, &PitchLabelerPage::onBatchExtract);

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

        builder.connect(this, &PitchLabelerPage::sliceChanged, sliceLabel, [sliceLabel](const QString &id) {
            sliceLabel->setText(id.isEmpty() ? QStringLiteral("未选择切片") : id);
        });
        builder.connect(m_editor, &pitchlabeler::PitchEditor::positionChanged, posLabel, [posLabel](double sec) {
            int minutes = static_cast<int>(sec) / 60;
            double seconds = sec - minutes * 60;
            posLabel->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 6, 'f', 3, QChar('0')));
        });
        builder.connect(m_editor, &pitchlabeler::PitchEditor::zoomChanged, zoomLabel,
                        [zoomLabel](int percent) { zoomLabel->setText(QString::number(percent) + "%"); });
        builder.connect(m_editor, &pitchlabeler::PitchEditor::noteCountChanged, noteLabel,
                        [noteLabel](int count) { noteLabel->setText(QStringLiteral("音符: %1").arg(count)); });

        return container;
    }

    template<typename EngineType>
    void PitchLabelerPage::ensureEngine(EngineType *&enginePtr, EngineAliveToken &aliveToken,
                                        const QString &taskKey) {
        using Traits = EngineTraits<EngineType>;

        if (Traits::isOpen(enginePtr))
            return;

        auto *mgr = ensureModelManager();
        if (!mgr)
            return;

        if (!aliveToken.isValid()) {
            connect(mgr, &IModelManager::modelInvalidated, this, &PitchLabelerPage::onModelInvalidated);
        }

        auto [mm2, typeId] = loadModelForTask(taskKey);
        if (!mm2 || !typeId.isValid())
            return;

        auto *provider = mm2->provider(typeId);
        auto *typedProvider = dynamic_cast<typename Traits::ProviderType *>(provider);
        if (Traits::isOpen(&typedProvider->engine())) {
            enginePtr = &typedProvider->engine();
            aliveToken.create();
        }
    }

    template<typename EngineType>
    void PitchLabelerPage::ensureEngineAsync(EngineType *&enginePtr, EngineAliveToken &aliveToken,
                                             const QString &taskKey, std::function<void()> onReady) {
        using Traits = EngineTraits<EngineType>;

        if (Traits::isOpen(enginePtr)) {
            if (onReady)
                onReady();
            return;
        }

        QPointer<PitchLabelerPage> guard(this);
        QTimer::singleShot(0, this, [this, guard, onReady = std::move(onReady), &aliveToken, taskKey, &enginePtr]() {
            if (!guard)
                return;
            ensureEngine(enginePtr, aliveToken, taskKey);
            if (Traits::isOpen(enginePtr) && onReady)
                onReady();
        });
    }

    void PitchLabelerPage::ensureRmvpeEngine() {
        ensureEngine(m_rmvpe, m_rmvpeAlive, QStringLiteral("pitch_extraction"));
    }

    void PitchLabelerPage::ensureGameEngine() {
        ensureEngine(m_game, m_gameAlive, QStringLiteral("midi_transcription"));
    }

    void PitchLabelerPage::onModelInvalidated(const QString &taskKey) {
        if (taskKey == QStringLiteral("pitch_extraction")) {
            m_rmvpeAlive.invalidate();
            m_rmvpe = nullptr;
            DSFW_LOG_WARN("infer", "Pitch extraction task cancelled: model invalidated");
        } else if (taskKey == QStringLiteral("midi_transcription")) {
            m_gameAlive.invalidate();
            m_game = nullptr;
            DSFW_LOG_WARN("infer", "MIDI transcription task cancelled: model invalidated");
        }
    }

    void PitchLabelerPage::ensureRmvpeEngineAsync(std::function<void()> onReady) {
        ensureEngineAsync(m_rmvpe, m_rmvpeAlive, QStringLiteral("pitch_extraction"), std::move(onReady));
    }

    void PitchLabelerPage::ensureGameEngineAsync(std::function<void()> onReady) {
        ensureEngineAsync(m_game, m_gameAlive, QStringLiteral("midi_transcription"), std::move(onReady));
    }

    template void PitchLabelerPage::ensureEngine<Rmvpe::Rmvpe>(Rmvpe::Rmvpe *&, EngineAliveToken &, const QString &);
    template void PitchLabelerPage::ensureEngine<Game::Game>(Game::Game *&, EngineAliveToken &, const QString &);
    template void PitchLabelerPage::ensureEngineAsync<Rmvpe::Rmvpe>(Rmvpe::Rmvpe *&, EngineAliveToken &,
                                                                     const QString &, std::function<void()>);
    template void PitchLabelerPage::ensureEngineAsync<Game::Game>(Game::Game *&, EngineAliveToken &,
                                                                   const QString &, std::function<void()>);

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
                    dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info,
                                                           QStringLiteral("自动提取音高..."), 3000);
                    runPitchExtraction(currentSliceId());
                }
            });
        }

        if (needMidi) {
            source()->clearDirtyLayers(currentSliceId(), {QStringLiteral("midi")});
            ensureGameEngineAsync([this]() {
                if (m_game && m_game->isOpen() && !m_inferRunning) {
                    dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info,
                                                           QStringLiteral("自动转写 MIDI..."), 3000);
                    runMidiTranscription(currentSliceId());
                }
            });
        }
    }

    void PitchLabelerPage::onExtractPitch() {
        if (currentSliceId().isEmpty()) {
            QMessageBox::information(this, QStringLiteral("提取音高"), QStringLiteral("请先选择一个切片。"));
            return;
        }

        ensureRmvpeEngine();
        if (!m_rmvpe || !m_rmvpe->is_open()) {
            QMessageBox::warning(this, QStringLiteral("提取音高"),
                                 QStringLiteral("RMVPE 模型未加载。请在设置中配置模型路径。"));
            return;
        }

        if (m_inferRunning) {
            QMessageBox::information(this, QStringLiteral("提取音高"), QStringLiteral("推理正在运行中，请稍候。"));
            return;
        }

        runPitchExtraction(currentSliceId());
    }

    void PitchLabelerPage::onExtractMidi() {
        if (currentSliceId().isEmpty()) {
            QMessageBox::information(this, QStringLiteral("提取 MIDI"), QStringLiteral("请先选择一个切片。"));
            return;
        }

        ensureGameEngine();
        if (!m_game || !m_game->isOpen()) {
            QMessageBox::warning(this, QStringLiteral("提取 MIDI"),
                                 QStringLiteral("GAME 模型未加载。请在设置中配置模型路径。"));
            return;
        }

        if (m_inferRunning) {
            QMessageBox::information(this, QStringLiteral("提取 MIDI"), QStringLiteral("推理正在运行中，请稍候。"));
            return;
        }

        bool useAlignMode = false;
        Game::AlignInput alignInput;
        alignInput.wavPath = source()->validatedAudioPath(currentSliceId()).toStdWString();

        auto loadResult = source()->loadSlice(currentSliceId());
        if (loadResult) {
            const auto &doc = loadResult.value();
            for (const auto &layer : doc.layers) {
                if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive) && !layer.boundaries.empty()) {
                    const auto &bnd = layer.boundaries;
                    for (size_t i = 0; i < bnd.size(); ++i) {
                        if (!bnd[i].text.isEmpty()) {
                            alignInput.phSeq.push_back(bnd[i].text.toStdString());
                            TimePos dur = (i + 1 < bnd.size()) ? bnd[i + 1].pos - bnd[i].pos
                                                               : secToUs(audioDurationSec(doc)) - bnd[i].pos;
                            alignInput.phDur.push_back(static_cast<float>(usToSec(dur)));
                        }
                    }
                    if (!alignInput.phSeq.empty()) {
                        useAlignMode = true;
                    }
                    break;
                }
            }
        }

        if (useAlignMode) {
            bool hasPhNum = false;
            if (loadResult) {
                const auto &doc = loadResult.value();
                for (const auto &layer : doc.layers) {
                    if (layer.name == QStringLiteral("ph_num") && !layer.boundaries.empty()) {
                        for (const auto &b : layer.boundaries) {
                            bool ok = false;
                            int val = b.text.toInt(&ok);
                            alignInput.phNum.push_back(ok ? val : 0);
                        }
                        hasPhNum = true;
                        break;
                    }
                }
            }

            if (!hasPhNum) {
                bool dictConfigured = false;
                if (settingsBackend()) {
                    auto cfg = settingsBackend()->load()["phNumConfig"].toObject();
                    auto langs = cfg["languages"].toObject();
                    for (auto it = langs.begin(); it != langs.end(); ++it) {
                        auto langCfg = it.value().toObject();
                        QString dictPath = langCfg["dictPath"].toString();
                        if (!dictPath.isEmpty()) {
                            dictConfigured = true;
                            break;
                        }
                    }
                }

                if (!dictConfigured) {
                    QMessageBox::warning(
                        this, QStringLiteral("缺少 ph_num"),
                        QStringLiteral("当前切片缺少 ph_num 数据，且未配置 ph_num 词典。\n"
                                       "请先在 设置 → ph_num 中配置词典路径。"));
                    return;
                }

                dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info,
                                                       QStringLiteral("自动计算 ph_num..."), 3000);
                loadPhNumCalculator();
                runAddPhNum(currentSliceId());
                alignInput.phNum.clear();
                auto reload = source()->loadSlice(currentSliceId());
                if (reload) {
                    for (const auto &layer : reload->layers) {
                        if (layer.name == QStringLiteral("ph_num") && !layer.boundaries.empty()) {
                            for (const auto &b : layer.boundaries) {
                                bool ok = false;
                                int val = b.text.toInt(&ok);
                                alignInput.phNum.push_back(ok ? val : 0);
                            }
                            break;
                        }
                    }
                }
            }
        }

        runMidiTranscription(currentSliceId(), useAlignMode ? &alignInput : nullptr);
    }

    void PitchLabelerPage::onAddPhNum() {
        if (currentSliceId().isEmpty()) {
            QMessageBox::information(this, QStringLiteral("计算 ph_num"), QStringLiteral("请先选择一个切片。"));
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
                QMessageBox::warning(this, QStringLiteral("提取音高"), QStringLiteral("当前切片没有音频文件。"));
            return;
        }

        m_inferRunning = true;
        m_extractPitchAction->setEnabled(false);
        m_extractMidiAction->setEnabled(false);
        DSFW_LOG_INFO("infer", ("Pitch extraction started: " + sliceId.toStdString()).c_str());
        auto *rmvpe = m_rmvpe;
        auto rmvpeAlive = m_rmvpeAlive.token();
        QPointer<PitchLabelerPage> guard(this);

        (void) QtConcurrent::run([rmvpe, rmvpeAlive, audioPath, sliceId, guard]() {
            if (!rmvpeAlive || !*rmvpeAlive)
                return;
            if (!rmvpe)
                return;
            std::vector<Rmvpe::RmvpeRes> results;
            dstools::Result<void> result = Err("Not executed");

            try {
                result = rmvpe->get_f0(audioPath.toStdWString(), 0.01f, results, nullptr);
            } catch (const std::exception &e) {
                DSFW_LOG_ERROR("infer",
                               ("Pitch extraction exception: " + sliceId.toStdString() + " - " + e.what()).c_str());
                result = Err(std::string("Exception: ") + e.what());
            }

            if (!guard)
                return;

            QMetaObject::invokeMethod(
                guard.data(),
                [guard, sliceId, result = std::move(result), results = std::move(results)]() {
                    if (!guard)
                        return;
                    guard->m_inferRunning = false;
                    guard->m_extractPitchAction->setEnabled(true);
                    guard->m_extractMidiAction->setEnabled(true);
                    if (result && !results.empty()) {
                        const auto &res = results[0];
                        std::vector<int32_t> f0Mhz(res.f0.size());
                        for (size_t i = 0; i < res.f0.size(); ++i) {
                            f0Mhz[i] = static_cast<int32_t>(res.f0[i] * 1000.0f);
                        }
                        float timestep = 0.01f;
                        guard->applyPitchResult(sliceId, f0Mhz, timestep);
                        DSFW_LOG_INFO("infer", ("Pitch extraction completed: " + sliceId.toStdString() + " - " +
                                                std::to_string(f0Mhz.size()) + " frames")
                                                   .c_str());
                        dsfw::widgets::ToastNotification::show(guard.data(), dsfw::widgets::ToastType::Info,
                                                               QStringLiteral("音高提取完成"), 3000);
                    } else {
                        QMessageBox::warning(guard.data(), QStringLiteral("提取音高"),
                                             QStringLiteral("音高提取失败") +
                                                 (result ? QString() : QString::fromStdString(": " + result.error())));
                    }
                },
                Qt::QueuedConnection);
        });
    }

    void PitchLabelerPage::runMidiTranscription(const QString &sliceId, const Game::AlignInput *alignInput) {
        if (!source())
            return;

        QString audioPath = source()->validatedAudioPath(sliceId);
        if (audioPath.isEmpty()) {
            if (source()->audioPath(sliceId).isEmpty())
                QMessageBox::warning(this, QStringLiteral("提取 MIDI"), QStringLiteral("当前切片没有音频文件。"));
            return;
        }

        m_inferRunning = true;
        m_extractPitchAction->setEnabled(false);
        m_extractMidiAction->setEnabled(false);
        DSFW_LOG_INFO(
            "infer", ("MIDI transcription started: " + sliceId.toStdString() + (alignInput ? " (align)" : "")).c_str());
        auto *game = m_game;
        auto gameAlive = m_gameAlive.token();
        QPointer<PitchLabelerPage> guard(this);
        bool useAlign = (alignInput != nullptr);
        auto capturedInput = useAlign ? std::make_shared<Game::AlignInput>(*alignInput) : nullptr;

        (void) QtConcurrent::run([game, gameAlive, audioPath, sliceId, guard, useAlign, capturedInput, this]() {
            if (!gameAlive || !*gameAlive)
                return;
            if (!game)
                return;
            std::vector<Game::GameNote> notes;
            dstools::Result<void> result = Err("Not executed");

            try {
                if (useAlign && capturedInput) {
                    Game::AlignOptions options;

                    if (settingsBackend()) {
                        auto cfg = settingsBackend()->load()["pitchConfig"].toObject();
                        if (cfg.contains("uvVocab") && cfg["uvVocab"].isString()) {
                            QString uvStr = cfg["uvVocab"].toString();
                            std::set<std::string> uvVocab;
                            const auto parts = uvStr.split(QLatin1Char(','), Qt::SkipEmptyParts);
                            for (const auto &p : parts) {
                                std::string trimmed = p.trimmed().toStdString();
                                if (!trimmed.empty())
                                    uvVocab.insert(trimmed);
                            }
                            if (!uvVocab.empty())
                                options.uvVocab = uvVocab;
                        }
                        if (cfg.contains("uvWordCond") && cfg["uvWordCond"].isDouble()) {
                            int cond = cfg["uvWordCond"].toInt();
                            if (cond == 1)
                                options.uvWordCond = Game::UvWordCond::All;
                            else if (cond == 2)
                                options.uvWordCond = Game::UvWordCond::All;
                        }
                    }

                    std::vector<Game::AlignedNote> alignedNotes;
                    capturedInput->wavPath = std::filesystem::path(audioPath.toStdWString());
                    result = game->align(*capturedInput, options, alignedNotes);
                    if (result) {
                        float currentTime = 0.0f;
                        for (const auto &an : alignedNotes) {
                            Game::GameNote gn;
                            bool isRest = an.name.empty() || an.name == "rest";
                            gn.voiced = !isRest;
                            if (!isRest) {
                                auto parsed = dstools::parseNoteName(QString::fromStdString(an.name));
                                gn.pitch = parsed.valid ? parsed.midiNumber : 60.0f;
                            } else {
                                gn.pitch = 0.0f;
                            }
                            gn.onset = currentTime;
                            gn.duration = an.duration;
                            notes.push_back(gn);
                            currentTime += an.duration;
                        }
                    }
                } else {
                    result = game->getNotes(audioPath.toStdWString(), notes, nullptr);
                }
            } catch (const std::exception &e) {
                DSFW_LOG_ERROR("infer",
                               ("MIDI transcription exception: " + sliceId.toStdString() + " - " + e.what()).c_str());
                result = Err(std::string("Exception: ") + e.what());
            }

            if (!guard)
                return;

            QMetaObject::invokeMethod(
                guard.data(),
                [guard, sliceId, result = std::move(result), notes = std::move(notes)]() {
                    if (!guard)
                        return;
                    guard->m_inferRunning = false;
                    guard->m_extractPitchAction->setEnabled(true);
                    guard->m_extractMidiAction->setEnabled(true);
                    if (result) {
                        guard->applyMidiResult(sliceId, notes);
                        DSFW_LOG_INFO("infer", ("MIDI transcription completed: " + sliceId.toStdString() + " - " +
                                                std::to_string(notes.size()) + " notes")
                                                   .c_str());
                        dsfw::widgets::ToastNotification::show(guard.data(), dsfw::widgets::ToastType::Info,
                                                               QStringLiteral("MIDI 转录完成"), 3000);
                    } else {
                        QMessageBox::warning(guard.data(), QStringLiteral("提取 MIDI"),
                                             QStringLiteral("MIDI 转录失败") +
                                                 (result ? QString() : QString::fromStdString(": " + result.error())));
                    }
                },
                Qt::QueuedConnection);
        });
    }

    void PitchLabelerPage::loadPhNumCalculator() {
    if (!settingsBackend())
        return;

    auto settings = settingsBackend()->load();
    auto phNumCfg = settings["phNumConfig"].toObject();
    auto langs = phNumCfg["languages"].toObject();

    for (auto it = langs.begin(); it != langs.end(); ++it) {
        auto langCfg = it.value().toObject();
        QString dictPath = langCfg["dictPath"].toString();
        QString vowelsPath = langCfg["vowelsPath"].toString();
        QString consonantsPath = langCfg["consonantsPath"].toString();

        if (!dictPath.isEmpty()) {
            QString error;
            m_phNumCalc.loadDictionary(dictPath, error);
            return;
        }

        if (!vowelsPath.isEmpty() || !consonantsPath.isEmpty()) {
            QString error;
            if (!vowelsPath.isEmpty())
                m_phNumCalc.loadVowelsFromFile(vowelsPath, error);
            if (!consonantsPath.isEmpty())
                m_phNumCalc.loadConsonantsFromFile(consonantsPath, error);
            return;
        }
    }
}

void PitchLabelerPage::runAddPhNum(const QString &sliceId) {
        if (!source())
            return;

        loadPhNumCalculator();

        auto result = source()->loadSlice(sliceId);
        if (!result) {
            QMessageBox::warning(this, QStringLiteral("计算 ph_num"), QStringLiteral("无法加载切片数据。"));
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

            dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info,
                                                   QStringLiteral("ph_num 计算完成"), 3000);
        } else {
            QMessageBox::warning(this, QStringLiteral("计算 ph_num"), QStringLiteral("ph_num 计算失败: %1").arg(error));
        }
    }

    void PitchLabelerPage::applyPitchResult(const QString &sliceId, const std::vector<int32_t> &f0, float timestep) {
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
        pitchCurve->timestep = secToUs(static_cast<double>(timestep));

        (void) source()->saveSlice(sliceId, doc);

        if (sliceId == currentSliceId()) {
            if (!m_currentFile)
                m_currentFile = std::make_shared<pitchlabeler::DSFile>();
            m_currentFile->f0.values = f0;
            m_currentFile->f0.timestep = secToUs(static_cast<double>(timestep));
            m_editor->loadDSFile(m_currentFile);
        }
    }

    void PitchLabelerPage::applyMidiResult(const QString &sliceId, const std::vector<Game::GameNote> &notes) {
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
            static const char *pcNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
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
                static const char *pcNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
                n.name = QStringLiteral("%1%2").arg(pcNames[pc]).arg(octave);
                m_currentFile->notes.push_back(std::move(n));
            }
            m_editor->loadDSFile(m_currentFile);
        }
    }

    void PitchLabelerPage::onBatchExtract() {
        if (!source()) {
            QMessageBox::warning(this, QStringLiteral("批量提取"), QStringLiteral("请先打开工程。"));
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
            QMessageBox::information(this, QStringLiteral("批量提取"), QStringLiteral("推理正在运行中，请稍候。"));
            return;
        }

        const auto ids = source()->sliceIds();
        if (ids.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("批量提取"), QStringLiteral("没有可处理的切片。"));
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
        auto rmvpeAlive = m_rmvpeAlive.token();
        auto gameAlive = m_gameAlive.token();
        auto *src = source();
        QPointer<PitchLabelerPage> guard(this);

        connect(
            dlg, &BatchProcessDialog::started, this,
            [dlg, rmvpe, game, rmvpeAlive, gameAlive, src, ids, guard, extractPitch, extractMidi, skipExisting]() {
                if (!guard) {
                    dlg->finish(0, 0, 0);
                    return;
                }
                guard->m_inferRunning = true;
                guard->m_extractPitchAction->setEnabled(false);
                guard->m_extractMidiAction->setEnabled(false);
                bool doPitch = extractPitch->isChecked();
                bool doMidi = extractMidi->isChecked();
                bool skip = skipExisting->isChecked();

                (void) QtConcurrent::run([dlg, rmvpe, game, rmvpeAlive, gameAlive, src, ids, guard, doPitch, doMidi,
                                          skip]() {
                    int processed = 0;
                    int skipped = 0;
                    int errors = 0;
                    int idx = 0;
                    try {
                        for (const auto &sliceId : ids) {
                            if (dlg->isCancelled())
                                break;
                            if ((doPitch && (!rmvpeAlive || !*rmvpeAlive)) || (doMidi && (!gameAlive || !*gameAlive)))
                                break;

                            QMetaObject::invokeMethod(
                                dlg, [dlg, idx, total = ids.size()]() { dlg->setProgress(idx, total); },
                                Qt::QueuedConnection);

                            QString audioPath = src->validatedAudioPath(sliceId);
                            if (audioPath.isEmpty()) {
                                ++skipped;
                                QMetaObject::invokeMethod(
                                    dlg,
                                    [dlg, sliceId]() {
                                        dlg->appendLog(QStringLiteral("[SKIP] %1 (missing audio)").arg(sliceId));
                                    },
                                    Qt::QueuedConnection);
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
                                        QMetaObject::invokeMethod(
                                            dlg,
                                            [dlg, sliceId]() {
                                                dlg->appendLog(
                                                    QStringLiteral("[SKIP] %1 (existing data)").arg(sliceId));
                                            },
                                            Qt::QueuedConnection);
                                        ++idx;
                                        continue;
                                    }
                                }
                            }

                            if (doPitch && rmvpe) {
                                std::vector<Rmvpe::RmvpeRes> results;
                                auto result = rmvpe->get_f0(audioPath.toStdWString(), 0.01f, results, nullptr);
                                if (result && !results.empty()) {
                                    const auto &res = results[0];
                                    std::vector<int32_t> f0Mhz(res.f0.size());
                                    for (size_t i = 0; i < res.f0.size(); ++i) {
                                        f0Mhz[i] = static_cast<int32_t>(res.f0[i] * 1000.0f);
                                    }
                                    float timestep = 0.01f;
                                    QMetaObject::invokeMethod(
                                        guard.data(),
                                        [guard, sliceId, f0Mhz, timestep]() {
                                            if (guard)
                                                guard->applyPitchResult(sliceId, f0Mhz, timestep);
                                        },
                                        Qt::QueuedConnection);
                                } else {
                                    DSFW_LOG_ERROR("infer", (std::string("Batch pitch extraction failed: ") +
                                                             sliceId.toStdString() + " - " +
                                                             (result ? "empty results" : result.error()))
                                                                .c_str());
                                    QMetaObject::invokeMethod(
                                        dlg,
                                        [dlg, sliceId]() {
                                            dlg->appendLog(
                                                QStringLiteral("[ERROR] %1: pitch extraction failed").arg(sliceId));
                                        },
                                        Qt::QueuedConnection);
                                }
                            }

                            if (doMidi && game) {
                                // Try align mode if phoneme data exists
                                Game::AlignInput alignInput;
                                bool useAlign = false;

                                auto loadResult = src->loadSlice(sliceId);
                                if (loadResult) {
                                    const auto &doc = loadResult.value();
                                    for (const auto &layer : doc.layers) {
                                        if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive) &&
                                            !layer.boundaries.empty()) {
                                            const auto &bnd = layer.boundaries;
                                            for (size_t i = 0; i < bnd.size(); ++i) {
                                                if (!bnd[i].text.isEmpty()) {
                                                    alignInput.phSeq.push_back(bnd[i].text.toStdString());
                                                    TimePos dur = (i + 1 < bnd.size())
                                                                      ? bnd[i + 1].pos - bnd[i].pos
                                                                      : secToUs(audioDurationSec(doc)) - bnd[i].pos;
                                                    alignInput.phDur.push_back(static_cast<float>(usToSec(dur)));
                                                }
                                            }
                                            if (!alignInput.phSeq.empty()) {
                                                useAlign = true;
                                            }
                                            break;
                                        }
                                    }
                                }

                                std::vector<Game::GameNote> notes;
                                dstools::Result<void> midiResult = Err("Not executed");

                                if (useAlign) {
                                    alignInput.wavPath = std::filesystem::path(audioPath.toStdWString());
                                    Game::AlignOptions options;
                                    std::vector<Game::AlignedNote> alignedNotes;
                                    midiResult = game->align(alignInput, options, alignedNotes);
                                    if (midiResult) {
                                        float currentTime = 0.0f;
                                        for (const auto &an : alignedNotes) {
                                            Game::GameNote gn;
                                            bool isRest = an.name.empty() || an.name == "rest";
                                            gn.voiced = !isRest;
                                            if (!isRest) {
                                                auto parsed = dstools::parseNoteName(QString::fromStdString(an.name));
                                                gn.pitch = parsed.valid ? parsed.midiNumber : 60.0f;
                                            } else {
                                                gn.pitch = 0.0f;
                                            }
                                            gn.onset = currentTime;
                                            gn.duration = static_cast<float>(an.duration);
                                            notes.push_back(gn);
                                            currentTime += static_cast<float>(an.duration);
                                        }
                                    }
                                } else {
                                    midiResult = game->getNotes(audioPath.toStdWString(), notes, nullptr);
                                }

                                if (midiResult) {
                                    QMetaObject::invokeMethod(
                                        guard.data(),
                                        [guard, sliceId, notes = std::move(notes)]() {
                                            if (guard)
                                                guard->applyMidiResult(sliceId, notes);
                                        },
                                        Qt::QueuedConnection);
                                } else {
                                    DSFW_LOG_ERROR("infer", (std::string("Batch MIDI transcription failed: ") +
                                                             sliceId.toStdString() + " - " + midiResult.error())
                                                                .c_str());
                                    QMetaObject::invokeMethod(
                                        dlg,
                                        [dlg, sliceId]() {
                                            dlg->appendLog(
                                                QStringLiteral("[ERROR] %1: MIDI transcription failed").arg(sliceId));
                                        },
                                        Qt::QueuedConnection);
                                }
                            }

                            QMetaObject::invokeMethod(
                                dlg, [dlg, sliceId]() { dlg->appendLog(QStringLiteral("[OK] %1").arg(sliceId)); },
                                Qt::QueuedConnection);
                            ++processed;
                            ++idx;
                        }
                    } catch (const std::exception &e) {
                        DSFW_LOG_ERROR("infer", ("Batch pitch/MIDI exception: " + std::string(e.what())).c_str());
                        QMetaObject::invokeMethod(
                            dlg,
                            [dlg, eMsg = std::string(e.what())]() {
                                dlg->appendLog(
                                    QStringLiteral("[ERROR] Exception: %1").arg(QString::fromStdString(eMsg)));
                            },
                            Qt::QueuedConnection);
                    }
                    QMetaObject::invokeMethod(
                        dlg, [dlg, processed, skipped, errors]() { dlg->finish(processed, skipped, errors); },
                        Qt::QueuedConnection);
                    QMetaObject::invokeMethod(
                        guard.data(),
                        [guard]() {
                            if (guard) {
                                guard->m_inferRunning = false;
                                guard->m_extractPitchAction->setEnabled(true);
                                guard->m_extractMidiAction->setEnabled(true);
                            }
                        },
                        Qt::QueuedConnection);
                });
            });

        connect(dlg, &BatchProcessDialog::cancelled, this, [guard]() {
            if (guard) {
                guard->m_inferRunning = false;
                guard->m_extractPitchAction->setEnabled(true);
                guard->m_extractMidiAction->setEnabled(true);
            }
        });

        dlg->show();
    }

    void PitchLabelerPage::updateProgress() {
        if (!source())
            return;
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
