#include "PitchLabelerPage.h"

#include "BatchProcessDialog.h"
#include "AppSettingsBackend.h"
#include "DsTextDocBridge.h"
#include "EnginePool.h"
#include "ModelConfigHelper.h"
#include "PitchLabelerPageHelpers.h"
#include "Keys.h"
#include "SliceListPanel.h"

#include <dstools/Constants.h>
#include <dstools/DsPitchDocument.h>
#include <QCheckBox>
#include <QJsonObject>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <dsfw/CommonKeys.h>
#include <dstools/ModelManager.h>
#include <dsfw/IModelProvider.h>
#include <dsfw/InferenceModelProvider.h>
#include <dsfw/Theme.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dstools/CurveTools.h>
#include <dstools/DsKeys.h>
#include <dstools/DsTextTypes.h>
#include <dstools/ExecutionProvider.h>
#include <dstools/ModelManager.h>
#include <dstools/PitchUtils.h>
#include <sndfile.hh>

struct PitchExtractionData {
    std::vector<float> f0Mhz;
    float timestep = 0.01f;
};

namespace dstools {

PitchLabelerPage::PitchLabelerPage(QWidget* parent) : EditorPageBase("PitchLabeler", parent) {
    m_editor = new pitchlabeler::PitchEditor(this);

    setupBaseLayout(m_editor);

    setupNavigationActions();

    m_extractPitchAction = new QAction(tr("Extract Pitch"), this);
    m_extractMidiAction = new QAction(tr("Extract MIDI"), this);
    connect(m_extractPitchAction, &QAction::triggered, this, &PitchLabelerPage::onExtractPitch);
    connect(m_extractMidiAction, &QAction::triggered, this, &PitchLabelerPage::onExtractMidi);

    auto* tb = m_editor->toolbar();
    tb->addSeparator();
    tb->addAction(m_extractPitchAction);
    tb->addAction(m_extractMidiAction);
    tb->addSeparator();
    tb->addAction(m_editor->saveAction());
    tb->addSeparator();
    tb->addAction(m_editor->zoomInAction());
    tb->addAction(m_editor->zoomOutAction());

    shortcutManager()->bind(m_editor->saveAction(), dstools::settings::kShortcutSave, tr("Save"), tr("File"));
    shortcutManager()->bind(m_editor->undoAction(), dstools::settings::kShortcutUndo, tr("Undo"), tr("Edit"));
    shortcutManager()->bind(m_editor->redoAction(), dstools::settings::kShortcutRedo, tr("Redo"), tr("Edit"));
    shortcutManager()->bind(m_editor->zoomInAction(), dstools::settings::kShortcutZoomIn, tr("Zoom In"), tr("View"));
    shortcutManager()->bind(m_editor->zoomOutAction(), dstools::settings::kShortcutZoomOut, tr("Zoom Out"), tr("View"));
    shortcutManager()->bind(m_editor->zoomResetAction(), dstools::settings::kShortcutZoomReset, tr("Reset Zoom"),
                            tr("View"));
    shortcutManager()->bind(m_editor->abCompareAction(), dstools::settings::kShortcutABCompare, tr("A/B Compare"),
                            tr("Tools"));
    shortcutManager()->bind(m_extractPitchAction, dstools::settings::kShortcutExtractPitch, tr("Extract Pitch"),
                            tr("Processing"));
    shortcutManager()->bind(m_extractMidiAction, dstools::settings::kShortcutExtractMidi, tr("Extract MIDI"),
                            tr("Processing"));
    shortcutManager()->bind(m_editor->playPauseAction(), dstools::settings::kShortcutPlayPause, tr("Play/Pause"),
                            tr("Playback"));
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

QString PitchLabelerPage::windowTitlePrefix() const {
    return tr("Pitch Labeling");
}

bool PitchLabelerPage::isDirty() const {
    return m_editor->undoStack() && !m_editor->undoStack()->isClean();
}

void PitchLabelerPage::onDeactivatedImpl() {
    enginePool()->invalidate(QLatin1String(dstools::keys::engines::pitchExtraction));
    enginePool()->invalidate(QLatin1String(dstools::keys::engines::midiTranscription));
    m_rmvpe = nullptr;
    m_game = nullptr;
}

void PitchLabelerPage::saveExtraSplitters() {
    if (m_editor) {
        m_editor->saveViewportResolution();
        settings().set(dstools::settings::kEditorSplitterState,
                       QString::fromLatin1(m_editor->saveSplitterState().toBase64()));
    }
}

void PitchLabelerPage::restoreExtraSplitters() {
    if (m_editor) {
        m_editor->restoreViewportResolution();
        auto state = settings().get(dstools::settings::kEditorSplitterState);
        if (!state.isEmpty())
            m_editor->restoreSplitterState(QByteArray::fromBase64(state.toUtf8()));
    }
}

BatchSliceResult PitchLabelerPage::processSlice(const QString& sliceId) {
    Q_UNUSED(sliceId)
    BatchSliceResult result;
    result.status = BatchSliceResult::Error;
    result.error = QStringLiteral("PitchLabelerPage uses manual pitch/MIDI flow, not batch pipeline");
    return result;
}

void PitchLabelerPage::onSliceSelectedImpl(const QString& sliceId) {
    m_currentFile.reset();

    if (!source()) {
        m_editor->clear();
        return;
    }

    const QString audioPath = source()->validatedAudioPath(sliceId);

    auto result = source()->loadSlice(sliceId);
    double audioDurSec = 0.0;
    if (result) {
        const auto& doc = result.value();
        TimePos totalDuration = secToUs(audioDurationSec(doc));
        auto file = DsTextDocBridge::toPitchDoc(doc, totalDuration);

        if (!audioPath.isEmpty())
            file->setFilePath(audioPath);

        m_currentFile = file;
        m_editor->loadDsPitchDocument(m_currentFile);
        audioDurSec = audioDurationSec(doc);
    } else {
        m_currentFile = std::make_shared<pitchlabeler::DsPitchDocument>();
        if (!audioPath.isEmpty())
            m_currentFile->setFilePath(audioPath);
        m_editor->loadDsPitchDocument(m_currentFile);
    }

    if (!audioPath.isEmpty())
        m_editor->loadAudio(audioPath, audioDurSec);
}

bool PitchLabelerPage::saveCurrentSlice() {
    if (currentSliceId().isEmpty() || !source() || !m_currentFile)
        return true;

    auto result = source()->loadSlice(currentSliceId());
    if (!result) {
        DSFW_LOG_ERROR(
            "pitch",
            ("Failed to load slice for save: " + currentSliceId().toStdString() + " - " + result.error()).c_str());
        return false;
    }
    DsTextDocument doc = std::move(result.value());

    auto mergeResult = DsTextDocBridge::fromPitchDoc(doc, *m_currentFile);
    if (!mergeResult) {
        DSFW_LOG_ERROR("pitch", ("fromPitchDoc validation failed: " + mergeResult.error()).c_str());
        QMessageBox::warning(this, tr("保存失败"),
                             tr("数据校验失败: %1").arg(QString::fromStdString(mergeResult.error())));
        return false;
    }

    if (!doc.meta.editedSteps.contains(QStringLiteral("pitch_review")))
        doc.meta.editedSteps.append(QStringLiteral("pitch_review"));

    auto saveResult = source()->saveSlice(currentSliceId(), doc);
    if (!saveResult) {
        QMessageBox::warning(this, tr("保存失败"), QString::fromStdString(saveResult.error()));
        return false;
    }

    QStringList modifiedLayers;
    modifiedLayers.append(QString::fromUtf8(dstools::keys::layers::pitch));
    if (!m_currentFile->notes.empty())
        modifiedLayers.append(QString::fromUtf8(dstools::keys::layers::midi));
    markLayersModified(modifiedLayers);

    for (const auto& layerName : modifiedLayers) {
        setLayerManuallyEdited(layerName, true);
        if (layerName == QString::fromUtf8(dstools::keys::layers::pitch))
            addEditedStep(QStringLiteral("pitch_review"));
        else if (layerName == QString::fromUtf8(dstools::keys::layers::midi))
            addEditedStep(QStringLiteral("midi_review"));
    }

    return true;
}

QMenuBar* PitchLabelerPage::createMenuBar(QWidget* parent) {
    auto* bar = buildStandardMenuBar(parent, m_editor->saveAction(), m_editor->undoAction(), m_editor->redoAction());

    auto* processMenu = bar->addMenu(tr("处理(&P)"));
    processMenu->addAction(m_extractPitchAction);
    processMenu->addAction(m_extractMidiAction);
    processMenu->addAction(tr("计算 ph_num (当前切片)"), this, &PitchLabelerPage::onAddPhNum);
    processMenu->addSeparator();
    processMenu->addAction(tr("批量提取音高 + MIDI..."), this, &PitchLabelerPage::onBatchExtract);

    auto* viewMenu =
        addStandardViewMenu(bar, {m_editor->zoomInAction(), m_editor->zoomOutAction(), m_editor->zoomResetAction()});

    auto* toolsMenu = bar->addMenu(tr("工具(&T)"));
    toolsMenu->addAction(m_editor->abCompareAction());

    return bar;
}

QWidget* PitchLabelerPage::createStatusBarContent(QWidget* parent) {
    auto* container = new QWidget(parent);
    auto builder = buildStandardStatusBar(container);

    auto* posLabel = builder.addLabel(QStringLiteral("00:00.000"));
    auto* zoomLabel = builder.addLabel(QStringLiteral("100%"));
    auto* noteLabel = builder.addLabel(tr("音符: 0"));

    builder.connect(m_editor, &pitchlabeler::PitchEditor::positionChanged, posLabel, [posLabel](double sec) {
        int minutes = static_cast<int>(sec) / 60;
        double seconds = sec - minutes * 60;
        posLabel->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 6, 'f', 3, QChar('0')));
    });
    builder.connect(m_editor, &pitchlabeler::PitchEditor::zoomChanged, zoomLabel,
                    [zoomLabel](int percent) { zoomLabel->setText(QString::number(percent) + "%"); });
    builder.connect(m_editor, &pitchlabeler::PitchEditor::noteCountChanged, noteLabel,
                    [noteLabel](int count) { noteLabel->setText(tr("音符: %1").arg(count)); });

