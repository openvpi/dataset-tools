#include "PhonemeLabelerPage.h"
#include "SliceListPanel.h"
#include "ModelConfigHelper.h"
#include "ISettingsBackend.h"

#include <dstools/IEditorDataSource.h>

#include <QHBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QPointer>
#include <QtConcurrent>

#include <hubert-infer/Hfa.h>

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

PhonemeLabelerPage::PhonemeLabelerPage(QWidget *parent)
    : QWidget(parent), m_settings("PhonemeLabeler") {
    m_editor = new phonemelabeler::PhonemeEditor(this);
    m_sliceList = new SliceListPanel(this);
    m_sliceList->setMinimumWidth(160);
    m_sliceList->setMaximumWidth(280);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_sliceList);
    m_splitter->addWidget(m_editor);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);

    m_prevAction = new QAction(QStringLiteral("上一个切片"), this);
    m_nextAction = new QAction(QStringLiteral("下一个切片"), this);
    m_faAction = new QAction(QStringLiteral("强制对齐当前切片"), this);
    connect(m_faAction, &QAction::triggered, this, &PhonemeLabelerPage::onRunFA);

    static const dstools::SettingsKey<QString> kShortcutSave("Shortcuts/save", "Ctrl+S");
    static const dstools::SettingsKey<QString> kShortcutUndo("Shortcuts/undo", "Ctrl+Z");
    static const dstools::SettingsKey<QString> kShortcutRedo("Shortcuts/redo", "Ctrl+Y");
    static const dstools::SettingsKey<QString> kShortcutFA("Shortcuts/fa", "F");
    static const dstools::SettingsKey<QString> kLastSlice("State/lastSlice", "");
    static const dstools::SettingsKey<QString> kSplitterState("Layout/splitterState", "");

    m_shortcutManager = new dstools::widgets::ShortcutManager(&m_settings, this);
    m_shortcutManager->bind(m_prevAction, dsfw::CommonKeys::NavigationPrev,
                            QStringLiteral("上一个切片"), QStringLiteral("导航"));
    m_shortcutManager->bind(m_nextAction, dsfw::CommonKeys::NavigationNext,
                            QStringLiteral("下一个切片"), QStringLiteral("导航"));
    m_shortcutManager->bind(m_editor->saveAction(), kShortcutSave,
                            QStringLiteral("保存"), QStringLiteral("文件"));
    m_shortcutManager->bind(m_editor->undoAction(), kShortcutUndo,
                            QStringLiteral("撤销"), QStringLiteral("编辑"));
    m_shortcutManager->bind(m_editor->redoAction(), kShortcutRedo,
                            QStringLiteral("重做"), QStringLiteral("编辑"));
    m_shortcutManager->bind(m_faAction, kShortcutFA,
                            QStringLiteral("强制对齐"), QStringLiteral("处理"));
    m_shortcutManager->applyAll();

    connect(m_sliceList, &SliceListPanel::sliceSelected,
            this, &PhonemeLabelerPage::onSliceSelected);
    connect(m_editor->saveAction(), &QAction::triggered,
            this, [this]() { saveCurrentSlice(); });
    connect(m_prevAction, &QAction::triggered, this, [this]() {
        m_sliceList->selectPrev();
    });
    connect(m_nextAction, &QAction::triggered, this, [this]() {
        m_sliceList->selectNext();
    });
}

PhonemeLabelerPage::~PhonemeLabelerPage() = default;

void PhonemeLabelerPage::setDataSource(IEditorDataSource *source,
                                        ISettingsBackend *settingsBackend) {
    m_source = source;
    m_settingsBackend = settingsBackend;
    m_sliceList->setDataSource(source);
}

