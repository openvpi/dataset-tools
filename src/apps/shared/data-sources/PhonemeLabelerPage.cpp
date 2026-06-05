#include "PhonemeLabelerPage.h"

#include "AudioVisualizerContainer.h"
#include "BatchProcessDialog.h"
#include "DsTextDocBridge.h"
#include <dstools/DsPitchDocument.h>
#include "AppSettingsBackend.h"
#include "EnginePool.h"
#include "ModelConfigHelper.h"
#include "MoeCurveProcessor.h"
#include "Keys.h"
#include "SliceListPanel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <dstools/ModelManager.h>
#include <dsfw/IModelProvider.h>
#include <dstools/InferenceModelProvider.h>
#include <dsfw/Theme.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dstools/DsKeys.h>
#include <dstools/DsTextTypes.h>
#include <dstools/ExecutionProvider.h>
#include <dstools/ModelManager.h>
#include <dstools/PitchUtils.h>
#include <filesystem>
#include <InferBridge.h>

namespace dstools {

// ---------------------------------------------------------------------------
// Build grapheme + phoneme layers with BindingGroups from HFA WordList.
//
// The FA system accepts grapheme input (from original grapheme layer) and
// produces Word→Phone mapping. FA output is stored in a grapheme layer
// (word-level boundaries from FA alignment) and a phoneme layer (phone-level).
// Where a word start coincides with a phone start, those two boundary IDs
// form a BindingGroup (they must move together). The original grapheme layer
// (G2P output) is preserved as reference for re-running FA.
//
// SP/AP words are included in grapheme so that grapheme and phoneme
// layers have aligned dependency relationships. readFaInput() reads from the
// original grapheme layer and filters out SP/AP text.
// ---------------------------------------------------------------------------
struct FaLayerResult {
    IntervalLayer graphemeLayer;
    IntervalLayer phonemeLayer;
    std::vector<BindingGroup> groups;
    std::vector<LayerDependency> dependencies;
};

static FaLayerResult buildFaLayers(const HFA::WordList& words) {
    FaLayerResult r;
    r.graphemeLayer.name = QString::fromUtf8(dstools::keys::layers::grapheme);
    r.graphemeLayer.type = QStringLiteral("interval");
    r.phonemeLayer.name = QString::fromUtf8(dstools::keys::layers::phoneme);
    r.phonemeLayer.type = QStringLiteral("interval");

    struct WordIds {
        int graphemeStartId = 0;
        std::vector<int> phoneBoundaryIds;
    };
    std::vector<WordIds> wordIdList;
    int nextId = 1;

    for (const auto& word : words) {
        if (word.phones.empty())
            continue;

        WordIds ids;

        Boundary graphemeStart;
        graphemeStart.id = nextId++;
        graphemeStart.pos = secToUs(std::max(0.0f, word.start));
        graphemeStart.text = QString::fromStdString(word.text);
        ids.graphemeStartId = graphemeStart.id;
        r.graphemeLayer.boundaries.push_back(std::move(graphemeStart));

        for (size_t pi = 0; pi < word.phones.size(); ++pi) {
            const auto& phone = word.phones[pi];

            Boundary phoneB;
            phoneB.id = nextId++;
            phoneB.pos = secToUs(std::max(0.0f, phone.start));
            phoneB.text = QString::fromStdString(phone.text);

            ids.phoneBoundaryIds.push_back(phoneB.id);

            if (pi == 0)
                r.groups.push_back({ids.graphemeStartId, phoneB.id});

            r.phonemeLayer.boundaries.push_back(std::move(phoneB));
        }

        wordIdList.push_back(std::move(ids));
    }

    for (size_t wi = 0; wi < wordIdList.size(); ++wi) {
        const auto& ids = wordIdList[wi];

        LayerDependency dep;
        dep.parentLayerName = QString::fromUtf8(dstools::keys::layers::grapheme);
        dep.childLayerName = QString::fromUtf8(dstools::keys::layers::phoneme);
        dep.parentStartBoundaryId = ids.graphemeStartId;
        dep.parentEndBoundaryId =
            (wi + 1 < wordIdList.size()) ? wordIdList[wi + 1].graphemeStartId : ids.graphemeStartId;

        if (!ids.phoneBoundaryIds.empty()) {
            dep.childStartBoundaryId = ids.phoneBoundaryIds.front();
            dep.childEndBoundaryId = ids.phoneBoundaryIds.back();
            dep.childBoundaryIds = ids.phoneBoundaryIds;
        }

        r.dependencies.push_back(std::move(dep));
    }

    return r;
}

PhonemeLabelerPage::PhonemeLabelerPage(QWidget* parent) : EditorPageBase("PhonemeLabeler", parent) {
    m_editor = new phonemelabeler::PhonemeEditor(this);

    setupBaseLayout(m_editor);
    setupNavigationActions();

    m_faAction = new QAction(tr("Force Align Current Slice"), this);
    connect(m_faAction, &QAction::triggered, this, &PhonemeLabelerPage::onRunFA);

    // Add FA button to the editor toolbar
    m_editor->toolbar()->addAction(m_faAction);

    m_actExtractF0 = new QAction(tr("Extract F0"), this);
    m_actExtractF0->setToolTip(tr("使用 RMVPE 提取 F0 到钢琴卷帘"));
    connect(m_actExtractF0, &QAction::triggered, this, &PhonemeLabelerPage::onExtractF0);
    m_editor->toolbar()->addAction(m_actExtractF0);

    m_actExtractMidi = new QAction(tr("Extract MIDI"), this);
    m_actExtractMidi->setToolTip(tr("使用 GAME 提取 MIDI 到钢琴卷帘"));
    connect(m_actExtractMidi, &QAction::triggered, this, &PhonemeLabelerPage::onExtractMidi);
    m_editor->toolbar()->addAction(m_actExtractMidi);

    shortcutManager()->bind(m_editor->saveAction(), dstools::settings::kShortcutSave, tr("Save"), tr("File"));
    shortcutManager()->bind(m_editor->undoAction(), dstools::settings::kShortcutUndo, tr("Undo"), tr("Edit"));
    shortcutManager()->bind(m_editor->redoAction(), dstools::settings::kShortcutRedo, tr("Redo"), tr("Edit"));
    shortcutManager()->bind(m_faAction, dstools::settings::kShortcutFA, tr("Force Align"), tr("Processing"));
    shortcutManager()->bind(m_editor->playPauseAction(), dstools::settings::kShortcutPlayPause, tr("Play/Pause"),
                            tr("Playback"));
    shortcutManager()->applyAll();
    shortcutManager()->updateTooltips();
    shortcutManager()->setEnabled(false);

    connect(m_editor->saveAction(), &QAction::triggered, this, [this]() {
        saveCurrentSlice();
        updateDirtyIndicator();
    });
    connect(m_editor->document(), &phonemelabeler::TextGridDocument::modifiedChanged, this,
            [this]() { updateDirtyIndicator(); });

    m_moeProcessor = new MoeCurveProcessor(this);
    connect(m_moeProcessor, &MoeCurveProcessor::curveReady, this, [this](const MouthCurve& curve) {
        if (m_editor && m_editor->mouthCurveChart()) {
            m_editor->container()->setChartVisible(QStringLiteral("mouthCurve"), true);
            m_editor->mouthCurveChart()->setData(curve);
        }
    });
    connect(m_moeProcessor, &MoeCurveProcessor::errorOccurred, this, [](const QString& error) {
        DSFW_LOG_WARN("moe", ("MOE inference error: " + error.toStdString()).c_str());
    });
}

PhonemeLabelerPage::~PhonemeLabelerPage() = default;

// ── EditorPageBase hooks ──────────────────────────────────────────────────────

QString PhonemeLabelerPage::windowTitlePrefix() const {
    return tr("Phoneme Labeling");
}

bool PhonemeLabelerPage::isDirty() const {
    return m_editor->document() && m_editor->document()->isModified();
}

bool PhonemeLabelerPage::saveCurrentSlice() {
    if (currentSliceId().isEmpty() || !source())
        return true;

    if (!m_editor->document() || !m_editor->document()->isModified())
        return true;

    auto* doc = m_editor->document();

    auto result = source()->loadSlice(currentSliceId());
    if (!result) {
        DSFW_LOG_ERROR(
            "phoneme",
            ("Failed to load slice for save: " + currentSliceId().toStdString() + " - " + result.error()).c_str());
        return false;
    }
    DsTextDocument dstext = std::move(result.value());

    auto editorLayers = doc->toDsText();

    QStringList modifiedLayers;
    for (const auto& editorLayer : editorLayers) {
        modifiedLayers.append(editorLayer.name);
    }
    DsTextDocBridge::mergeIntervalLayers(dstext, editorLayers);

    dstext.groups = doc->bindingGroups();

    auto saveResult = source()->saveSlice(currentSliceId(), dstext);
    if (!saveResult) {
        QMessageBox::warning(this, tr("Save Failed"), QString::fromStdString(saveResult.error()));
        return false;
    }

    markLayersModified(modifiedLayers);
    for (const auto& layerName : modifiedLayers) {
        setLayerManuallyEdited(layerName, true);
        if (layerName == QString::fromUtf8(dstools::keys::layers::phoneme))
            addEditedStep(QStringLiteral("phoneme_review"));
    }
    return true;
}

void PhonemeLabelerPage::onSliceSelectedImpl(const QString& sliceId) {
    aliveToken(QStringLiteral("moe_curve")).invalidate();

    const QString audioPath = source()->validatedAudioPath(sliceId);

    auto result = source()->loadSlice(sliceId);
    if (result && !result.value().layers.empty()) {
        const auto& doc = result.value();

        QList<IntervalLayer> layers;
        for (const auto& layer : doc.layers) {
            if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme) ||
                layer.name == QString::fromUtf8(dstools::keys::layers::phoneme))
                layers.append(layer);
        }

