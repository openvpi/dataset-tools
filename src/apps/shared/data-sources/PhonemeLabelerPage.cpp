#include "PhonemeLabelerPage.h"
#include "SliceListPanel.h"
#include "ModelConfigHelper.h"
#include "ISettingsBackend.h"
#include "BatchProcessDialog.h"

#include <dstools/IEditorDataSource.h>

#include <dsfw/Log.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>
#include <QtConcurrent>

#include <hubert-infer/Hfa.h>

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

// ---------------------------------------------------------------------------
// Build fa_grapheme + phoneme layers with BindingGroups from HFA WordList.
//
// The FA system accepts grapheme input (from original grapheme layer) and
// produces Word→Phone mapping. FA output is stored in a fa_grapheme layer
// (word-level boundaries from FA alignment) and a phoneme layer (phone-level).
// Where a word start coincides with a phone start, those two boundary IDs
// form a BindingGroup (they must move together). The original grapheme layer
// (G2P output) is preserved as reference for re-running FA.
//
// SP/AP words are included in fa_grapheme so that fa_grapheme and phoneme
// layers have aligned dependency relationships. readFaInput() reads from the
// original grapheme layer and filters out SP/AP text.
// ---------------------------------------------------------------------------
struct FaLayerResult {
    IntervalLayer graphemeLayer;
    IntervalLayer phonemeLayer;
    std::vector<BindingGroup> groups;
    std::vector<LayerDependency> dependencies;
};

static FaLayerResult buildFaLayers(const HFA::WordList &words) {
    FaLayerResult r;
    r.graphemeLayer.name = QStringLiteral("fa_grapheme");
    r.graphemeLayer.type = QStringLiteral("interval");
    r.phonemeLayer.name = QStringLiteral("phoneme");
    r.phonemeLayer.type = QStringLiteral("interval");

    int nextId = 1;

    for (const auto &word : words) {
        if (word.phones.empty())
            continue;

        Boundary graphemeStart;
        graphemeStart.id = nextId++;
        graphemeStart.pos = secToUs(std::max(0.0f, word.start));
        graphemeStart.text = QString::fromStdString(word.text);
        int graphemeStartId = graphemeStart.id;
        r.graphemeLayer.boundaries.push_back(std::move(graphemeStart));

        Boundary graphemeEnd;
        graphemeEnd.id = nextId++;
        graphemeEnd.pos = secToUs(std::max(0.0f, word.end));
        int graphemeEndId = graphemeEnd.id;
        r.graphemeLayer.boundaries.push_back(std::move(graphemeEnd));

        std::vector<int> childBoundaryIds;

        for (size_t pi = 0; pi < word.phones.size(); ++pi) {
            const auto &phone = word.phones[pi];

            Boundary phoneB;
            phoneB.id = nextId++;
            phoneB.pos = secToUs(std::max(0.0f, phone.start));
            phoneB.text = QString::fromStdString(phone.text);

            childBoundaryIds.push_back(phoneB.id);

            if (pi == 0)
                r.groups.push_back({graphemeStartId, phoneB.id});

            r.phonemeLayer.boundaries.push_back(std::move(phoneB));
        }

        if (!word.phones.empty()) {
            r.groups.push_back({graphemeEndId, r.phonemeLayer.boundaries.back().id});
        }

        LayerDependency dep;
        dep.parentLayerName = QStringLiteral("fa_grapheme");
        dep.childLayerName = QStringLiteral("phoneme");
        dep.parentStartBoundaryId = graphemeStartId;
        dep.parentEndBoundaryId = graphemeEndId;
        if (!childBoundaryIds.empty()) {
            dep.childStartBoundaryId = childBoundaryIds.front();
            dep.childEndBoundaryId = childBoundaryIds.back();
        }
        dep.childBoundaryIds = std::move(childBoundaryIds);
        r.dependencies.push_back(std::move(dep));
    }

    return r;
}