void PhonemeLabelerPage::onSliceSelected(const QString &sliceId) {
    if (!maybeSave())
        return;

    m_currentSliceId = sliceId;
    static const dstools::SettingsKey<QString> kLastSlice("State/lastSlice", "");
    m_settings.set(kLastSlice, sliceId);

    if (!m_source)
        return;

    const QString audioPath = m_source->audioPath(sliceId);

    if (!audioPath.isEmpty() && QFile::exists(audioPath))
        m_editor->loadAudio(audioPath);
    else if (!audioPath.isEmpty()) {
        // Audio file doesn't exist — clear editor to avoid stale data / 0-sample errors
        m_editor->loadAudio(QString());
        dsfw::widgets::ToastNotification::show(
            this, dsfw::widgets::ToastType::Warning,
            QStringLiteral("音频文件不存在: %1\n请返回切片页面重新导出。")
                .arg(QFileInfo(audioPath).fileName()),
            5000);
    }

    auto result = m_source->loadSlice(sliceId);
    if (result && !result.value().layers.empty()) {
        const auto &doc = result.value();
        QList<IntervalLayer> layers;
        for (const auto &layer : doc.layers)
            layers.append(layer);

        TimePos duration = m_editor->document()->totalDuration();
        if (doc.audio.out > doc.audio.in)
            duration = doc.audio.out - doc.audio.in;

        m_editor->document()->loadFromDsText(layers, duration);
    } else {
        // No layers for this slice — clear the document to avoid showing stale data
        TimePos duration = m_editor->document()->totalDuration();
        if (result && result.value().audio.out > result.value().audio.in)
            duration = result.value().audio.out - result.value().audio.in;
        m_editor->document()->loadFromDsText({}, duration);
    }

    emit sliceChanged(sliceId);
}

bool PhonemeLabelerPage::saveCurrentSlice() {
    if (m_currentSliceId.isEmpty() || !m_source)
        return true;

    if (!m_editor->document()->isModified())
        return true;

    auto *doc = m_editor->document();

    auto result = m_source->loadSlice(m_currentSliceId);
    DsTextDocument dstext;
    if (result)
        dstext = std::move(result.value());

    auto layers = doc->toDsText();
    dstext.layers.clear();
    for (const auto &layer : layers)
        dstext.layers.push_back(layer);

    auto saveResult = m_source->saveSlice(m_currentSliceId, dstext);
    if (!saveResult) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                             QString::fromStdString(saveResult.error()));
        return false;
    }

    return true;
}