        TimePos duration = 0;
        double durSec = audioDurationSec(doc);
        if (durSec > 0.0) {
            duration = secToUs(durSec);
        } else {
            double audioDur = m_editor->viewport()->totalDuration();
            if (audioDur > 0.0)
                duration = secToUs(audioDur);
        }

        if (duration > 0)
            m_editor->document()->loadFromDsText(layers, duration);
        else
            m_editor->document()->loadFromDsText(layers, secToUs(m_editor->viewport()->totalDuration()));

        if (!audioPath.isEmpty())
            m_editor->loadAudio(audioPath);

        if (!doc.groups.empty())
            m_editor->document()->setBindingGroups(doc.groups);
        else if (layers.size() > 1)
            m_editor->document()->autoDetectBindingGroups();

        for (int i = 0; i < static_cast<int>(layers.size()); ++i) {
            if (layers[i].name == QString::fromUtf8(dstools::keys::layers::phoneme)) {
                m_editor->document()->setTierReadOnly(i, true);
                break;
            }
        }
    } else {
        double durSec = result ? audioDurationSec(result.value()) : 0.0;
        TimePos duration = 0;
        if (durSec > 0.0) {
            duration = secToUs(durSec);
        } else {
            double audioDur = m_editor->viewport()->totalDuration();
            if (audioDur > 0.0)
                duration = secToUs(audioDur);
        }
        if (duration > 0)
            m_editor->document()->loadFromDsText({}, duration);

        if (!audioPath.isEmpty())
            m_editor->loadAudio(audioPath);
    }

    double docDuration = usToSec(m_editor->document()->totalDuration());
    if (docDuration > 0.0)
        m_editor->viewport()->setTotalDuration(docDuration);

    if (m_moeProcessor && settingsBackend() && !audioPath.isEmpty()) {
        auto cfg = readModelConfig(settingsBackend(), QStringLiteral("moe_curve"));
        if (!cfg.modelPath.isEmpty()) {
            auto& token = aliveToken(QStringLiteral("moe_curve"));
            if (!token.isValid())
                token.create();
            m_moeProcessor->setModelPath(std::filesystem::path(cfg.modelPath.toStdWString()));
            m_moeProcessor->runInference(std::filesystem::path(audioPath.toStdWString()), token.token());
        }
    }
}

