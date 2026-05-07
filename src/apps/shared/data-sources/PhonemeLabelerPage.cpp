#include "PhonemeLabelerPage.h"
#include "SliceListPanel.h"
#include "ModelConfigHelper.h"
#include "ISettingsBackend.h"

#include <dstools/IEditorDataSource.h>

#include <dsfw/Log.h>

#include <QHBoxLayout>
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
// Build grapheme + phoneme layers with BindingGroups from HFA WordList.
//
// The FA system accepts grapheme input and produces Word→Phone mapping.
// Word boundaries are grapheme-layer boundaries; phone boundaries are
// phoneme-layer boundaries. Where a word start coincides with a phone start,
// those two boundary IDs form a BindingGroup (they must move together).
//
// Leading/trailing auto-added SP/AP words are filtered out.
// ---------------------------------------------------------------------------
struct FaLayerResult {
    IntervalLayer graphemeLayer;
    IntervalLayer phonemeLayer;
    std::vector<BindingGroup> groups;
};

static FaLayerResult buildFaLayers(const HFA::WordList &words) {
    FaLayerResult r;
    r.graphemeLayer.name = QStringLiteral("grapheme");
    r.graphemeLayer.type = QStringLiteral("interval");
    r.phonemeLayer.name = QStringLiteral("phoneme");
    r.phonemeLayer.type = QStringLiteral("interval");

    int nextId = 1;
    int groupIdx = 0;

    for (const auto &word : words) {
        if (word.phones.empty())
            continue;

        Boundary graphemeB;
        graphemeB.id = nextId++;
        graphemeB.pos = secToUs(word.start);
        graphemeB.text = QString::fromStdString(word.text);
        int graphemeBId = graphemeB.id;

        DSFW_LOG_DEBUG("fa",
            ("[tier=0/grapheme] id=" + std::to_string(graphemeB.id) +
             " word='" + word.text + "' @ " +
             std::to_string(word.start) + "s-" +
             std::to_string(word.end) + "s").c_str());

        r.graphemeLayer.boundaries.push_back(std::move(graphemeB));

        std::string containmentLog;
        for (size_t pi = 0; pi < word.phones.size(); ++pi) {
            const auto &phone = word.phones[pi];

            Boundary phoneB;
            phoneB.id = nextId++;
            phoneB.pos = (pi == 0) ? secToUs(word.start) : secToUs(phone.start);
            phoneB.text = QString::fromStdString(phone.text);

            DSFW_LOG_DEBUG("fa",
                ("[tier=1/phoneme] id=" + std::to_string(phoneB.id) +
                 " phone='" + phone.text + "' @ " +
                 std::to_string(phone.start) + "s").c_str());

            if (pi == 0) {
                r.groups.push_back({graphemeBId, phoneB.id});

                DSFW_LOG_INFO("fa",
                    ("[binding#" + std::to_string(groupIdx++) +
                     "] tier0.id=" + std::to_string(graphemeBId) +
                     " ↔ tier1.id=" + std::to_string(phoneB.id) +
                     " @ " + std::to_string(word.start) + "s").c_str());
            }

            if (pi > 0) containmentLog += ", ";
            containmentLog += phone.text + " [" +
                std::to_string(phone.start) + "-" +
                std::to_string(phone.end) + "s]";

            r.phonemeLayer.boundaries.push_back(std::move(phoneB));
        }

        DSFW_LOG_INFO("fa",
            ("grapheme \"" + word.text + "\" [" +
             std::to_string(word.start) + "-" +
             std::to_string(word.end) + "s] → phones: " +
             containmentLog).c_str());
    }

    if (!r.graphemeLayer.boundaries.empty() && !words.empty()) {
        Boundary endG;
        endG.id = nextId++;
        endG.pos = secToUs(words.back().end);
        r.graphemeLayer.boundaries.push_back(std::move(endG));

        DSFW_LOG_DEBUG("fa",
            ("[tier=0/grapheme] id=" + std::to_string(endG.id) +
             " END @ " + std::to_string(words.back().end) + "s").c_str());
    }

    if (!r.phonemeLayer.boundaries.empty() && !words.empty()) {
        TimePos lastEnd = 0;
        for (auto it = words.end(); it != words.begin(); ) {
            --it;
            if (!it->phones.empty()) {
                lastEnd = secToUs(it->phones.back().end);
                break;
            }
        }
        Boundary endP;
        endP.id = nextId++;
        endP.pos = lastEnd;
        endP.text = QStringLiteral("SP");
        r.phonemeLayer.boundaries.push_back(std::move(endP));

        r.groups.push_back({r.graphemeLayer.boundaries.back().id,
                            r.phonemeLayer.boundaries.back().id});

        DSFW_LOG_DEBUG("fa",
            ("[tier=1/phoneme] id=" + std::to_string(endP.id) +
             " END @ " + std::to_string(usToSec(lastEnd)) + "s").c_str());

        DSFW_LOG_INFO("fa",
            ("[binding#" + std::to_string(groupIdx++) +
             "] tier0.id=" + std::to_string(r.graphemeLayer.boundaries.back().id) +
             " ↔ tier1.id=" + std::to_string(r.phonemeLayer.boundaries.back().id) +
             " @ " + std::to_string(usToSec(lastEnd)) + "s (end)").c_str());
    }

    DSFW_LOG_INFO("fa",
        ("buildFaLayers: " +
         std::to_string(r.graphemeLayer.boundaries.size()) + " grapheme boundaries, " +
         std::to_string(r.phonemeLayer.boundaries.size()) + " phoneme boundaries, " +
         std::to_string(r.groups.size()) + " binding groups").c_str());

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

    shortcutManager()->bind(m_editor->saveAction(), kShortcutSave,
                            tr("Save"), tr("File"));
    shortcutManager()->bind(m_editor->undoAction(), kShortcutUndo,
                            tr("Undo"), tr("Edit"));
    shortcutManager()->bind(m_editor->redoAction(), kShortcutRedo,
                            tr("Redo"), tr("Edit"));
    shortcutManager()->bind(m_faAction, kShortcutFA,
                            tr("Force Align"), tr("Processing"));
    shortcutManager()->applyAll();
    shortcutManager()->updateTooltips();
    shortcutManager()->setEnabled(false);

    connect(m_editor->saveAction(), &QAction::triggered,
            this, [this]() { saveCurrentSlice(); updateDirtyIndicator(); });
    connect(m_editor->document(), &phonemelabeler::TextGridDocument::modifiedChanged,
            this, [this]() { updateDirtyIndicator(); });
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

    auto *doc = m_editor->document();

    auto result = source()->loadSlice(currentSliceId());
    DsTextDocument dstext;
    if (result)
        dstext = std::move(result.value());

    auto layers = doc->toDsText();
    dstext.layers.clear();
    for (const auto &layer : layers)
        dstext.layers.push_back(layer);

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
    if (!audioPath.isEmpty())
        m_editor->loadAudio(audioPath);

    auto result = source()->loadSlice(sliceId);
    if (result && !result.value().layers.empty()) {
        const auto &doc = result.value();
        QList<IntervalLayer> layers;
        for (const auto &layer : doc.layers)
            layers.append(layer);

        TimePos duration = 0;
        if (doc.audio.out > doc.audio.in)
            duration = doc.audio.out - doc.audio.in;

        // Use audio file duration as fallback if document doesn't specify
        if (duration <= 0) {
            double audioDur = m_editor->viewport()->totalDuration();
            if (audioDur > 0.0)
                duration = secToUs(audioDur);
        }

        if (duration > 0)
            m_editor->document()->loadFromDsText(layers, duration);
        else
            m_editor->document()->loadFromDsText(layers, secToUs(m_editor->viewport()->totalDuration()));

        // Restore or auto-detect binding groups
        if (!doc.groups.empty())
            m_editor->document()->setBindingGroups(doc.groups);
        else if (doc.layers.size() > 1)
            m_editor->document()->autoDetectBindingGroups();
    } else {
        // No layers — load empty document with audio duration
        double audioDur = m_editor->viewport()->totalDuration();
        TimePos duration = 0;
        if (result && result.value().audio.out > result.value().audio.in)
            duration = result.value().audio.out - result.value().audio.in;
        if (duration <= 0 && audioDur > 0.0)
            duration = secToUs(audioDur);
        if (duration > 0)
            m_editor->document()->loadFromDsText({}, duration);
    }

    // Sync viewport to document duration (after loadFromDsText set the TextGrid maxTime)
    double docDuration = usToSec(m_editor->document()->totalDuration());
    if (docDuration > 0.0) {
        // setTotalDuration() internally calls clampAndEmit(), which clamps the
        // view range to within the new total duration while preserving the current
        // view position — no need to force setViewRange(0.0, docDuration).
        m_editor->viewport()->setTotalDuration(docDuration);
    }
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

            // Use async engine loading so UI doesn't block during model init
            auto *tierLabel = m_editor->document() ?
                dynamic_cast<PhonemeTextGridTierLabel *>(
                    m_editor->findChild<PhonemeTextGridTierLabel *>()) : nullptr;

            ensureHfaEngineAsync([this, tierLabel]() {
                if (m_hfa && m_hfa->isOpen() && !m_faRunning) {
                    if (tierLabel)
                        tierLabel->setAlignmentRunning(true);
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
    disconnect(m_sliceLabelConn);
    disconnect(m_posLabelConn);

    auto *container = new QWidget(parent);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *sliceLabel = new QLabel(tr("No slice selected"), container);
    auto *posLabel = new QLabel(QStringLiteral("0.000s"), container);
    layout->addWidget(sliceLabel, 1);
    layout->addWidget(posLabel);

    m_sliceLabelConn = connect(this, &PhonemeLabelerPage::sliceChanged, sliceLabel,
                               [this, sliceLabel](const QString &id) {
        sliceLabel->setText(id.isEmpty() ? tr("No slice selected") : id);
    });
    QPointer<QLabel> posLabelGuard(posLabel);
    m_posLabelConn = connect(m_editor, &phonemelabeler::PhonemeEditor::positionChanged,
            this, [posLabelGuard](double sec) {
        if (posLabelGuard)
            posLabelGuard->setText(QString::number(sec, 'f', 3) + "s");
    });

    return container;
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

    QStringList graphemeTexts;
    for (const auto &layer : doc.layers) {
        if (layer.name == QStringLiteral("grapheme")) {
            for (const auto &b : layer.boundaries) {
                if (!b.text.isEmpty())
                    graphemeTexts << b.text;
            }
        }
    }

    if (graphemeTexts.isEmpty()) {
        DSFW_LOG_ERROR("fa", ("FA pipeline error [grapheme]: no grapheme layer data - " + sliceId.toStdString()).c_str());
        QMessageBox::information(this, tr("Force Align"),
                                 tr("Current slice has no lyrics data. Please label lyrics first."));
        return;
    }

    m_faRunning = true;
    DSFW_LOG_INFO("fa", ("FA started: " + sliceId.toStdString()).c_str());
    auto *hfa = m_hfa;
    auto hfaAlive = m_hfaAlive;
    QPointer<PhonemeLabelerPage> guard(this);

    QString lyricsQStr = graphemeTexts.join(QStringLiteral(" "));
    std::string lyricsText = lyricsQStr.toStdString();

    (void) QtConcurrent::run([hfa, hfaAlive, audioPath, lyricsText, sliceId, guard]() {
        if (!hfaAlive || !*hfaAlive)
            return;
        if (!hfa)
            return;
        HFA::WordList words;

        std::vector<std::string> nonSpeechPh = {"AP", "SP"};
        auto result = hfa->recognize(audioPath.toStdWString(), "zh", nonSpeechPh, lyricsText, words);

        if (!guard)
            return;

        QMetaObject::invokeMethod(guard.data(), [guard, sliceId, words = std::move(words), result = std::move(result)]() {
            if (!guard)
                return;
            guard->m_faRunning = false;

            auto *tierLabel = guard->m_editor->findChild<PhonemeTextGridTierLabel *>();
            if (tierLabel)
                tierLabel->setAlignmentRunning(false);

            if (result) {
                auto faResult = buildFaLayers(words);
                DSFW_LOG_INFO("fa", ("FA completed: " + sliceId.toStdString() + " - "
                              + std::to_string(faResult.phonemeLayer.boundaries.size()) + " phonemes, "
                              + std::to_string(faResult.groups.size()) + " bindings").c_str());

                QList<IntervalLayer> layers;
                layers.push_back(std::move(faResult.graphemeLayer));
                layers.push_back(std::move(faResult.phonemeLayer));

                guard->applyFaResult(sliceId, layers, faResult.groups);
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
                                        const std::vector<BindingGroup> &groups) {
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

    // Save binding groups from FA (grapheme↔phoneme boundary associations)
    if (!groups.empty())
        doc.groups = groups;

    (void) source()->saveSlice(sliceId, doc);

    if (sliceId == currentSliceId()) {
        QList<IntervalLayer> allLayers;
        for (const auto &layer : doc.layers)
            allLayers.append(layer);

        // Use document's audio duration, falling back to viewport's audio duration
        TimePos duration = 0;
        if (doc.audio.out > doc.audio.in)
            duration = doc.audio.out - doc.audio.in;
        if (duration <= 0) {
            double audioDur = m_editor->viewport()->totalDuration();
            if (audioDur > 0.0)
                duration = secToUs(audioDur);
        }

        if (duration > 0)
            m_editor->document()->loadFromDsText(allLayers, duration);

        // Restore binding groups from the saved document
        if (!doc.groups.empty())
            m_editor->document()->setBindingGroups(doc.groups);
        else if (doc.layers.size() > 1)
            m_editor->document()->autoDetectBindingGroups();
    }
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

    m_faRunning = true;
    {
        std::string msg = "Batch FA started: " + std::to_string(ids.size()) + " slices";
        DSFW_LOG_INFO("fa", msg.c_str());
    }
    auto *hfa = m_hfa;
    auto hfaAlive = m_hfaAlive;
    auto *src = source();
    QPointer<PhonemeLabelerPage> guard(this);

    (void) QtConcurrent::run([hfa, hfaAlive, src, ids, guard]() {
        if (!hfaAlive || !*hfaAlive)
            return;
        if (!hfa)
            return;
        int processed = 0;
        int skipped = 0;
        for (const auto &sliceId : ids) {
            if (!*hfaAlive)
                break;
            QString audioPath = src->validatedAudioPath(sliceId);
            if (audioPath.isEmpty()) {
                ++skipped;
                continue;
            }

            auto loadResult = src->loadSlice(sliceId);
            if (!loadResult)
                continue;

            const auto &doc = loadResult.value();
            QStringList graphemeTexts;
            for (const auto &layer : doc.layers) {
                if (layer.name == QStringLiteral("grapheme")) {
                    for (const auto &b : layer.boundaries) {
                        if (!b.text.isEmpty())
                            graphemeTexts << b.text;
                    }
                }
            }
            if (graphemeTexts.isEmpty())
                continue;

            HFA::WordList words;
            for (const auto &text : graphemeTexts) {
                HFA::Word word;
                word.text = text.toStdString();
                HFA::Phone p;
                p.text = text.toStdString();
                word.phones.push_back(p);
                words.push_back(word);
            }

            std::vector<std::string> nonSpeechPh = {"AP", "SP"};
            auto result = hfa->recognize(audioPath.toStdWString(), "zh", nonSpeechPh, words);
            if (result) {
                auto faResult = buildFaLayers(words);

                QList<IntervalLayer> layers;
                layers.push_back(std::move(faResult.graphemeLayer));
                layers.push_back(std::move(faResult.phonemeLayer));

                QMetaObject::invokeMethod(guard.data(), [guard, sliceId, layers, groups = std::move(faResult.groups)]() {
                    if (guard)
                        guard->applyFaResult(sliceId, layers, groups);
                }, Qt::QueuedConnection);
            }
            ++processed;
        }
        QMetaObject::invokeMethod(guard.data(), [guard, processed, skipped]() {
            if (!guard)
                return;
            guard->m_faRunning = false;
            DSFW_LOG_INFO("fa", ("Batch FA completed: " + std::to_string(processed)
                          + " processed, " + std::to_string(skipped) + " skipped").c_str());
            QString msg = tr("Batch force align completed: %1 slices").arg(processed);
            if (skipped > 0)
                msg += tr(", %1 skipped (missing audio)").arg(skipped);
            dsfw::widgets::ToastNotification::show(
                guard.data(), dsfw::widgets::ToastType::Info,
                msg, 3000);
        }, Qt::QueuedConnection);
    });
}

} // namespace dstools