bool PhonemeLabelerPage::maybeSave() {
    if (!m_editor->document() || !m_editor->document()->isModified())
        return true;

    auto ret = QMessageBox::question(
        this, QStringLiteral("未保存的更改"),
        QStringLiteral("当前切片已修改，是否保存？"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
        return saveCurrentSlice();
    if (ret == QMessageBox::Discard)
        return true;
    return false;
}

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
    editMenu->addAction(m_prevAction);
    editMenu->addAction(m_nextAction);
    editMenu->addSeparator();
    {
        auto *shortcutAction = editMenu->addAction(QStringLiteral("快捷键设置..."));
        connect(shortcutAction, &QAction::triggered, this, [this]() {
            m_shortcutManager->showEditor(this);
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

QString PhonemeLabelerPage::windowTitle() const {
    QString title = QStringLiteral("音素标注");
    if (!m_currentSliceId.isEmpty())
        title += QStringLiteral(" — ") + m_currentSliceId;
    return title;
}

bool PhonemeLabelerPage::hasUnsavedChanges() const {
    return m_editor->document() && m_editor->document()->isModified();
}

void PhonemeLabelerPage::onActivated() {
    m_sliceList->refresh();

    {
        static const dstools::SettingsKey<QString> kSplitterState("Layout/splitterState", "");
        auto state = m_settings.get(kSplitterState);
        if (!state.isEmpty())
            m_splitter->restoreState(QByteArray::fromBase64(state.toUtf8()));
    }
    {
        static const dstools::SettingsKey<QString> kEditorSplitterState("Layout/editorSplitterState", "");
        auto state = m_settings.get(kEditorSplitterState);
        if (!state.isEmpty())
            m_editor->restoreSplitterState(QByteArray::fromBase64(state.toUtf8()));
    }

    if (m_settingsBackend) {
        auto settingsData = m_settingsBackend->load();
        auto preload = settingsData["preload"].toObject();
        auto faPreload = preload["phoneme_alignment"].toObject();
        if (faPreload["enabled"].toBool(false)) {
            ensureHfaEngine();
        }
    }

    static const dstools::SettingsKey<QString> kLastSlice("State/lastSlice", "");
    if (m_currentSliceId.isEmpty()) {
        QString lastSlice = m_settings.get(kLastSlice);
        if (!lastSlice.isEmpty()) {
            m_sliceList->setCurrentSlice(lastSlice);
        }
        // If m_currentSliceId is still empty after trying lastSlice
        // (e.g. lastSlice was empty, or setCurrentSlice didn't trigger
        // onSliceSelected because the row was already selected),
        // explicitly select the first slice.
        if (m_currentSliceId.isEmpty() && m_sliceList->sliceCount() > 0) {
            QString firstId = m_sliceList->currentSliceId();
            if (!firstId.isEmpty())
                onSliceSelected(firstId);
        }
    }

    if (m_source && !m_currentSliceId.isEmpty()) {
        auto dirty = m_source->dirtyLayers(m_currentSliceId);

        bool needAutoFA = dirty.contains(QStringLiteral("phoneme"));

        if (!needAutoFA && !m_currentSliceId.isEmpty()) {
            auto result = m_source->loadSlice(m_currentSliceId);
            if (result) {
                bool hasGrapheme = false;
                bool hasPhoneme = false;
                for (const auto &layer : result.value().layers) {
                    if (layer.name == QStringLiteral("grapheme"))
                        hasGrapheme = true;
                    if (layer.name == QStringLiteral("phoneme") && !layer.boundaries.empty())
                        hasPhoneme = true;
                }
                if (hasGrapheme && !hasPhoneme)
                    needAutoFA = true;
            }
        }

        if (needAutoFA) {
            m_source->clearDirtyLayers(m_currentSliceId, {QStringLiteral("phoneme")});

            ensureHfaEngine();
            if (m_hfa && m_hfa->isOpen() && !m_faRunning) {
                dsfw::widgets::ToastNotification::show(
                    this, dsfw::widgets::ToastType::Info,
                    QStringLiteral("自动运行强制对齐..."), 3000);
                runFaForSlice(m_currentSliceId);
            } else {
                dsfw::widgets::ToastNotification::show(
                    this, dsfw::widgets::ToastType::Warning,
                    QStringLiteral("音素层数据已过期，请手动运行强制对齐"),
                    3000);
            }
        }
    }
}

bool PhonemeLabelerPage::onDeactivating() {
    {
        static const dstools::SettingsKey<QString> kSplitterState("Layout/splitterState", "");
        m_settings.set(kSplitterState, QString::fromLatin1(m_splitter->saveState().toBase64()));
    }
    {
        static const dstools::SettingsKey<QString> kEditorSplitterState("Layout/editorSplitterState", "");
        m_settings.set(kEditorSplitterState, QString::fromLatin1(m_editor->saveSplitterState().toBase64()));
    }
    return maybeSave();
}

void PhonemeLabelerPage::onDeactivated() {
    m_hfa = nullptr;
}

void PhonemeLabelerPage::onShutdown() {
    m_shortcutManager->saveAll();
}

dstools::widgets::ShortcutManager *PhonemeLabelerPage::shortcutManager() const {
    return m_shortcutManager;
}

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

    auto config = readModelConfig(m_settingsBackend, QStringLiteral("phoneme_alignment"));
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

void PhonemeLabelerPage::onRunFA() {
    if (m_currentSliceId.isEmpty()) {
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

    runFaForSlice(m_currentSliceId);
}

void PhonemeLabelerPage::runFaForSlice(const QString &sliceId) {
    if (!m_source)
        return;

    QString audioPath = m_source->audioPath(sliceId);
    if (audioPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("强制对齐"),
                             QStringLiteral("当前切片没有音频文件。"));
        return;
    }
    if (!QFile::exists(audioPath)) {
        QMessageBox::warning(this, QStringLiteral("强制对齐"),
                             QStringLiteral("音频文件不存在: %1\n请返回切片页面重新导出。")
                                 .arg(audioPath));
        return;
    }

    auto loadResult = m_source->loadSlice(sliceId);
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

    // Build lyrics text from grapheme layer for G2P
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
            if (result) {
                QList<IntervalLayer> layers;

                IntervalLayer phonemeLayer;
                phonemeLayer.name = QStringLiteral("phoneme");
                phonemeLayer.type = QStringLiteral("interval");
                int id = 1;
                for (const auto &word : words) {
                    for (const auto &phone : word.phones) {
                        Boundary b;
                        b.id = id++;
                        b.pos = secToUs(phone.start);
                        b.text = QString::fromStdString(phone.text);
                        phonemeLayer.boundaries.push_back(std::move(b));
                    }
                }
                if (!phonemeLayer.boundaries.empty()) {
                    Boundary endB;
                    endB.id = id;
                    endB.pos = secToUs(words.back().phones.back().end);
                    endB.text = QString();
                    phonemeLayer.boundaries.push_back(std::move(endB));
                }
                layers.push_back(std::move(phonemeLayer));

                guard->applyFaResult(sliceId, layers);
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
                                        const QList<IntervalLayer> &layers) {
    if (!m_source)
        return;

    auto result = m_source->loadSlice(sliceId);
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

    (void) m_source->saveSlice(sliceId, doc);

    if (sliceId == m_currentSliceId) {
        QList<IntervalLayer> allLayers;
        for (const auto &layer : doc.layers)
            allLayers.append(layer);

        TimePos duration = m_editor->document()->totalDuration();
        if (doc.audio.out > doc.audio.in)
            duration = doc.audio.out - doc.audio.in;

        m_editor->document()->loadFromDsText(allLayers, duration);
    }
}

void PhonemeLabelerPage::onBatchFA() {
    if (!m_source) {
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

    const auto ids = m_source->sliceIds();
    if (ids.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("批量强制对齐"),
                                 QStringLiteral("没有可处理的切片。"));
        return;
    }

    m_faRunning = true;
    auto *hfa = m_hfa;
    auto *source = m_source;
    QPointer<PhonemeLabelerPage> guard(this);

    (void) QtConcurrent::run([hfa, source, ids, guard]() {
        int processed = 0;
        for (const auto &sliceId : ids) {
            QString audioPath = source->audioPath(sliceId);
            if (audioPath.isEmpty())
                continue;

            auto loadResult = source->loadSlice(sliceId);
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
                QList<IntervalLayer> layers;
                IntervalLayer phonemeLayer;
                phonemeLayer.name = QStringLiteral("phoneme");
                phonemeLayer.type = QStringLiteral("interval");
                int id = 1;
                for (const auto &word : words) {
                    for (const auto &phone : word.phones) {
                        Boundary b;
                        b.id = id++;
                        b.pos = secToUs(phone.start);
                        b.text = QString::fromStdString(phone.text);
                        phonemeLayer.boundaries.push_back(std::move(b));
                    }
                }
                if (!phonemeLayer.boundaries.empty()) {
                    Boundary endB;
                    endB.id = id;
                    endB.pos = secToUs(words.back().phones.back().end);
                    endB.text = QString();
                    phonemeLayer.boundaries.push_back(std::move(endB));
                }
                layers.push_back(std::move(phonemeLayer));

                QMetaObject::invokeMethod(guard.data(), [guard, sliceId, layers]() {
                    if (guard)
                        guard->applyFaResult(sliceId, layers);
                }, Qt::QueuedConnection);
            }
            ++processed;
        }
        QMetaObject::invokeMethod(guard.data(), [guard, processed]() {
            if (!guard)
                return;
            guard->m_faRunning = false;
            dsfw::widgets::ToastNotification::show(
                guard.data(), dsfw::widgets::ToastType::Info,
                QStringLiteral("批量强制对齐完成: %1 个切片").arg(processed), 3000);
        }, Qt::QueuedConnection);
    });
}

} // namespace dstools