void PhonemeLabelerPage::onDeactivatedImpl() {
    enginePool()->invalidate(QLatin1String(dstools::keys::engines::phonemeAlignment));
    aliveToken(QStringLiteral("moe_curve")).invalidate();
    bool hadFA = m_hfa != nullptr || isBatchRunning();
    m_hfa.reset();
    if (m_editor && m_editor->playWidget())
        m_editor->playWidget()->setPlaying(false);
    setBatchRunning(false);
    if (hadFA) {
        DSFW_LOG_WARN("fa", "FA task cancelled: page deactivated");
    } else {
        DSFW_LOG_INFO("fa", "FA page deactivated");
    }
}

BatchSliceResult PhonemeLabelerPage::processSlice(const QString& sliceId) {
    Q_UNUSED(sliceId)
    BatchSliceResult result;
    result.status = BatchSliceResult::Error;
    result.error = QStringLiteral("PhonemeLabelerPage uses manual FA flow, not batch pipeline");
    return result;
}

void PhonemeLabelerPage::restoreExtraSplitters() {
    if (m_editor) {
        m_editor->setViewportResolutionKey(QStringLiteral("Viewport/phoneme/resolution"));
        auto state = settings().get(dstools::settings::kEditorSplitterState);
        if (!state.isEmpty())
            m_editor->restoreSplitterState(QByteArray::fromBase64(state.toUtf8()));
        m_editor->restoreChartVisibility();
        m_editor->restoreViewportResolution();
    }
}

void PhonemeLabelerPage::saveExtraSplitters() {
    settings().set(dstools::settings::kEditorSplitterState,
                   QString::fromLatin1(m_editor->saveSplitterState().toBase64()));
    m_editor->saveChartVisibility();
    m_editor->saveViewportResolution();
}

void PhonemeLabelerPage::onAutoInferPreloadEngines() {
    auto preload = preloadConfig();
    if (!preload.isEmpty()) {
        auto faPreload = preload[dstools::keys::engines::phonemeAlignment].toObject();
        if (faPreload["enabled"].toBool(false))
            enginePool()->acquire<HFA::HFA>(QLatin1String(dstools::keys::engines::phonemeAlignment));
    }
}

void PhonemeLabelerPage::onAutoInferProcessDirty(const QStringList& dirty) {
    bool needAutoFA = dirty.contains(QString::fromUtf8(dstools::keys::layers::phoneme));

    if (!needAutoFA && !currentSliceId().isEmpty()) {
        auto result = source()->loadSlice(currentSliceId());
        if (result) {
            bool hasGrapheme = false;
            bool hasPhoneme = false;
            for (const auto& layer : result.value().layers) {
                if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme) && !layer.boundaries.empty())
                    hasGrapheme = true;
                if (layer.name == QString::fromUtf8(dstools::keys::layers::phoneme) && !layer.boundaries.empty())
                    hasPhoneme = true;
            }
            if (hasGrapheme && !hasPhoneme)
                needAutoFA = true;
        }
    }

    if (needAutoFA) {
        if (source()->isLayerManuallyEdited(currentSliceId(), QString::fromUtf8(dstools::keys::layers::phoneme))) {
            dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info,
                                                   tr("已跳过手动编辑的音素层，请手动重新对齐"), 4000);
        } else {
            source()->clearDirtyLayers(currentSliceId(), {QString::fromUtf8(dstools::keys::layers::phoneme)});

            const QString sliceId = currentSliceId();
            QString audioPath = source()->validatedAudioPath(sliceId);
            if (audioPath.isEmpty()) {
                return;
            }
            auto loadResult = source()->loadSlice(sliceId);
            if (!loadResult) {
                return;
            }
            bool hasGraphemeForFA = false;
            for (const auto& layer : loadResult.value().layers) {
                if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme) && !layer.boundaries.empty()) {
                    hasGraphemeForFA = true;
                    break;
                }
            }
            if (!hasGraphemeForFA) {
                return;
            }

            QPointer<PhonemeLabelerPage> guard(this);
            QTimer::singleShot(500, this, [this, guard]() {
                if (!guard || !guard->isVisible())
                    return;
                m_hfa = enginePool()->acquire<HFA::HFA>(QLatin1String(dstools::keys::engines::phonemeAlignment));
                if (m_hfa && !isBatchRunning()) {
                    dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info,
                                                           tr("Auto force-aligning..."), 3000);
                    runFaForSlice(currentSliceId());
                } else {
                    dsfw::widgets::ToastNotification::show(
                        this, dsfw::widgets::ToastType::Warning,
                        tr("Phoneme layer outdated, please run force align manually"), 3000);
                }
            });
        }
    }
}