    return container;
}

void PitchLabelerPage::onEngineInvalidated(const QString& taskKey) {
    enginePool()->invalidate(taskKey);
    if (taskKey == QLatin1String(dstools::keys::engines::pitchExtraction)) {
        m_rmvpe = nullptr;
        DSFW_LOG_WARN("infer", "Pitch extraction task cancelled: model invalidated");
    } else if (taskKey == QLatin1String(dstools::keys::engines::midiTranscription)) {
        m_game = nullptr;
        DSFW_LOG_WARN("infer", "MIDI transcription task cancelled: model invalidated");
    }
}

void PitchLabelerPage::onAutoInferPreloadEngines() {
    auto preload = preloadConfig();
    if (!preload.isEmpty()) {
        auto pitchPreload = preload[dstools::keys::engines::pitchExtraction].toObject();
        if (pitchPreload["enabled"].toBool(false))
            enginePool()->acquire<Rmvpe::Rmvpe>(QLatin1String(dstools::keys::engines::pitchExtraction));

        auto midiPreload = preload[dstools::keys::engines::midiTranscription].toObject();
        if (midiPreload["enabled"].toBool(false))
            enginePool()->acquire<Game::Game>(QLatin1String(dstools::keys::engines::midiTranscription));
    }
}

void PitchLabelerPage::onAutoInferProcessDirty(const QStringList& dirty) {
    bool needPitch = dirty.contains(QString::fromUtf8(dstools::keys::layers::pitch));
    bool needMidi = dirty.contains(QString::fromUtf8(dstools::keys::layers::midi));

    if (needPitch) {
        if (source()->isLayerManuallyEdited(currentSliceId(), QString::fromUtf8(dstools::keys::layers::pitch))) {
            dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info, tr("已跳过手动编辑的F0层"),
                                                   4000);
            source()->clearDirtyLayers(currentSliceId(), {QString::fromUtf8(dstools::keys::layers::pitch)});
        } else {
            source()->clearDirtyLayers(currentSliceId(), {QString::fromUtf8(dstools::keys::layers::pitch)});
            m_rmvpe = enginePool()->acquire<Rmvpe::Rmvpe>(QLatin1String(dstools::keys::engines::pitchExtraction));
            if (m_rmvpe && !isBatchRunning()) {
                dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info, tr("自动提取音高..."),
                                                       3000);
                runPitchExtraction(currentSliceId());
            } else {
                dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Warning,
                                                       tr("F0 数据缺失，请手动提取"), 3000);
            }
        }
    }

    if (needMidi) {
        if (source()->isLayerManuallyEdited(currentSliceId(), QString::fromUtf8(dstools::keys::layers::midi))) {
            dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info, tr("已跳过手动编辑的MIDI层"),
                                                   4000);
            source()->clearDirtyLayers(currentSliceId(), {QString::fromUtf8(dstools::keys::layers::midi)});
        } else {
            source()->clearDirtyLayers(currentSliceId(), {QString::fromUtf8(dstools::keys::layers::midi)});
            m_game = enginePool()->acquire<Game::Game>(QLatin1String(dstools::keys::engines::midiTranscription));
            if (m_game && !isBatchRunning()) {
                dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info, tr("自动转写 MIDI..."),
                                                       3000);
                runMidiTranscription(currentSliceId(), nullptr);
            } else {
                dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Warning,
                                                       tr("MIDI 数据缺失，请手动转写"), 3000);
            }
        }
    }

    bool needPhNum = dirty.contains(QString::fromUtf8(dstools::keys::layers::phNum));
    if (!needPhNum) {
        auto docResult = source()->loadSlice(currentSliceId());
        if (docResult) {
            bool hasPhoneme = false;
            bool hasPhNum = false;
            for (const auto& layer : docResult.value().layers) {
                if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive) &&
                    !layer.boundaries.empty())
                    hasPhoneme = true;
                if (layer.name == QString::fromUtf8(dstools::keys::layers::phNum) && !layer.boundaries.empty())
                    hasPhNum = true;
            }
            if (hasPhoneme && !hasPhNum)
                needPhNum = true;
        }
    }

    if (needPhNum) {
        source()->clearDirtyLayers(currentSliceId(), {QString::fromUtf8(dstools::keys::layers::phNum)});
        loadPhNumCalculator();
        if (m_phNumCalc.isLoaded()) {
            dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info, tr("自动计算发音数..."), 3000);
            runAddPhNum(currentSliceId());
        }
    }
}

