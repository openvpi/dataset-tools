#include "PhonemeLabelerPage.h"
#include "SliceListPanel.h"
#include "ModelConfigHelper.h"
#include "ISettingsBackend.h"

#include <dstools/IEditorDataSource.h>

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

    // Include all words and phones from FA output (including SP/AP).
    // SP/AP are part of the alignment result and should be preserved.
    for (const auto &word : words) {
        if (word.phones.empty())
            continue;

        // Grapheme boundary at word start
        Boundary graphemeB;
        graphemeB.id = nextId++;
        graphemeB.pos = secToUs(word.start);
        graphemeB.text = QString::fromStdString(word.text);
        int graphemeBId = graphemeB.id;
        r.graphemeLayer.boundaries.push_back(std::move(graphemeB));

        // Phoneme boundaries for each phone in this word
        for (size_t pi = 0; pi < word.phones.size(); ++pi) {
            const auto &phone = word.phones[pi];

            Boundary phoneB;
            phoneB.id = nextId++;
            phoneB.pos = secToUs(phone.start);
            phoneB.text = QString::fromStdString(phone.text);

            // The first phone of each word shares its start time with the word
            // → create a BindingGroup linking grapheme boundary ↔ phoneme boundary
            if (pi == 0) {
                r.groups.push_back({graphemeBId, phoneB.id});
            }

            r.phonemeLayer.boundaries.push_back(std::move(phoneB));
        }
    }

    // Add end boundaries (closing the last interval)
    if (!r.graphemeLayer.boundaries.empty() && !words.empty()) {
        Boundary endG;
        endG.id = nextId++;
        endG.pos = secToUs(words.back().end);
        r.graphemeLayer.boundaries.push_back(std::move(endG));
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
        r.phonemeLayer.boundaries.push_back(std::move(endP));

        // End boundaries also form a binding group
        r.groups.push_back({r.graphemeLayer.boundaries.back().id,
                            r.phonemeLayer.boundaries.back().id});
    }

    return r;
}

PhonemeLabelerPage::PhonemeLabelerPage(QWidget *parent)
    : EditorPageBase("PhonemeLabeler", parent) {
    m_editor = new phonemelabeler::PhonemeEditor(this);

    setupBaseLayout(m_editor);
    setupNavigationActions();

    m_faAction = new QAction(QStringLiteral("强制对齐当前切片"), this);
    connect(m_faAction, &QAction::triggered, this, &PhonemeLabelerPage::onRunFA);

    // Add FA button to the editor toolbar
    m_editor->toolbar()->addAction(m_faAction);

    static const dstools::SettingsKey<QString> kShortcutSave("Shortcuts/save", "Ctrl+S");
    static const dstools::SettingsKey<QString> kShortcutUndo("Shortcuts/undo", "Ctrl+Z");
    static const dstools::SettingsKey<QString> kShortcutRedo("Shortcuts/redo", "Ctrl+Y");
    static const dstools::SettingsKey<QString> kShortcutFA("Shortcuts/fa", "F");

    shortcutManager()->bind(m_editor->saveAction(), kShortcutSave,
                            QStringLiteral("保存"), QStringLiteral("文件"));
    shortcutManager()->bind(m_editor->undoAction(), kShortcutUndo,
                            QStringLiteral("撤销"), QStringLiteral("编辑"));
    shortcutManager()->bind(m_editor->redoAction(), kShortcutRedo,
                            QStringLiteral("重做"), QStringLiteral("编辑"));
    shortcutManager()->bind(m_faAction, kShortcutFA,
                            QStringLiteral("强制对齐"), QStringLiteral("处理"));
    shortcutManager()->applyAll();
    shortcutManager()->updateTooltips();
    shortcutManager()->setEnabled(false);

    connect(m_editor->saveAction(), &QAction::triggered,
            this, [this]() { saveCurrentSlice(); });
}

PhonemeLabelerPage::~PhonemeLabelerPage() = default;

// ── EditorPageBase hooks ──────────────────────────────────────────────────────

QString PhonemeLabelerPage::windowTitlePrefix() const {
    return QStringLiteral("音素标注");
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
        QMessageBox::warning(this, QStringLiteral("保存失败"),
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
    m_hfa = nullptr;
}

void PhonemeLabelerPage::restoreExtraSplitters() {
    static const dstools::SettingsKey<QString> kEditorSplitterState("Layout/editorSplitterState", "");
    auto state = settings().get(kEditorSplitterState);
    if (!state.isEmpty())
        m_editor->restoreSplitterState(QByteArray::fromBase64(state.toUtf8()));
}

void PhonemeLabelerPage::saveExtraSplitters() {
    static const dstools::SettingsKey<QString> kEditorSplitterState("Layout/editorSplitterState", "");
    settings().set(kEditorSplitterState, QString::fromLatin1(m_editor->saveSplitterState().toBase64()));
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
                        QStringLiteral("自动运行强制对齐..."), 3000);
                    runFaForSlice(currentSliceId());
                } else {
                    dsfw::widgets::ToastNotification::show(
                        this, dsfw::widgets::ToastType::Warning,
                        QStringLiteral("音素层数据已过期，请手动运行强制对齐"),
                        3000);
                }
            });
        }
    }
}

// ── IPageActions ──────────────────────────────────────────────────────────────