PhonemeLabelerPage::PhonemeLabelerPage(QWidget *parent)
    : EditorPageBase("PhonemeLabeler", parent) {
    m_editor = new phonemelabeler::PhonemeEditor(this);

    setupBaseLayout(m_editor);
    setupNavigationActions();

    m_faAction = new QAction(tr("Force Align Current Slice"), this);
    connect(m_faAction, &QAction::triggered, this, &PhonemeLabelerPage::onRunFA);

    // Add FA button to the editor toolbar
    m_editor->toolbar()->addAction(m_faAction);

    static const dstools::SettingsKey<QString> kShortcutSave("Shortcuts/save", "Ctrl+S");
    static const dstools::SettingsKey<QString> kShortcutUndo("Shortcuts/undo", "Ctrl+Z");
    static const dstools::SettingsKey<QString> kShortcutRedo("Shortcuts/redo", "Ctrl+Y");
    static const dstools::SettingsKey<QString> kShortcutFA("Shortcuts/fa", "F");
    static const dstools::SettingsKey<QString> kShortcutPlayPause("Shortcuts/playPause", "Space");

    shortcutManager()->bind(m_editor->saveAction(), kShortcutSave,
                            tr("Save"), tr("File"));
    shortcutManager()->bind(m_editor->undoAction(), kShortcutUndo,
                            tr("Undo"), tr("Edit"));
    shortcutManager()->bind(m_editor->redoAction(), kShortcutRedo,
                            tr("Redo"), tr("Edit"));
    shortcutManager()->bind(m_faAction, kShortcutFA,
                            tr("Force Align"), tr("Processing"));
    shortcutManager()->bind(m_editor->playPauseAction(), kShortcutPlayPause,
                            tr("Play/Pause"), tr("Playback"));
    shortcutManager()->applyAll();
    shortcutManager()->updateTooltips();
    shortcutManager()->setEnabled(false);

    connect(m_editor->saveAction(), &QAction::triggered,
            this, [this]() { saveCurrentSlice(); updateDirtyIndicator(); });
    connect(m_editor->document(), &phonemelabeler::TextGridDocument::modifiedChanged,
            this, [this]() { updateDirtyIndicator(); });
}

PhonemeLabelerPage::~PhonemeLabelerPage() = default;

// ── Batch processing (P-12 template) ──────────────────────────────────────────

bool PhonemeLabelerPage::isBatchRunning() const {
    return m_batchRunning;
}

void PhonemeLabelerPage::setBatchRunning(bool running) {
    m_batchRunning = running;
}

std::shared_ptr<std::atomic<bool>> PhonemeLabelerPage::batchAliveToken() const {
    return m_batchAlive;
}

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

    auto *doc = m_editor->document();

    auto result = source()->loadSlice(currentSliceId());
    DsTextDocument dstext;
    if (result)
        dstext = std::move(result.value());

    IntervalLayer rawTextLayer;
    for (const auto &layer : dstext.layers) {
        if (layer.name == QStringLiteral("raw_text")) {
            rawTextLayer = layer;
            break;
        }
    }

    auto layers = doc->toDsText();
    dstext.layers.clear();
    for (const auto &layer : layers)
        dstext.layers.push_back(layer);

    if (!rawTextLayer.name.isEmpty())
        dstext.layers.push_back(std::move(rawTextLayer));

    // Preserve binding groups from the current document
    dstext.groups = doc->bindingGroups();

    auto saveResult = source()->saveSlice(currentSliceId(), dstext);
    if (!saveResult) {
        QMessageBox::warning(this, tr("Save Failed"),
                             QString::fromStdString(saveResult.error()));
        return false;
    }

    return true;
}