void PitchLabelerPage::onExtractPitch() {
    if (currentSliceId().isEmpty()) {
        QMessageBox::information(this, tr("提取音高"), tr("请先选择一个切片。"));
        return;
    }

    auto engineKey = QLatin1String(dstools::keys::engines::pitchExtraction);
    m_rmvpe = enginePool()->acquire<Rmvpe::Rmvpe>(engineKey);
    if (!enginePool()->isOpen<Rmvpe::Rmvpe>(engineKey)) {
        QMessageBox::warning(this, tr("提取音高"), tr("RMVPE 模型未加载。请在设置中配置模型路径。"));
        return;
    }

    if (isBatchRunning()) {
        QMessageBox::information(this, tr("提取音高"), tr("推理正在运行中，请稍候。"));
        return;
    }

    runPitchExtraction(currentSliceId());
}

bool PitchLabelerPage::resolveAlignInputWithPhNum(Game::AlignInput& alignInput) {
    auto tryLoadPhNumFromDoc = [&]() -> bool {
        alignInput.phNum.clear();
        auto loadResult = source()->loadSlice(currentSliceId());
        if (loadResult) {
            for (const auto& layer : loadResult.value().layers) {
                if (layer.name == QString::fromUtf8(dstools::keys::layers::phNum) && !layer.boundaries.empty()) {
                    for (const auto& b : layer.boundaries) {
                        bool ok = false;
                        int val = b.text.toInt(&ok);
                        alignInput.phNum.push_back(ok ? val : 0);
                    }
                    return true;
                }
            }
        }
        return false;
    };

    auto validatePhNum = [](const std::vector<int>& phNum, size_t phSeqSize) -> bool {
        long long sum = 0;
        for (int v : phNum)
            sum += v;
        return static_cast<size_t>(sum) == phSeqSize;
    };

    bool hasPhNum = tryLoadPhNumFromDoc();

    if (hasPhNum && validatePhNum(alignInput.phNum, alignInput.phSeq.size())) {
        return true;
    }

    if (hasPhNum) {
        alignInput.phNum.clear();
        hasPhNum = false;
    }

    bool dictConfigured = false;
    if (settingsBackend()) {
        auto cfg = settingsBackend()->load()["phNumConfig"].toObject();
        auto langs = cfg["languages"].toObject();
        for (auto it = langs.begin(); it != langs.end(); ++it) {
            auto langCfg = it.value().toObject();
            if (!langCfg["dictPath"].toString().isEmpty()) {
                dictConfigured = true;
                break;
            }
        }
    }

    if (!dictConfigured) {
        QMessageBox::warning(this, tr("缺少 ph_num"),
                             tr("当前切片缺少 ph_num 数据，且未配置 ph_num 词典。\n"
                                "请先在 设置 → ph_num 中配置词典路径。"));
        return false;
    }

    loadPhNumCalculator();
    runAddPhNum(currentSliceId());
    tryLoadPhNumFromDoc();

    if (!validatePhNum(alignInput.phNum, alignInput.phSeq.size())) {
        QMessageBox::warning(this, tr("缺少 ph_num"),
                             tr("自动计算的 ph_num 与音素数量不匹配，请检查 ph_num 词典配置。"));
        return false;
    }

    return true;
}