QMenuBar *PhonemeLabelerPage::createMenuBar(QWidget *parent) {
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
    processMenu->addAction(m_faAction);
    processMenu->addSeparator();
    processMenu->addAction(QStringLiteral("批量强制对齐..."), this, &PhonemeLabelerPage::onBatchFA);

    auto *viewMenu = bar->addMenu(QStringLiteral("视图(&V)"));
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
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *sliceLabel = new QLabel(QStringLiteral("未选择切片"), container);
    auto *posLabel = new QLabel(QStringLiteral("0.000s"), container);
    layout->addWidget(sliceLabel, 1);
    layout->addWidget(posLabel);

    connect(this, &PhonemeLabelerPage::sliceChanged, sliceLabel, [sliceLabel](const QString &id) {
        sliceLabel->setText(id.isEmpty() ? QStringLiteral("未选择切片") : id);
    });
    connect(m_editor, &phonemelabeler::PhonemeEditor::positionChanged,
            this, [posLabel](double sec) {
        posLabel->setText(QString::number(sec, 'f', 3) + "s");
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
    if (hfaProvider && hfaProvider->engine().isOpen())
        m_hfa = &hfaProvider->engine();
}

void PhonemeLabelerPage::onModelInvalidated(const QString &taskKey) {
    if (taskKey == QStringLiteral("phoneme_alignment"))
        m_hfa = nullptr;
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
        QMessageBox::information(this, QStringLiteral("强制对齐"),
                                 QStringLiteral("请先选择一个切片。"));
        return;
    }

    ensureHfaEngine();
    if (!m_hfa || !m_hfa->isOpen()) {
        QMessageBox::warning(this, QStringLiteral("强制对齐"),
                             QStringLiteral("强制对齐模型未加载。请在设置中配置 HuBERT-FA 模型路径。"));
        return;
    }

    if (m_faRunning) {
        QMessageBox::information(this, QStringLiteral("强制对齐"),
                                 QStringLiteral("强制对齐正在运行中，请稍候。"));
        return;
    }

    runFaForSlice(currentSliceId());
}

void PhonemeLabelerPage::runFaForSlice(const QString &sliceId) {
    if (!source())
        return;

    QString audioPath = source()->validatedAudioPath(sliceId);
    if (audioPath.isEmpty()) {
        if (source()->audioPath(sliceId).isEmpty())
            QMessageBox::warning(this, QStringLiteral("强制对齐"),
                                 QStringLiteral("当前切片没有音频文件。"));
        return;
    }

    auto loadResult = source()->loadSlice(sliceId);
    if (!loadResult) {
        QMessageBox::warning(this, QStringLiteral("强制对齐"),
                             QStringLiteral("无法加载切片数据。"));
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
        QMessageBox::information(this, QStringLiteral("强制对齐"),
                                 QStringLiteral("当前切片没有歌词数据，请先在歌词页标注。"));
        return;
    }

    m_faRunning = true;
    auto *hfa = m_hfa;
    QPointer<PhonemeLabelerPage> guard(this);

    QString lyricsQStr = graphemeTexts.join(QStringLiteral(" "));
    std::string lyricsText = lyricsQStr.toStdString();

    (void) QtConcurrent::run([hfa, audioPath, lyricsText, sliceId, guard]() {
        HFA::WordList words;

        std::vector<std::string> nonSpeechPh = {"AP", "SP"};
        auto result = hfa->recognize(audioPath.toStdWString(), "zh", nonSpeechPh, lyricsText, words);

        if (!guard)
            return;

        QMetaObject::invokeMethod(guard.data(), [guard, sliceId, words = std::move(words), result = std::move(result)]() {
            if (!guard)
                return;
            guard->m_faRunning = false;

            // Clear alignment indicator
            auto *tierLabel = guard->m_editor->findChild<PhonemeTextGridTierLabel *>();
            if (tierLabel)
                tierLabel->setAlignmentRunning(false);

            if (result) {
                auto faResult = buildFaLayers(words);

                QList<IntervalLayer> layers;
                layers.push_back(std::move(faResult.graphemeLayer));
                layers.push_back(std::move(faResult.phonemeLayer));

                guard->applyFaResult(sliceId, layers, faResult.groups);
                dsfw::widgets::ToastNotification::show(
                    guard.data(), dsfw::widgets::ToastType::Info,
                    QStringLiteral("强制对齐完成"), 3000);
            } else {
                QMessageBox::warning(guard.data(), QStringLiteral("强制对齐"),
                                     QStringLiteral("强制对齐失败: %1")
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
        QMessageBox::warning(this, QStringLiteral("批量强制对齐"),
                             QStringLiteral("请先打开工程。"));
        return;
    }

    ensureHfaEngine();
    if (!m_hfa || !m_hfa->isOpen()) {
        QMessageBox::warning(this, QStringLiteral("批量强制对齐"),
                             QStringLiteral("强制对齐模型未加载。请在设置中配置 HuBERT-FA 模型路径。"));
        return;
    }

    if (m_faRunning) {
        QMessageBox::information(this, QStringLiteral("批量强制对齐"),
                                 QStringLiteral("强制对齐正在运行中，请稍候。"));
        return;
    }

    const auto ids = source()->sliceIds();
    if (ids.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("批量强制对齐"),
                                 QStringLiteral("没有可处理的切片。"));
        return;
    }

    m_faRunning = true;
    auto *hfa = m_hfa;
    auto *src = source();
    QPointer<PhonemeLabelerPage> guard(this);

    (void) QtConcurrent::run([hfa, src, ids, guard]() {
        int processed = 0;
        int skipped = 0;
        for (const auto &sliceId : ids) {
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
            QString msg = QStringLiteral("批量强制对齐完成: %1 个切片").arg(processed);
            if (skipped > 0)
                msg += QStringLiteral("，%1 个音频文件缺失已跳过").arg(skipped);
            dsfw::widgets::ToastNotification::show(
                guard.data(), dsfw::widgets::ToastType::Info,
                msg, 3000);
        }, Qt::QueuedConnection);
    });
}

} // namespace dstools