// ── IPageActions ──────────────────────────────────────────────────────────────

QMenuBar* PhonemeLabelerPage::createMenuBar(QWidget* parent) {
    auto* bar = buildStandardMenuBar(parent, m_editor->saveAction(), m_editor->undoAction(), m_editor->redoAction());

    auto* processMenu = bar->addMenu(tr("&Processing"));
    processMenu->addAction(m_faAction);
    processMenu->addSeparator();
    processMenu->addAction(m_actExtractF0);
    processMenu->addAction(m_actExtractMidi);
    processMenu->addSeparator();
    processMenu->addAction(tr("Batch Force Align..."), this, &PhonemeLabelerPage::onBatchFA);

    QList<QAction*> zoomActions;
    for (auto* act : m_editor->viewActions()) {
        if (act)
            zoomActions.append(act);
    }
    auto* viewMenu = addStandardViewMenu(bar, zoomActions);
    viewMenu->addMenu(m_editor->spectrogramColorMenu());

    return bar;
}

QWidget* PhonemeLabelerPage::createStatusBarContent(QWidget* parent) {
    auto* container = new QWidget(parent);
    auto builder = buildStandardStatusBar(container);

    auto* posLabel = builder.addLabel(QStringLiteral("0.000s"));

    builder.connect(m_editor, &phonemelabeler::PhonemeEditor::positionChanged, posLabel,
                    [posLabel](double sec) { posLabel->setText(QString::number(sec, 'f', 3) + "s"); });

    return container;
}

// ── Input helpers ──────────────────────────────────────────────────────────────

/// Reads pinyin lyrics text from the original grapheme layer, which is the
/// direct output of the minlabel step (equivalent to .lab in main branch).
/// applyFaResult now creates a separate grapheme layer, preserving the
/// original grapheme for re-running FA.
/// Returns empty string when grapheme layer is absent or empty.
static QString readFaInput(const DsTextDocument& doc) {
    QStringList parts;
    const IntervalLayer* graphemeLayer = nullptr;
    const IntervalLayer* phonemeLayer = nullptr;
    for (const auto& layer : doc.layers) {
        if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme))
            graphemeLayer = &layer;
        else if (layer.name == QString::fromUtf8(dstools::keys::layers::phoneme))
            phonemeLayer = &layer;
    }
    if (graphemeLayer) {
        static const QStringList kNonSpeech = {QStringLiteral("SP"), QStringLiteral("AP")};
        for (const auto& b : graphemeLayer->boundaries) {
            if (!b.text.isEmpty() && !kNonSpeech.contains(b.text))
                parts << b.text;
        }
        if (graphemeLayer && phonemeLayer) {
            for (const auto& dep : doc.dependencies) {
                bool valid = dep.parentLayerIndex >= 0 && dep.childLayerIndex >= 0;
                if (!valid)
                    valid = !dep.parentLayerName.isEmpty() && !dep.childLayerName.isEmpty();
                if (valid) {
                    for (const auto& gb : graphemeLayer->boundaries) {
                        bool found = false;
                        for (const auto& pb : phonemeLayer->boundaries) {
                            if (gb.pos == pb.pos) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            DSFW_LOG_WARN("fa", ("Grapheme boundary '" + gb.text.toStdString() + "' pos=" +
                                                 std::to_string(gb.pos) + " not aligned with any phoneme boundary")
                                                    .c_str());
                        }
                    }
                    break;
                }
            }
        }
    }
    return parts.join(QStringLiteral(" "));
}

// ── FA engine ─────────────────────────────────────────────────────────────────

void PhonemeLabelerPage::onEngineInvalidated(const QString& taskKey) {
    enginePool()->invalidate(taskKey);
    if (taskKey == QLatin1String(dstools::keys::engines::phonemeAlignment)) {
        m_hfa.reset();
        DSFW_LOG_WARN("fa", "FA task cancelled: model invalidated");
    }
}