void PitchLabelerPage::onExtractMidi() {
    if (currentSliceId().isEmpty()) {
        QMessageBox::information(this, tr("提取 MIDI"), tr("请先选择一个切片。"));
        return;
    }

    auto midiEngineKey = QLatin1String(dstools::keys::engines::midiTranscription);
    m_game = enginePool()->acquire<Game::Game>(midiEngineKey);
    if (!enginePool()->isOpen<Game::Game>(midiEngineKey)) {
        QMessageBox::warning(this, tr("提取 MIDI"), tr("GAME 模型未加载。请在设置中配置模型路径。"));
        return;
    }

    if (isBatchRunning()) {
        QMessageBox::information(this, tr("提取 MIDI"), tr("推理正在运行中，请稍候。"));
        return;
    }

    bool useAlignMode = false;
    Game::AlignInput alignInput;
    alignInput.wavPath = source()->validatedAudioPath(currentSliceId()).toStdWString();
    useAlignMode = buildAlignInput(alignInput);

    if (useAlignMode) {
        if (!resolveAlignInputWithPhNum(alignInput))
            return;
    }

    runMidiTranscription(currentSliceId(), useAlignMode ? &alignInput : nullptr);
}

bool PitchLabelerPage::buildAlignInput(Game::AlignInput& outAlignInput) const {
    auto loadResult = source()->loadSlice(currentSliceId());
    if (!loadResult)
        return false;
    const auto& doc = loadResult.value();
    for (const auto& layer : doc.layers) {
        if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive) &&
            !layer.boundaries.empty()) {
            const auto& bnd = layer.boundaries;
            for (size_t i = 0; i < bnd.size(); ++i) {
                if (!bnd[i].text.isEmpty()) {
                    outAlignInput.phSeq.push_back(bnd[i].text.toStdString());
                    TimePos dur = (i + 1 < bnd.size()) ? bnd[i + 1].pos - bnd[i].pos
                                                       : secToUs(audioDurationSec(doc)) - bnd[i].pos;
                    outAlignInput.phDur.push_back(static_cast<float>(usToSec(dur)));
                }
            }
            return !outAlignInput.phSeq.empty();
        }
    }
    return false;
}