void PhonemeLabelerPage::onSliceSelectedImpl(const QString &sliceId) {
    const QString audioPath = source()->validatedAudioPath(sliceId);

    auto result = source()->loadSlice(sliceId);
    if (result && !result.value().layers.empty()) {
        const auto &doc = result.value();

        QList<IntervalLayer> layers;
        for (const auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("fa_grapheme")
                || layer.name == QStringLiteral("phoneme"))
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

        for (int i = 0; i < layers.size(); ++i) {
            if (layers[i].name == QStringLiteral("phoneme")) {
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
}

void PhonemeLabelerPage::onDeactivatedImpl() {
    if (m_hfaAlive)
        m_hfaAlive->store(false);
    m_hfa = nullptr;
    cancelAsyncTask(m_hfaAlive);
    if (m_editor && m_editor->playWidget())
        m_editor->playWidget()->setPlaying(false);
    DSFW_LOG_WARN("fa", "FA task cancelled: page deactivated");
}

void PhonemeLabelerPage::restoreExtraSplitters() {
    static const dstools::SettingsKey<QString> kEditorSplitterState("Layout/editorSplitterState", "");
    auto state = settings().get(kEditorSplitterState);
    if (!state.isEmpty())
        m_editor->restoreSplitterState(QByteArray::fromBase64(state.toUtf8()));
    m_editor->restoreChartVisibility();
}

void PhonemeLabelerPage::saveExtraSplitters() {
    static const dstools::SettingsKey<QString> kEditorSplitterState("Layout/editorSplitterState", "");
    settings().set(kEditorSplitterState, QString::fromLatin1(m_editor->saveSplitterState().toBase64()));
    m_editor->saveChartVisibility();
    m_editor->saveViewportResolution();
}

void PhonemeLabelerPage::onAutoInfer() {
    updateProgress();

    if (settingsBackend()) {
        auto settingsData = settingsBackend()->load();
        auto preload = settingsData["preload"].toObject();
        auto faPreload = preload["phoneme_alignment"].toObject();
        if (faPreload["enabled"].toBool(false)) {
            ensureHfaEngineAsync();
        }
    }

    if (source() && !currentSliceId().isEmpty()) {
        auto dirty = source()->dirtyLayers(currentSliceId());

        bool needAutoFA = dirty.contains(QStringLiteral("phoneme"));

        if (!needAutoFA && !currentSliceId().isEmpty()) {
            auto result = source()->loadSlice(currentSliceId());
            if (result) {
                bool hasGrapheme = false;
                bool hasPhoneme = false;
                for (const auto &layer : result.value().layers) {
                    if (layer.name == QStringLiteral("grapheme") && !layer.boundaries.empty())
                        hasGrapheme = true;
                    if (layer.name == QStringLiteral("phoneme") && !layer.boundaries.empty())
                        hasPhoneme = true;
                }
                if (hasGrapheme && !hasPhoneme)
                    needAutoFA = true;
            }
        }

        if (needAutoFA) {
            source()->clearDirtyLayers(currentSliceId(), {QStringLiteral("phoneme")});

            ensureHfaEngineAsync([this]() {
                if (m_hfa && m_hfa->isOpen() && !m_faRunning) {
                    dsfw::widgets::ToastNotification::show(
                        this, dsfw::widgets::ToastType::Info,
                        tr("Auto force-aligning..."), 3000);
                    runFaForSlice(currentSliceId());
                } else {
                    dsfw::widgets::ToastNotification::show(
                        this, dsfw::widgets::ToastType::Warning,
                        tr("Phoneme layer outdated, please run force align manually"),
                        3000);
                }
            });
        }
    }
}

// ── IPageActions ──────────────────────────────────────────────────────────────

QMenuBar *PhonemeLabelerPage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);

    auto *fileMenu = bar->addMenu(tr("&File"));
    fileMenu->addAction(m_editor->saveAction());
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), this, [this]() {
        if (auto *w = window()) w->close();
    });

    auto *editMenu = bar->addMenu(tr("&Edit"));
    editMenu->addAction(m_editor->undoAction());
    editMenu->addAction(m_editor->redoAction());
    editMenu->addSeparator();
    editMenu->addAction(prevAction());
    editMenu->addAction(nextAction());
    editMenu->addSeparator();
    {
        auto *shortcutAction = editMenu->addAction(tr("Shortcut Settings..."));
        connect(shortcutAction, &QAction::triggered, this, [this]() {
            shortcutManager()->showEditor(this);
        });
    }

    auto *processMenu = bar->addMenu(tr("&Processing"));
    processMenu->addAction(m_faAction);
    processMenu->addSeparator();
    processMenu->addAction(tr("Batch Force Align..."), this, &PhonemeLabelerPage::onBatchFA);

    auto *viewMenu = bar->addMenu(tr("&View"));
    for (auto *act : m_editor->viewActions()) {
        if (act)
            viewMenu->addAction(act);
        else
            viewMenu->addSeparator();
    }
    viewMenu->addMenu(m_editor->spectrogramColorMenu());
    viewMenu->addSeparator();
    dsfw::Theme::instance().populateThemeMenu(viewMenu);

    return bar;
}