void PhonemeLabelerPage::onRunFA() {
    if (currentSliceId().isEmpty()) {
        QMessageBox::information(this, tr("Force Align"), tr("Please select a slice first."));
        return;
    }

    m_hfa = enginePool()->acquire<HFA::HFA>(QLatin1String(dstools::keys::engines::phonemeAlignment));
    if (!enginePool()->isOpen<HFA::HFA>(QLatin1String(dstools::keys::engines::phonemeAlignment))) {
        QMessageBox::warning(
            this, tr("Force Align"),
            tr("Force alignment model not loaded. Please configure the HuBERT-FA model path in Settings."));
        return;
    }

    if (isBatchRunning()) {
        QMessageBox::information(this, tr("Force Align"), tr("Force alignment is running, please wait."));
        return;
    }

    runFaForSlice(currentSliceId());
}

void PhonemeLabelerPage::runFaForSlice(const QString& sliceId) {
    if (!source())
        return;

    QString audioPath = source()->validatedAudioPath(sliceId);
    if (audioPath.isEmpty()) {
        if (source()->audioPath(sliceId).isEmpty()) {
            DSFW_LOG_ERROR("fa", ("FA pipeline error [audio]: no audio file - " + sliceId.toStdString()).c_str());
            QMessageBox::warning(this, tr("Force Align"), tr("Current slice has no audio file."));
        }
        return;
    }

    auto loadResult = source()->loadSlice(sliceId);
    if (!loadResult) {
        DSFW_LOG_ERROR("fa",
                       ("FA pipeline error [load]: unable to load slice data - " + sliceId.toStdString()).c_str());
        QMessageBox::warning(this, tr("Force Align"), tr("Unable to load slice data."));
        return;
    }

    const auto& doc = loadResult.value();

    QString lyricsQStr = readFaInput(doc);
    if (lyricsQStr.isEmpty()) {
        DSFW_LOG_ERROR("fa", ("FA pipeline error [input]: no lyrics data - " + sliceId.toStdString()).c_str());
        QMessageBox::information(this, tr("Force Align"),
                                 tr("Current slice has no lyrics data. Please label lyrics first."));
        return;
    }

    setBatchRunning(true);
    std::string lyricsText = lyricsQStr.toStdString();
    std::vector<std::string> nonSpeechPh;
    if (settingsBackend()) {
        auto cfg = settingsBackend()->load()["faConfig"].toObject();
        QString nsStr = cfg.contains("nonSpeechPh") && cfg["nonSpeechPh"].isString() ? cfg["nonSpeechPh"].toString()
                                                                                     : QStringLiteral("AP, SP");
        const auto parts = nsStr.split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const auto& p : parts)
            nonSpeechPh.push_back(p.trimmed().toStdString());
    }
    if (nonSpeechPh.empty())
        nonSpeechPh = {"AP", "SP"};
    DSFW_LOG_INFO("fa", ("FA started: " + sliceId.toStdString() + " | audio: " + audioPath.toStdString() +
                         " | language: zh" + " | nonSpeechPh: AP SP" + " | lyrics: " + lyricsText)
                            .c_str());
    std::weak_ptr<HFA::HFA> weakHfa = m_hfa;

    runAsyncTask<HFA::WordList>(
        QLatin1String(dstools::keys::engines::phonemeAlignment), sliceId,
        [weakHfa, audioPath, lyricsText = std::move(lyricsText),
         nonSpeechPh = std::move(nonSpeechPh)](const std::shared_ptr<std::atomic<bool>>&) -> Result<HFA::WordList> {
            auto hfa = weakHfa.lock();
            if (!hfa)
                return Err<HFA::WordList>("FA engine is null");
            HFA::WordList words;
            auto result = hfa->recognize(audioPath.toStdWString(), "zh", nonSpeechPh, lyricsText, words);
            if (result)
                return std::move(words);
            return Err<HFA::WordList>(result.error());
        },
        [this](const QString& sliceId, const Result<HFA::WordList>& result) {
            setBatchRunning(false);

            if (result) {
                auto faResult = buildFaLayers(result.value());

                std::string phonemeDetail;
                for (const auto& b : faResult.phonemeLayer.boundaries) {
                    if (!phonemeDetail.empty())
                        phonemeDetail += ", ";
                    phonemeDetail += b.text.toStdString() + "@" + std::to_string(usToSec(b.pos));
                }
                DSFW_LOG_INFO("fa", ("FA completed: " + sliceId.toStdString() +
                                     " | phonemes: " + std::to_string(faResult.phonemeLayer.boundaries.size()) +
                                     " | bindings: " + std::to_string(faResult.groups.size()) + " | detail: [" +
                                     phonemeDetail + "]")
                                        .c_str());

                QList<IntervalLayer> layers;
                layers.push_back(std::move(faResult.graphemeLayer));
                layers.push_back(std::move(faResult.phonemeLayer));

                applyFaResult(sliceId, layers, faResult.groups, faResult.dependencies);
                dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info,
                                                       tr("Force alignment completed"), 3000);
            } else {
                DSFW_LOG_ERROR(
                    "fa", ("FA pipeline error [inference]: " + sliceId.toStdString() + " - " + result.error()).c_str());
                QMessageBox::warning(this, tr("Force Align"),
                                     tr("Force alignment failed: %1").arg(QString::fromStdString(result.error())));
            }
        });
}