void PitchLabelerPage::onAddPhNum() {
    if (currentSliceId().isEmpty()) {
        QMessageBox::information(this, tr("计算 ph_num"), tr("请先选择一个切片。"));
        return;
    }
    runAddPhNum(currentSliceId());
}

void PitchLabelerPage::runPitchExtraction(const QString& sliceId) {
    if (!source())
        return;

    QString audioPath = source()->validatedAudioPath(sliceId);
    if (audioPath.isEmpty()) {
        if (source()->audioPath(sliceId).isEmpty())
            QMessageBox::warning(this, tr("提取音高"), tr("当前切片没有音频文件。"));
        return;
    }

    setBatchRunning(true);
    m_extractPitchAction->setEnabled(false);
    m_extractMidiAction->setEnabled(false);
    DSFW_LOG_INFO("infer", ("Pitch extraction started: " + sliceId.toStdString()).c_str());
    auto* rmvpe = m_rmvpe;

    runAsyncTask<PitchExtractionData>(
        QLatin1String(dstools::keys::engines::pitchExtraction), sliceId,
        [rmvpe, audioPath](const std::shared_ptr<std::atomic<bool>>&) -> Result<PitchExtractionData> {
            if (!rmvpe)
                return Err<PitchExtractionData>("RMVPE engine is null");
            std::vector<Rmvpe::RmvpeRes> results;
            auto result = rmvpe->get_f0(audioPath.toStdWString(), 0.01f, results, nullptr);
            if (result && !results.empty()) {
                const auto& res = results[0];
                std::vector<float> mergedF0(res.f0.size());
                for (size_t i = 0; i < res.f0.size(); ++i) {
                    mergedF0[i] = res.f0[i] * 1000.0f;
                }
                PitchExtractionData data;
#ifdef _WIN32
                auto pathStr = audioPath.toStdWString();
#else
                auto pathStr = audioPath.toStdString();
#endif
                SndfileHandle sf(pathStr.c_str());
                if (sf && sf.samplerate() > 0) {
                    const int sampleRate = sf.samplerate();
                    const int64_t audioFrames = sf.frames();
                    const TimePos dstTimestepUs = hopSizeToTimestep(constants::kDefaultHopSize, sampleRate);
                    const int alignLength = expectedFrameCount(audioFrames, constants::kDefaultHopSize);
                    data.f0Mhz = resampleCurve(mergedF0, secToUs(0.01), dstTimestepUs, alignLength);
                    data.timestep = static_cast<float>(usToSec(dstTimestepUs));
                } else {
                    data.f0Mhz = std::move(mergedF0);
                    data.timestep = 0.01f;
                }
                return data;
            }
            return Err<PitchExtractionData>(result.error());
        },
        [this](const QString& sliceId, const Result<PitchExtractionData>& result) {
            setBatchRunning(false);
            m_extractPitchAction->setEnabled(true);
            m_extractMidiAction->setEnabled(true);
            if (result) {
                applyPitchResult(sliceId, result.value().f0Mhz, result.value().timestep);
                DSFW_LOG_INFO("infer", ("Pitch extraction completed: " + sliceId.toStdString() + " - " +
                                        std::to_string(result.value().f0Mhz.size()) + " frames")
                                           .c_str());
                dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info, tr("音高提取完成"), 3000);
            } else {
                QMessageBox::warning(this, tr("提取音高"),
                                     tr("音高提取失败") + QString::fromStdString(": " + result.error()));
            }
        });
}