QWidget *PhonemeLabelerPage::createStatusBarContent(QWidget *parent) {
    auto *container = new QWidget(parent);
    StatusBarBuilder builder(container);

    auto *sliceLabel = builder.addLabel(tr("No slice selected"), 1);
    auto *posLabel = builder.addLabel(QStringLiteral("0.000s"));

    builder.connect(this, &PhonemeLabelerPage::sliceChanged, sliceLabel,
                    [this, sliceLabel](const QString &id) {
        sliceLabel->setText(id.isEmpty() ? tr("No slice selected") : id);
    });
    builder.connect(m_editor, &phonemelabeler::PhonemeEditor::positionChanged, posLabel,
                    [posLabel](double sec) {
        posLabel->setText(QString::number(sec, 'f', 3) + "s");
    });

    return container;
}

// ── Input helpers ──────────────────────────────────────────────────────────────

/// Reads pinyin lyrics text from the original grapheme layer, which is the
/// direct output of the minlabel step (equivalent to .lab in main branch).
/// applyFaResult now creates a separate fa_grapheme layer, preserving the
/// original grapheme for re-running FA.
/// Returns empty string when grapheme layer is absent or empty.
static QString readFaInput(const DsTextDocument &doc) {
    QStringList parts;
    const IntervalLayer *graphemeLayer = nullptr;
    const IntervalLayer *phonemeLayer = nullptr;
    for (const auto &layer : doc.layers) {
        if (layer.name == QStringLiteral("grapheme"))
            graphemeLayer = &layer;
        else if (layer.name == QStringLiteral("phoneme"))
            phonemeLayer = &layer;
    }
    if (graphemeLayer) {
        static const QStringList kNonSpeech = {QStringLiteral("SP"), QStringLiteral("AP")};
        for (const auto &b : graphemeLayer->boundaries) {
            if (!b.text.isEmpty() && !kNonSpeech.contains(b.text))
                parts << b.text;
        }
        if (graphemeLayer && phonemeLayer) {
            for (const auto &dep : doc.dependencies) {
                bool valid = dep.parentLayerIndex >= 0 && dep.childLayerIndex >= 0;
                if (!valid)
                    valid = !dep.parentLayerName.isEmpty() && !dep.childLayerName.isEmpty();
                if (valid) {
                    for (const auto &gb : graphemeLayer->boundaries) {
                        bool found = false;
                        for (const auto &pb : phonemeLayer->boundaries) {
                            if (gb.pos == pb.pos) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            DSFW_LOG_WARN("fa",
                                ("Grapheme boundary '" + gb.text.toStdString()
                                 + "' pos=" + std::to_string(gb.pos)
                                 + " not aligned with any phoneme boundary").c_str());
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

void PhonemeLabelerPage::ensureHfaEngine() {
    if (m_hfa && m_hfa->isOpen())
        return;

    if (!m_modelManager) {
        m_modelManager = ServiceLocator::get<IModelManager>();
        if (m_modelManager) {
            connect(m_modelManager, &IModelManager::modelInvalidated,
                    this, &PhonemeLabelerPage::onModelInvalidated);
        }
    }

    if (!m_modelManager)
        return;

    auto *mm = dynamic_cast<ModelManager *>(m_modelManager);
    if (!mm)
        return;

    auto config = readModelConfig(settingsBackend(), QStringLiteral("phoneme_alignment"));
    if (config.modelPath.isEmpty())
        return;

    auto result = mm->loadModel(QStringLiteral("phoneme_alignment"), config, config.deviceId);
    if (!result)
        return;

    auto typeId = registerModelType("phoneme_alignment");
    auto *provider = mm->provider(typeId);
    auto *hfaProvider = dynamic_cast<InferenceModelProvider<HFA::HFA> *>(provider);
    if (hfaProvider && hfaProvider->engine().isOpen()) {
        m_hfa = &hfaProvider->engine();
        m_hfaAlive = std::make_shared<std::atomic<bool>>(true);
    }
}

void PhonemeLabelerPage::onModelInvalidated(const QString &taskKey) {
    if (taskKey == QStringLiteral("phoneme_alignment")) {
        if (m_hfaAlive)
            m_hfaAlive->store(false);
        m_hfa = nullptr;
        cancelAsyncTask(m_hfaAlive);
        DSFW_LOG_WARN("fa", "FA task cancelled: model invalidated");
    }
}

void PhonemeLabelerPage::ensureHfaEngineAsync(std::function<void()> onReady) {
    // Already loaded — invoke callback immediately
    if (m_hfa && m_hfa->isOpen()) {
        if (onReady) onReady();
        return;
    }

    // Already loading in background
    if (isEngineLoading(QStringLiteral("phoneme_alignment")))
        return;

    // ModelManager::loadModel must run on main thread (Q_ASSERT).
    // The heavy work is IModelProvider::load() (ONNX session creation).
    // We use loadEngineAsync to track loading state and provide callback,
    // but the actual load is deferred to a QTimer::singleShot so the
    // current call stack (onAutoInfer → onActivated) can return first.
    QPointer<PhonemeLabelerPage> guard(this);
    QTimer::singleShot(0, this, [this, guard, onReady = std::move(onReady)]() {
        if (!guard)
            return;
        ensureHfaEngine();
        if (m_hfa && m_hfa->isOpen() && onReady)
            onReady();
    });
}

void PhonemeLabelerPage::onRunFA() {
    if (currentSliceId().isEmpty()) {
        QMessageBox::information(this, tr("Force Align"),
                                 tr("Please select a slice first."));
        return;
    }

    ensureHfaEngine();
    if (!m_hfa || !m_hfa->isOpen()) {
        QMessageBox::warning(this, tr("Force Align"),
                             tr("Force alignment model not loaded. Please configure the HuBERT-FA model path in Settings."));
        return;
    }

    if (m_faRunning) {
        QMessageBox::information(this, tr("Force Align"),
                                 tr("Force alignment is running, please wait."));
        return;
    }

    runFaForSlice(currentSliceId());
}

void PhonemeLabelerPage::runFaForSlice(const QString &sliceId) {
    if (!source())
        return;

    QString audioPath = source()->validatedAudioPath(sliceId);
    if (audioPath.isEmpty()) {
        if (source()->audioPath(sliceId).isEmpty()) {
            DSFW_LOG_ERROR("fa", ("FA pipeline error [audio]: no audio file - " + sliceId.toStdString()).c_str());
            QMessageBox::warning(this, tr("Force Align"),
                                 tr("Current slice has no audio file."));
        }
        return;
    }

    auto loadResult = source()->loadSlice(sliceId);
    if (!loadResult) {
        DSFW_LOG_ERROR("fa", ("FA pipeline error [load]: unable to load slice data - " + sliceId.toStdString()).c_str());
        QMessageBox::warning(this, tr("Force Align"),
                             tr("Unable to load slice data."));
        return;
    }

    const auto &doc = loadResult.value();

    QString lyricsQStr = readFaInput(doc);
    if (lyricsQStr.isEmpty()) {
        DSFW_LOG_ERROR("fa", ("FA pipeline error [input]: no lyrics data - " + sliceId.toStdString()).c_str());
        QMessageBox::information(this, tr("Force Align"),
                                 tr("Current slice has no lyrics data. Please label lyrics first."));
        return;
    }

    m_faRunning = true;
    std::string lyricsText = lyricsQStr.toStdString();
    std::vector<std::string> nonSpeechPh = {"AP", "SP"};
    DSFW_LOG_INFO("fa", ("FA started: " + sliceId.toStdString()
                          + " | audio: " + audioPath.toStdString()
                          + " | language: zh"
                          + " | nonSpeechPh: AP SP"
                          + " | lyrics: " + lyricsText).c_str());
    auto *hfa = m_hfa;
    auto hfaAlive = m_hfaAlive;
    QPointer<PhonemeLabelerPage> guard(this);

    (void) QtConcurrent::run([hfa, hfaAlive, audioPath, lyricsText, nonSpeechPh, sliceId, guard]() {
        if (!hfaAlive || !*hfaAlive)
            return;
        if (!hfa)
            return;
        HFA::WordList words;
        dstools::Result<void> result = Err("Not executed");

        try {
            result = hfa->recognize(audioPath.toStdWString(), "zh", nonSpeechPh, lyricsText, words);
        } catch (const std::exception &e) {
            DSFW_LOG_ERROR("infer", ("FA inference exception: " + sliceId.toStdString() + " - " + e.what()).c_str());
            result = Err(std::string("Exception: ") + e.what());
        }

        if (!guard)
            return;

        QMetaObject::invokeMethod(guard.data(), [guard, sliceId, words = std::move(words), result = std::move(result)]() {
            if (!guard)
                return;
            guard->m_faRunning = false;

            if (result) {
                auto faResult = buildFaLayers(words);

                std::string phonemeDetail;
                for (const auto &b : faResult.phonemeLayer.boundaries) {
                    if (!phonemeDetail.empty()) phonemeDetail += ", ";
                    phonemeDetail += b.text.toStdString() + "@" + std::to_string(usToSec(b.pos));
                }
                DSFW_LOG_INFO("fa", ("FA completed: " + sliceId.toStdString()
                                      + " | phonemes: " + std::to_string(faResult.phonemeLayer.boundaries.size())
                                      + " | bindings: " + std::to_string(faResult.groups.size())
                                      + " | detail: [" + phonemeDetail + "]").c_str());

                QList<IntervalLayer> layers;
                layers.push_back(std::move(faResult.graphemeLayer));
                layers.push_back(std::move(faResult.phonemeLayer));

                guard->applyFaResult(sliceId, layers, faResult.groups, faResult.dependencies);
                dsfw::widgets::ToastNotification::show(
                    guard.data(), dsfw::widgets::ToastType::Info,
                    tr("Force alignment completed"), 3000);
            } else {
                DSFW_LOG_ERROR("fa", ("FA pipeline error [inference]: " + sliceId.toStdString() + " - " + result.error()).c_str());
                QMessageBox::warning(guard.data(), tr("Force Align"),
                                     tr("Force alignment failed: %1")
                                         .arg(QString::fromStdString(result.error())));
            }
        }, Qt::QueuedConnection);
    });
}

void PhonemeLabelerPage::applyFaResult(const QString &sliceId,
                                        const QList<IntervalLayer> &layers,
                                        const std::vector<BindingGroup> &groups,
                                        const std::vector<LayerDependency> &dependencies) {
    if (!source())
        return;

    auto result = source()->loadSlice(sliceId);
    DsTextDocument doc;
    if (result)
        doc = std::move(result.value());

    for (const auto &newLayer : layers) {
        IntervalLayer *existing = nullptr;
        for (auto &layer : doc.layers) {
            if (layer.name == newLayer.name) {
                existing = &layer;
                break;
            }
        }
        if (existing) {
            existing->boundaries = newLayer.boundaries;
            existing->type = newLayer.type;
        } else {
            doc.layers.push_back(newLayer);
        }
    }

    // Preserve the original grapheme layer (G2P output) alongside
    // the FA-aligned fa_grapheme layer. fa_grapheme contains the
    // FA result with word-level boundaries; grapheme retains the
    // original input for re-running FA.

    if (!groups.empty())
        doc.groups = groups;

    if (!dependencies.empty()) {
        doc.dependencies = dependencies;
        for (auto &dep : doc.dependencies) {
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

    (void) source()->saveSlice(sliceId, doc);

    if (sliceId == currentSliceId()) {
        QList<IntervalLayer> allLayers;
        for (const auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("fa_grapheme")
                || layer.name == QStringLiteral("phoneme"))
                allLayers.append(layer);
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
            m_editor->document()->loadFromDsText(allLayers, duration);

        if (!doc.groups.empty())
            m_editor->document()->setBindingGroups(doc.groups);
        else if (allLayers.size() > 1)
            m_editor->document()->autoDetectBindingGroups();

        for (int i = 0; i < allLayers.size(); ++i) {
            if (allLayers[i].name == QStringLiteral("phoneme")) {
                m_editor->document()->setTierReadOnly(i, true);
                break;
            }
        }
    }

    updateProgress();
}

void PhonemeLabelerPage::onBatchFA() {
    if (!source()) {
        QMessageBox::warning(this, tr("Batch Force Align"),
                             tr("Please open a project first."));
        return;
    }

    ensureHfaEngine();
    if (!m_hfa || !m_hfa->isOpen()) {
        QMessageBox::warning(this, tr("Batch Force Align"),
                             tr("Force alignment model not loaded. Please configure the HuBERT-FA model path in Settings."));
        return;
    }

    if (m_faRunning) {
        QMessageBox::information(this, tr("Batch Force Align"),
                                 tr("Force alignment is running, please wait."));
        return;
    }

    const auto ids = source()->sliceIds();
    if (ids.isEmpty()) {
        DSFW_LOG_WARN("fa", "Batch FA skipped: no slices");
        QMessageBox::information(this, tr("Batch Force Align"),
                                 tr("No slices to process."));
        return;
    }

    auto *dlg = new BatchProcessDialog(tr("Batch Force Align"), this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    auto *skipExisting = new QCheckBox(tr("Skip slices with existing phoneme alignment"), dlg);
    skipExisting->setChecked(true);
    dlg->addParamWidget(skipExisting);

    auto *langCombo = new QComboBox(dlg);
    langCombo->addItem(tr("Chinese"), QStringLiteral("zh"));
    langCombo->addItem(tr("Japanese"), QStringLiteral("ja"));
    langCombo->addItem(tr("English"), QStringLiteral("en"));
    dlg->addParamRow(tr("Language:"), langCombo);

    dlg->appendLog(tr("Total slices: %1").arg(ids.size()));

    auto *hfa = m_hfa;
    auto hfaAlive = m_hfaAlive;
    auto *src = source();
    QPointer<PhonemeLabelerPage> guard(this);

    connect(dlg, &BatchProcessDialog::started, this,
            [dlg, hfa, hfaAlive, src, ids, guard, skipExisting, langCombo]() {
        if (!guard) {
            dlg->finish(0, 0, 0);
            return;
        }
        guard->m_faRunning = true;
        bool skip = skipExisting->isChecked();
        QString lang = langCombo->currentData().toString();

        (void) QtConcurrent::run([dlg, hfa, hfaAlive, src, ids, guard, skip, lang]() {
            if (!hfaAlive || !*hfaAlive)
                return;
            if (!hfa)
                return;
            int processed = 0;
            int skipped = 0;
            int errors = 0;
            int idx = 0;
            for (const auto &sliceId : ids) {
                if (!*hfaAlive || dlg->isCancelled())
                    break;

                QMetaObject::invokeMethod(dlg, [dlg, idx, total = ids.size()]() {
                    dlg->setProgress(idx, total);
                }, Qt::QueuedConnection);

                QString audioPath = src->validatedAudioPath(sliceId);
                if (audioPath.isEmpty()) {
                    ++skipped;
                    QMetaObject::invokeMethod(dlg, [dlg, sliceId]() {
                        dlg->appendLog(tr("[SKIP] %1 (missing audio)").arg(sliceId));
                    }, Qt::QueuedConnection);
                    ++idx;
                    continue;
                }

                auto loadResult = src->loadSlice(sliceId);
                if (!loadResult) {
                    ++errors;
                    QMetaObject::invokeMethod(dlg, [dlg, sliceId]() {
                        dlg->appendLog(tr("[ERROR] %1 (load failed)").arg(sliceId));
                    }, Qt::QueuedConnection);
                    ++idx;
                    continue;
                }

                const auto &doc = loadResult.value();

                QString lyricsQStr = readFaInput(doc);
                if (lyricsQStr.isEmpty()) {
                    ++skipped;
                    QMetaObject::invokeMethod(dlg, [dlg, sliceId]() {
                        dlg->appendLog(tr("[SKIP] %1 (no lyrics)").arg(sliceId));
                    }, Qt::QueuedConnection);
                    ++idx;
                    continue;
                }

                if (skip) {
                    bool hasPhoneme = false;
                    for (const auto &layer : doc.layers) {
                        if (layer.name == QStringLiteral("phoneme") && !layer.boundaries.empty()) {
                            hasPhoneme = true;
                            break;
                        }
                    }
                    if (hasPhoneme) {
                        ++skipped;
                        QMetaObject::invokeMethod(dlg, [dlg, sliceId]() {
                            dlg->appendLog(tr("[SKIP] %1 (existing alignment)").arg(sliceId));
                        }, Qt::QueuedConnection);
                        ++idx;
                        continue;
                    }
                }

                HFA::WordList words;
                std::string lyricsText = lyricsQStr.toStdString();

                std::vector<std::string> nonSpeechPh = {"AP", "SP"};
                DSFW_LOG_INFO("fa", ("Batch FA started: " + sliceId.toStdString()
                                      + " | language: " + lang.toStdString()
                                      + " | lyrics: " + lyricsText).c_str());
                auto result = hfa->recognize(audioPath.toStdWString(), lang.toStdString(), nonSpeechPh, lyricsText, words);
                if (result) {
                    auto faResult = buildFaLayers(words);

                    std::string phonemeDetail;
                    for (const auto &b : faResult.phonemeLayer.boundaries) {
                        if (!phonemeDetail.empty()) phonemeDetail += ", ";
                        phonemeDetail += b.text.toStdString() + "@" + std::to_string(usToSec(b.pos));
                    }
                    DSFW_LOG_INFO("fa", ("Batch FA completed: " + sliceId.toStdString()
                                          + " | phonemes: " + std::to_string(faResult.phonemeLayer.boundaries.size())
                                          + " | detail: [" + phonemeDetail + "]").c_str());

                    QList<IntervalLayer> layers;
                    layers.push_back(std::move(faResult.graphemeLayer));
                    layers.push_back(std::move(faResult.phonemeLayer));

                    QMetaObject::invokeMethod(guard.data(), [guard, sliceId, layers, groups = std::move(faResult.groups), deps = std::move(faResult.dependencies)]() {
                        if (guard)
                            guard->applyFaResult(sliceId, layers, groups, deps);
                    }, Qt::QueuedConnection);
                    QMetaObject::invokeMethod(dlg, [dlg, sliceId, phoneCount = faResult.phonemeLayer.boundaries.size()]() {
                        dlg->appendLog(tr("[OK] %1: %2 phonemes").arg(sliceId).arg(phoneCount));
                    }, Qt::QueuedConnection);
                    ++processed;
                } else {
                    DSFW_LOG_ERROR("fa", ("Batch FA failed: " + sliceId.toStdString() + " - " + result.error()).c_str());
                    ++errors;
                    QMetaObject::invokeMethod(dlg, [dlg, sliceId, errMsg = result.error()]() {
                        dlg->appendLog(tr("[ERROR] %1: %2").arg(sliceId, QString::fromStdString(errMsg)));
                    }, Qt::QueuedConnection);
                }
                ++idx;
            }
            QMetaObject::invokeMethod(dlg, [dlg, processed, skipped, errors]() {
                dlg->finish(processed, skipped, errors);
            }, Qt::QueuedConnection);
            QMetaObject::invokeMethod(guard.data(), [guard]() {
                if (guard)
                    guard->m_faRunning = false;
            }, Qt::QueuedConnection);
        });
    });

    connect(dlg, &BatchProcessDialog::cancelled, this, [guard, hfaAlive]() {
        if (hfaAlive)
            hfaAlive->store(false);
        if (guard)
            guard->m_faRunning = false;
    });

    dlg->show();
}

void PhonemeLabelerPage::updateProgress() {
    if (!source()) return;
    const auto ids = source()->sliceIds();
    int total = ids.size();
    int completed = 0;
    for (const auto &id : ids) {
        auto result = source()->loadSlice(id);
        if (result) {
            for (const auto &layer : result.value().layers) {
                if (layer.name == QStringLiteral("phoneme") && !layer.boundaries.empty()) {
                    ++completed;
                    break;
                }
            }
        }
    }
    sliceListPanel()->setProgress(completed, total);
}

} // namespace dstools