void PhonemeLabelerPage::applyFaResult(const QString& sliceId, const QList<IntervalLayer>& layers,
                                       const std::vector<BindingGroup>& groups,
                                       const std::vector<LayerDependency>& dependencies) {
    if (!source())
        return;

    auto result = source()->loadSlice(sliceId);
    DsTextDocument doc;
    if (result)
        doc = std::move(result.value());

    DsTextDocBridge::mergeIntervalLayers(doc, layers);

    if (!groups.empty())
        doc.groups = groups;

    if (!dependencies.empty()) {
        doc.dependencies = dependencies;
        for (auto& dep : doc.dependencies) {
            if (dep.parentLayerIndex < 0 && !dep.parentLayerName.isEmpty()) {
                for (int i = 0; i < static_cast<int>(doc.layers.size()); ++i) {
                    if (doc.layers[i].name == dep.parentLayerName) {
                        dep.parentLayerIndex = i;
                        break;
                    }
                }
            }
            if (dep.childLayerIndex < 0 && !dep.childLayerName.isEmpty()) {
                for (int i = 0; i < static_cast<int>(doc.layers.size()); ++i) {
                    if (doc.layers[i].name == dep.childLayerName) {
                        dep.childLayerIndex = i;
                        break;
                    }
                }
            }
        }
    }

    if (auto res = source()->saveSlice(sliceId, doc); !res.ok())
        DSFW_LOG_ERROR("fa", ("Failed to save slice after FA: " + sliceId.toStdString() + " - " + res.error()).c_str());

    if (sliceId == currentSliceId())
        loadFaResultIntoEditor(doc);

    updateProgress();
}