void PitchLabelerPage::runMidiTranscription(const QString& sliceId, const Game::AlignInput* alignInput) {
    if (!source())
        return;

    QString audioPath = source()->validatedAudioPath(sliceId);
    if (audioPath.isEmpty()) {
        if (source()->audioPath(sliceId).isEmpty())
            QMessageBox::warning(this, tr("提取 MIDI"), tr("当前切片没有音频文件。"));
        return;
    }

    setBatchRunning(true);
    m_extractPitchAction->setEnabled(false);
    m_extractMidiAction->setEnabled(false);
    DSFW_LOG_INFO("infer",
                  ("MIDI transcription started: " + sliceId.toStdString() + (alignInput ? " (align)" : "")).c_str());
    auto* game = m_game;
    bool useAlign = (alignInput != nullptr);
    auto capturedInput = useAlign ? std::make_shared<Game::AlignInput>(*alignInput) : nullptr;

    Game::AlignOptions options;
    if (useAlign && capturedInput) {
        auto* sb = settingsBackend();
        if (sb) {
            auto cfg = sb->load()["pitchConfig"].toObject();
            if (cfg.contains("uvVocab") && cfg["uvVocab"].isString()) {
                QString uvStr = cfg["uvVocab"].toString();
                std::set<std::string> uvVocab;
                const auto parts = uvStr.split(QLatin1Char(','), Qt::SkipEmptyParts);
                for (const auto& p : parts) {
                    std::string trimmed = p.trimmed().toStdString();
                    if (!trimmed.empty())
                        uvVocab.insert(trimmed);
                }
                if (!uvVocab.empty())
                    options.uvVocab = uvVocab;
            }
            if (cfg.contains("uvWordCond") && cfg["uvWordCond"].isDouble()) {
                int cond = cfg["uvWordCond"].toInt();
                if (cond == 1 || cond == 2)
                    options.uvWordCond = Game::UvWordCond::All;
            }
        }
    }

    runAsyncTask<std::vector<Game::GameNote>>(
        QLatin1String(dstools::keys::engines::midiTranscription), sliceId,
        [game, audioPath, useAlign, capturedInput, options = std::move(options)](
            const std::shared_ptr<std::atomic<bool>>&) -> Result<std::vector<Game::GameNote>> {
            if (!game)
                return Err<std::vector<Game::GameNote>>("Game engine is null");
            std::vector<Game::GameNote> notes;

            if (useAlign && capturedInput) {
                capturedInput->wavPath = std::filesystem::path(audioPath.toStdWString());
                std::vector<Game::AlignedNote> alignedNotes;
                auto result = game->align(*capturedInput, options, alignedNotes);
                if (result) {
                    float currentTime = 0.0f;
                    for (const auto& an : alignedNotes) {
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
                    return notes;
                }
                return Err<std::vector<Game::GameNote>>(result.error());
            } else {
                auto result = game->getNotes(audioPath.toStdWString(), notes, nullptr);
                if (result)
                    return notes;
                return Err<std::vector<Game::GameNote>>(result.error());
            }
        },
        [this](const QString& sliceId, const Result<std::vector<Game::GameNote>>& result) {
            setBatchRunning(false);
            m_extractPitchAction->setEnabled(true);
            m_extractMidiAction->setEnabled(true);
            if (result) {
                applyMidiResult(sliceId, result.value());
                DSFW_LOG_INFO("infer", ("MIDI transcription completed: " + sliceId.toStdString() + " - " +
                                        std::to_string(result.value().size()) + " notes")
                                           .c_str());
                dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info, tr("MIDI 转录完成"), 3000);
            } else {
                QMessageBox::warning(this, tr("提取 MIDI"),
                                     tr("MIDI 转录失败") + QString::fromStdString(": " + result.error()));
            }
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
            (void)m_phNumCalc.loadDictionary(dictPath);
            break;
        }

        if (!vowelsPath.isEmpty() || !consonantsPath.isEmpty()) {
            if (!vowelsPath.isEmpty())
                (void)m_phNumCalc.loadVowelsFromFile(vowelsPath);
            if (!consonantsPath.isEmpty())
                (void)m_phNumCalc.loadConsonantsFromFile(consonantsPath);
            break;
        }
    }

    QString specialWordsStr = phNumCfg["specialWords"].toString().trimmed();
    if (!specialWordsStr.isEmpty()) {
        QSet<QString> specialWords;
        const auto parts = specialWordsStr.split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const auto& p : parts) {
            QString trimmed = p.trimmed();
            if (!trimmed.isEmpty())
                specialWords.insert(trimmed);
        }
        m_phNumCalc.setSpecialWords(specialWords);
    }
}