void PhonemeLabelerPage::onBatchFA() {
    if (!source()) {
        QMessageBox::warning(this, tr("Batch Force Align"), tr("Please open a project first."));
        return;
    }

    m_hfa = enginePool()->acquire<HFA::HFA>(QLatin1String(dstools::keys::engines::phonemeAlignment));
    if (!enginePool()->isOpen<HFA::HFA>(QLatin1String(dstools::keys::engines::phonemeAlignment))) {
        QMessageBox::warning(
            this, tr("Force Align"),
            tr("Force alignment model not loaded. Please configure the HuBERT-FA model path in Settings."));
        return;
    }

    if (isBatchRunning()) {
        QMessageBox::information(this, tr("Batch Force Align"), tr("Force alignment is running, please wait."));
        return;
    }

    const auto ids = source()->sliceIds();
    if (ids.isEmpty()) {
        DSFW_LOG_WARN("fa", "Batch FA skipped: no slices");
        QMessageBox::information(this, tr("Batch Force Align"), tr("No slices to process."));
        return;
    }

    auto* dlg = new BatchProcessDialog(tr("Batch Force Align"), this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    auto* skipExisting = new QCheckBox(tr("Skip slices with existing phoneme alignment"), dlg);
    skipExisting->setChecked(true);
    dlg->addParamWidget(skipExisting);

    auto* langCombo = new QComboBox(dlg);
    langCombo->addItem(tr("Chinese"), QStringLiteral("zh"));
    langCombo->addItem(tr("Japanese"), QStringLiteral("ja"));
    langCombo->addItem(tr("English"), QStringLiteral("en"));
    dlg->addParamRow(tr("Language:"), langCombo);

    dlg->appendLog(tr("Total slices: %1").arg(ids.size()));

    auto* hfa = m_hfa.get();
    auto hfaAlive = aliveToken(QLatin1String(dstools::keys::engines::phonemeAlignment)).token();
    auto* src = source();
    QPointer<PhonemeLabelerPage> guard(this);

    connect(
        dlg, &BatchProcessDialog::started, this,
        [dlg, hfa, hfaAlive, src, ids, guard, skipExisting, langCombo, this]() {
            if (!guard) {
                dlg->finish(0, 0, 0);
                return;
            }
            guard->setBatchRunning(true);
            bool skip = skipExisting->isChecked();
            QString lang = langCombo->currentData().toString();
            std::vector<std::string> nonSpeechPh = {"AP", "SP"};
            if (settingsBackend()) {
                auto cfg = settingsBackend()->load()["faConfig"].toObject();
                QString nsStr = cfg.contains("nonSpeechPh") && cfg["nonSpeechPh"].isString()
                                    ? cfg["nonSpeechPh"].toString()
                                    : QStringLiteral("AP, SP");
                const auto parts = nsStr.split(QLatin1Char(','), Qt::SkipEmptyParts);
                nonSpeechPh.clear();
                for (const auto& p : parts)
                    nonSpeechPh.push_back(p.trimmed().toStdString());
            }

            (void)QtConcurrent::run([dlg, hfa, hfaAlive, src, ids, guard, skip, lang, nonSpeechPh]() {
                if (!hfaAlive || !*hfaAlive)
                    return;
                if (!hfa)
                    return;
                int processed = 0;
                int skipped = 0;
                int errors = 0;
                int idx = 0;
                for (const auto& sliceId : ids) {
                    if (!*hfaAlive || dlg->isCancelled())
                        break;

                    QMetaObject::invokeMethod(
                        dlg, [dlg, idx, total = ids.size()]() { dlg->setProgress(idx, total); }, Qt::QueuedConnection);

                    QString audioPath = src->validatedAudioPath(sliceId);
                    if (audioPath.isEmpty()) {
                        ++skipped;
                        QMetaObject::invokeMethod(
                            dlg, [dlg, sliceId]() { dlg->appendLog(tr("[SKIP] %1 (missing audio)").arg(sliceId)); },
                            Qt::QueuedConnection);
                        ++idx;
                        continue;
                    }

                    auto loadResult = src->loadSlice(sliceId);
                    if (!loadResult) {
                        ++errors;
                        QMetaObject::invokeMethod(
                            dlg, [dlg, sliceId]() { dlg->appendLog(tr("[ERROR] %1 (load failed)").arg(sliceId)); },
                            Qt::QueuedConnection);
                        ++idx;
                        continue;
                    }

                    const auto& doc = loadResult.value();

                    QString lyricsQStr = readFaInput(doc);
                    if (lyricsQStr.isEmpty()) {
                        ++skipped;
                        QMetaObject::invokeMethod(
                            dlg, [dlg, sliceId]() { dlg->appendLog(tr("[SKIP] %1 (no lyrics)").arg(sliceId)); },
                            Qt::QueuedConnection);
                        ++idx;
                        continue;
                    }

                    if (skip) {
                        bool hasPhoneme = false;
                        for (const auto& layer : doc.layers) {
                            if (layer.name == QString::fromUtf8(dstools::keys::layers::phoneme) &&
                                !layer.boundaries.empty()) {
                                hasPhoneme = true;
                                break;
                            }
                        }
                        if (hasPhoneme) {
                            ++skipped;
                            QMetaObject::invokeMethod(
                                dlg,
                                [dlg, sliceId]() { dlg->appendLog(tr("[SKIP] %1 (existing alignment)").arg(sliceId)); },
                                Qt::QueuedConnection);
                            ++idx;
                            continue;
                        }
                    }

                    HFA::WordList words;
                    std::string lyricsText = lyricsQStr.toStdString();

                    auto sidStr = sliceId.toStdString();
                    auto langStr = lang.toStdString();

                    DSFW_LOG_INFO(
                        "fa", ("Batch FA started: " + sidStr + " | language: " + langStr + " | lyrics: " + lyricsText)
                                  .c_str());
                    auto result = hfa->recognize(audioPath.toStdWString(), langStr, nonSpeechPh, lyricsText, words);
                    if (result) {
                        auto faResult = buildFaLayers(words);

                        std::string phonemeDetail;
                        for (const auto& b : faResult.phonemeLayer.boundaries) {
                            if (!phonemeDetail.empty())
                                phonemeDetail += ", ";
                            phonemeDetail += b.text.toStdString() + "@" + std::to_string(usToSec(b.pos));
                        }
                        DSFW_LOG_INFO("fa", ("Batch FA completed: " + sidStr +
                                             " | phonemes: " + std::to_string(faResult.phonemeLayer.boundaries.size()) +
                                             " | detail: [" + phonemeDetail + "]")
                                                .c_str());

                        QList<IntervalLayer> layers;
                        layers.push_back(std::move(faResult.graphemeLayer));
                        layers.push_back(std::move(faResult.phonemeLayer));

                        QMetaObject::invokeMethod(
                            guard.data(),
                            [guard, sliceId, layers, groups = std::move(faResult.groups),
                             deps = std::move(faResult.dependencies)]() {
                                if (guard)
                                    guard->applyFaResult(sliceId, layers, groups, deps);
                            },
                            Qt::QueuedConnection);
                        QMetaObject::invokeMethod(
                            dlg,
                            [dlg, sliceId, phoneCount = faResult.phonemeLayer.boundaries.size()]() {
                                dlg->appendLog(tr("[OK] %1: %2 phonemes").arg(sliceId).arg(phoneCount));
                            },
                            Qt::QueuedConnection);
                        ++processed;
                    } else {
                        DSFW_LOG_ERROR("fa", ("Batch FA failed: " + sidStr + " - " + result.error()).c_str());
                        ++errors;
                        QMetaObject::invokeMethod(
                            dlg,
                            [dlg, sliceId, errMsg = result.error()]() {
                                dlg->appendLog(tr("[ERROR] %1: %2").arg(sliceId, QString::fromStdString(errMsg)));
                            },
                            Qt::QueuedConnection);
                    }
                    ++idx;
                }
                QMetaObject::invokeMethod(
                    dlg, [dlg, processed, skipped, errors]() { dlg->finish(processed, skipped, errors); },
                    Qt::QueuedConnection);
                QMetaObject::invokeMethod(
                    guard.data(),
                    [guard]() {
                        if (guard)
                            guard->setBatchRunning(false);
                    },
                    Qt::QueuedConnection);
            });
        });

    connect(dlg, &BatchProcessDialog::cancelled, this, [guard, hfaAlive]() {
        if (hfaAlive)
            hfaAlive->store(false);
        if (guard)
            guard->setBatchRunning(false);
    });

    dlg->show();
}

void PhonemeLabelerPage::updateProgress() {
    EditorPageBase::updateProgress({QString::fromUtf8(dstools::keys::layers::phoneme)});
}

void PhonemeLabelerPage::onExtractF0() {
    if (!m_editor)
        return;

    const QString audioPath = source() ? source()->validatedAudioPath(currentSliceId()) : QString{};
    if (audioPath.isEmpty()) {
        QMessageBox::warning(this, tr("Extract F0"), tr("No audio file available."));
        return;
    }

    if (!settingsBackend())
        return;
    auto cfg = readModelConfig(settingsBackend(), QStringLiteral("rmvpe"));
    if (cfg.modelPath.isEmpty()) {
        QMessageBox::warning(this, tr("Extract F0"), tr("RMVPE model not configured."));
        return;
    }

    auto provider = dsfw::infer::ExecutionProvider::DML;
    int deviceId = 0;

    auto rmvpeProbe = InferBridge::probeRmvpeEngine<Rmvpe::Rmvpe>(std::filesystem::path(cfg.modelPath.toStdWString()),
                                                                  provider, deviceId);
    if (!rmvpeProbe.success()) {
        QMessageBox::warning(this, tr("Extract F0"), tr("Failed to load RMVPE model."));
        return;
    }
    auto& rmvpe = *rmvpeProbe.engine;

    std::vector<Rmvpe::RmvpeRes> results;
    auto result = rmvpe.get_f0(std::filesystem::path(audioPath.toStdWString()), 0.3f, results, nullptr);
    if (!result) {
        QMessageBox::warning(this, tr("Extract F0"),
                             tr("F0 extraction failed: %1").arg(QString::fromStdString(result.error())));
        return;
    }

    if (results.empty() || results[0].f0.empty())
        return;

    auto dsFile = std::make_shared<dstools::pitchlabeler::DsPitchDocument>();
    dsFile->offset = secToUs(results[0].offset);
    static constexpr TimePos kRmvpeTimestep = 5000;
    dsFile->f0.timestep = kRmvpeTimestep;
    dsFile->f0.values = results[0].f0;

    m_editor->pianoRollChart()->setDsPitchDocument(dsFile);
    m_editor->setPianoRollVisible(true);
}

void PhonemeLabelerPage::onExtractMidi() {
    if (!m_editor)
        return;

    const QString audioPath = source() ? source()->validatedAudioPath(currentSliceId()) : QString{};
    if (audioPath.isEmpty()) {
        QMessageBox::warning(this, tr("Extract MIDI"), tr("No audio file available."));
        return;
    }

    if (!settingsBackend())
        return;
    auto cfg = readModelConfig(settingsBackend(), QStringLiteral("game"));
    if (cfg.modelPath.isEmpty()) {
        QMessageBox::warning(this, tr("Extract MIDI"), tr("GAME model not configured."));
        return;
    }

    auto provider = dsfw::infer::ExecutionProvider::DML;
    int deviceId = 0;

    auto gameProbe = InferBridge::probeGameEngine<Game::Game>(std::filesystem::path(cfg.modelPath.toStdWString()),
                                                              provider, deviceId);
    if (!gameProbe.success()) {
        QMessageBox::warning(this, tr("Extract MIDI"),
                             tr("Failed to load GAME model: %1").arg(QString::fromStdString(gameProbe.error)));
        return;
    }
    auto& game = *gameProbe.engine;

    std::vector<Game::GameNote> gameNotes;
    auto result = game.getNotes(std::filesystem::path(audioPath.toStdWString()), gameNotes, nullptr);
    if (!result) {
        QMessageBox::warning(this, tr("Extract MIDI"),
                             tr("MIDI extraction failed: %1").arg(QString::fromStdString(result.error())));
        return;
    }

    if (gameNotes.empty())
        return;

    auto dsFile = std::make_shared<dstools::pitchlabeler::DsPitchDocument>();
    dsFile->offset = 0;

    for (const auto& gn : gameNotes) {
        if (!gn.voiced)
            continue;
        dstools::pitchlabeler::Note note;
        note.name = dstools::midiToNoteName(static_cast<int>(gn.pitch));
        note.duration = secToUs(static_cast<double>(gn.duration));
        note.slur = 0;
        note.glide.clear();
        dsFile->notes.push_back(note);
    }

    dsFile->recomputeNoteStarts();

    m_editor->pianoRollChart()->setDsPitchDocument(dsFile);
    m_editor->setPianoRollVisible(true);
}

void PhonemeLabelerPage::loadFaResultIntoEditor(const DsTextDocument& doc) {
    QList<IntervalLayer> layers;
    for (const auto& layer : doc.layers) {
        if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme) ||
            layer.name == QString::fromUtf8(dstools::keys::layers::phoneme))
            layers.append(layer);
    }

    TimePos duration = 0;
    double durSec = audioDurationSec(doc);
    if (durSec > 0.0) {
        duration = secToUs(durSec);
    } else {
        double audioDur = m_editor->viewport()->totalDuration();
        if (audioDur > 0.0)
            duration = secToUs(audioDur);
    }

    if (duration > 0)
        m_editor->document()->loadFromDsText(layers, duration);

    if (!doc.groups.empty())
        m_editor->document()->setBindingGroups(doc.groups);
    else if (layers.size() > 1)
        m_editor->document()->autoDetectBindingGroups();

    for (int i = 0; i < static_cast<int>(layers.size()); ++i) {
        if (layers[i].name == QString::fromUtf8(dstools::keys::layers::phoneme)) {
            m_editor->document()->setTierReadOnly(i, true);
            break;
        }
    }
}

} // namespace dstools