void PitchLabelerPage::runAddPhNum(const QString& sliceId) {
    if (!source())
        return;

    loadPhNumCalculator();

    auto result = source()->loadSlice(sliceId);
    if (!result) {
        QMessageBox::warning(this, tr("计算 ph_num"), tr("无法加载切片数据。"));
        return;
    }

    DsTextDocument doc = std::move(result.value());

    QStringList phSeq;
    for (const auto& layer : doc.layers) {
        if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive)) {
            for (const auto& b : layer.boundaries) {
                if (!b.text.isEmpty())
                    phSeq << b.text;
            }
        }
    }

    if (phSeq.isEmpty()) {
        QMessageBox::information(this, tr("计算 ph_num"), tr("当前切片没有音素数据，请先进行强制对齐。"));
        return;
    }

    auto calcResult = m_phNumCalc.calculate(phSeq.join(' '));
    if (calcResult.ok()) {
        const QString& phNumStr = calcResult.value();
        IntervalLayer* phNumLayer = nullptr;
        for (auto& layer : doc.layers) {
            if (layer.name == QString::fromUtf8(dstools::keys::layers::phNum)) {
                phNumLayer = &layer;
                break;
            }
        }
        if (!phNumLayer) {
            doc.layers.push_back({});
            phNumLayer = &doc.layers.back();
            phNumLayer->name = QString::fromUtf8(dstools::keys::layers::phNum);
            phNumLayer->type = QStringLiteral("attribute");
        }

        phNumLayer->boundaries.clear();
        const auto parts = phNumStr.split(QChar(' '), Qt::SkipEmptyParts);
        int id = 1;
        for (const auto& part : parts) {
            Boundary b;
            b.id = id++;
            b.text = part;
            phNumLayer->boundaries.push_back(std::move(b));
        }

        if (auto res = source()->saveSlice(sliceId, doc); !res.ok())
            DSFW_LOG_ERROR(
                "pitch", ("Failed to save slice after ph_num: " + sliceId.toStdString() + " - " + res.error()).c_str());
    } else {
        QMessageBox::warning(this, tr("计算 ph_num"),
                             tr("ph_num 计算失败: %1").arg(QString::fromStdString(calcResult.error())));
    }
}

void PitchLabelerPage::applyPitchResult(const QString& sliceId, const std::vector<float>& f0, float timestep) {
    pitchLabelerApplyPitchResult(source(), m_currentFile, m_editor, sliceId, f0, timestep);
}

void PitchLabelerPage::applyMidiResult(const QString& sliceId, const std::vector<Game::GameNote>& notes) {
    pitchLabelerApplyMidiResult(source(), m_currentFile, m_editor, sliceId, notes);
}

void PitchLabelerPage::onBatchExtract() {
    pitchLabelerRunBatchExtract(this);
}

void PitchLabelerPage::updateProgress() {
    if (!source())
        return;
    const auto ids = source()->sliceIds();
    int total = ids.size();
    int completed = 0;
    for (const auto& id : ids) {
        auto result = source()->loadSlice(id);
        if (result) {
            bool hasPitch = false;
            for (const auto& curve : result.value().curves) {
                if (curve.name == QString::fromUtf8(dstools::keys::layers::pitch) && !curve.values.empty()) {
                    hasPitch = true;
                    break;
                }
            }
            bool hasMidi = false;
            for (const auto& layer : result.value().layers) {
                if (layer.name == QString::fromUtf8(dstools::keys::layers::midi) && !layer.boundaries.empty()) {
                    hasMidi = true;
                    break;
                }
            }
            if (hasPitch && hasMidi)
                ++completed;
        }
    }
    sliceListPanel()->setProgress(completed, total);
}

EngineAliveToken& PitchLabelerPage::aliveToken(const QString& key) {
    return EditorPageBase::aliveToken(key);
}

bool PitchLabelerPage::isBatchRunning() const {
    return EditorPageBase::isBatchRunning();
}

void PitchLabelerPage::setBatchRunning(bool running) {
    EditorPageBase::setBatchRunning(running);
}

} // namespace dstools
