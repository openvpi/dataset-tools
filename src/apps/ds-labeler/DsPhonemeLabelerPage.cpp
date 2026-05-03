#include "DsPhonemeLabelerPage.h"
#include "DsLabelerKeys.h"
#include "ProjectDataSource.h"
#include "SliceListPanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QtConcurrent>

#include <hubert-infer/Hfa.h>

#include <dsfw/PipelineContext.h>
#include <dsfw/Theme.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dstools/DsProject.h>
#include <dstools/DsTextTypes.h>
#include <dstools/ExecutionProvider.h>
#include <ui/TextGridDocument.h>

namespace dstools {

DsPhonemeLabelerPage::DsPhonemeLabelerPage(QWidget *parent)
    : QWidget(parent), m_settings("DsLabeler/PhonemeLabeler") {
    m_editor = new phonemelabeler::PhonemeEditor(this);
    m_sliceList = new SliceListPanel(this);
    m_sliceList->setMinimumWidth(160);
    m_sliceList->setMaximumWidth(280);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_sliceList);
    splitter->addWidget(m_editor);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);

    m_prevAction = new QAction(QStringLiteral("上一个切片"), this);
    m_nextAction = new QAction(QStringLiteral("下一个切片"), this);

    m_shortcutManager = new dstools::widgets::ShortcutManager(&m_settings, this);
    m_shortcutManager->bind(m_prevAction, DsLabelerKeys::NavigationPrev,
                            QStringLiteral("上一个切片"), QStringLiteral("导航"));
    m_shortcutManager->bind(m_nextAction, DsLabelerKeys::NavigationNext,
                            QStringLiteral("下一个切片"), QStringLiteral("导航"));
    m_shortcutManager->bind(m_editor->saveAction(), DsLabelerKeys::ShortcutSave,
                            QStringLiteral("保存"), QStringLiteral("文件"));
    m_shortcutManager->bind(m_editor->undoAction(), DsLabelerKeys::ShortcutUndo,
                            QStringLiteral("撤销"), QStringLiteral("编辑"));
    m_shortcutManager->bind(m_editor->redoAction(), DsLabelerKeys::ShortcutRedo,
                            QStringLiteral("重做"), QStringLiteral("编辑"));
    m_shortcutManager->applyAll();

    connect(m_sliceList, &SliceListPanel::sliceSelected,
            this, &DsPhonemeLabelerPage::onSliceSelected);
    connect(m_editor->saveAction(), &QAction::triggered,
            this, [this]() { saveCurrentSlice(); });
    connect(m_prevAction, &QAction::triggered, this, [this]() {
        m_sliceList->selectPrev();
    });
    connect(m_nextAction, &QAction::triggered, this, [this]() {
        m_sliceList->selectNext();
    });
}

DsPhonemeLabelerPage::~DsPhonemeLabelerPage() = default;

void DsPhonemeLabelerPage::setDataSource(ProjectDataSource *source) {
    m_source = source;
    m_sliceList->setDataSource(source);
}

void DsPhonemeLabelerPage::onSliceSelected(const QString &sliceId) {
    if (!maybeSave())
        return;

    m_currentSliceId = sliceId;

    if (!m_source)
        return;

    const QString audioPath = m_source->audioPath(sliceId);

    if (!audioPath.isEmpty())
        m_editor->loadAudio(audioPath);

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
    }

    emit sliceChanged(sliceId);
}

bool DsPhonemeLabelerPage::saveCurrentSlice() {
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

bool DsPhonemeLabelerPage::maybeSave() {
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

QMenuBar *DsPhonemeLabelerPage::createMenuBar(QWidget *parent) {
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
    processMenu->addAction(QStringLiteral("强制对齐当前切片"), this, &DsPhonemeLabelerPage::onRunFA);
    processMenu->addSeparator();
    processMenu->addAction(QStringLiteral("批量强制对齐..."), this, &DsPhonemeLabelerPage::onBatchFA);

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

QWidget *DsPhonemeLabelerPage::createStatusBarContent(QWidget *parent) {
    auto *container = new QWidget(parent);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *sliceLabel = new QLabel(QStringLiteral("未选择切片"), container);
    auto *posLabel = new QLabel(QStringLiteral("0.000s"), container);
    layout->addWidget(sliceLabel, 1);
    layout->addWidget(posLabel);

    connect(this, &DsPhonemeLabelerPage::sliceChanged, sliceLabel, [sliceLabel](const QString &id) {
        sliceLabel->setText(id.isEmpty() ? QStringLiteral("未选择切片") : id);
    });
    connect(m_editor, &phonemelabeler::PhonemeEditor::positionChanged,
            this, [posLabel](double sec) {
        posLabel->setText(QString::number(sec, 'f', 3) + "s");
    });

    return container;
}

QString DsPhonemeLabelerPage::windowTitle() const {
    QString title = QStringLiteral("DsLabeler — 音素标注");
    if (!m_currentSliceId.isEmpty())
        title += QStringLiteral(" — ") + m_currentSliceId;
    return title;
}

bool DsPhonemeLabelerPage::hasUnsavedChanges() const {
    return m_editor->document() && m_editor->document()->isModified();
}

void DsPhonemeLabelerPage::onActivated() {
    m_sliceList->refresh();

    if (m_source && !m_currentSliceId.isEmpty()) {
        auto *ctx = m_source->context(m_currentSliceId);
        if (ctx && ctx->dirty.contains(QStringLiteral("phoneme"))) {
            dsfw::widgets::ToastNotification::show(
                this, dsfw::widgets::ToastType::Warning,
                QStringLiteral("音素层数据已过期（上游歌词已修改），建议重新对齐"),
                3000);
            ctx->dirty.removeAll(QStringLiteral("phoneme"));
            m_source->saveContext(m_currentSliceId);
        }
    }
}

bool DsPhonemeLabelerPage::onDeactivating() {
    return maybeSave();
}

void DsPhonemeLabelerPage::onShutdown() {
    m_shortcutManager->saveAll();
}

dstools::widgets::ShortcutManager *DsPhonemeLabelerPage::shortcutManager() const {
    return m_shortcutManager;
}

void DsPhonemeLabelerPage::ensureHfaEngine() {
    if (m_hfa && m_hfa->isOpen())
        return;

    if (!m_source || !m_source->project())
        return;

    auto it = m_source->project()->defaults().taskModels.find(QStringLiteral("phoneme_alignment"));
    if (it == m_source->project()->defaults().taskModels.end())
        return;

    const auto &config = it->second;
    if (config.modelPath.isEmpty())
        return;

    auto provider = dstools::infer::ExecutionProvider::CPU;
#ifdef ONNXRUNTIME_ENABLE_DML
    if (config.provider == QStringLiteral("dml"))
        provider = dstools::infer::ExecutionProvider::DML;
#endif

    m_hfa = std::make_unique<HFA::HFA>();
    auto result = m_hfa->load(config.modelPath.toStdWString(), provider, config.deviceId);
    if (!result) {
        m_hfa.reset();
    }
}

void DsPhonemeLabelerPage::onRunFA() {
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

void DsPhonemeLabelerPage::runFaForSlice(const QString &sliceId) {
    if (!m_source)
        return;

    QString audioPath = m_source->audioPath(sliceId);
    if (audioPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("强制对齐"),
                             QStringLiteral("当前切片没有音频文件。"));
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
    auto *hfa = m_hfa.get();
    auto *source = m_source;

    (void) QtConcurrent::run([hfa, audioPath, graphemeTexts, sliceId, source, this]() {
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

        QMetaObject::invokeMethod(this, [this, sliceId, words = std::move(words), result = std::move(result)]() {
            m_faRunning = false;
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
                        b.pos = phone.start;
                        b.text = QString::fromStdString(phone.text);
                        phonemeLayer.boundaries.push_back(std::move(b));
                    }
                }
                if (!phonemeLayer.boundaries.empty()) {
                    Boundary endB;
                    endB.id = id;
                    endB.pos = words.back().phones.back().end;
                    endB.text = QString();
                    phonemeLayer.boundaries.push_back(std::move(endB));
                }
                layers.push_back(std::move(phonemeLayer));

                applyFaResult(sliceId, layers);
                dsfw::widgets::ToastNotification::show(
                    this, dsfw::widgets::ToastType::Info,
                    QStringLiteral("强制对齐完成"), 3000);
            } else {
                QMessageBox::warning(this, QStringLiteral("强制对齐"),
                                     QStringLiteral("强制对齐失败: %1")
                                         .arg(QString::fromStdString(result.error())));
            }
        }, Qt::QueuedConnection);
    });
}

void DsPhonemeLabelerPage::applyFaResult(const QString &sliceId,
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

void DsPhonemeLabelerPage::onBatchFA() {
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
    auto *hfa = m_hfa.get();
    auto *source = m_source;

    (void) QtConcurrent::run([hfa, source, ids, this]() {
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
                        b.pos = phone.start;
                        b.text = QString::fromStdString(phone.text);
                        phonemeLayer.boundaries.push_back(std::move(b));
                    }
                }
                if (!phonemeLayer.boundaries.empty()) {
                    Boundary endB;
                    endB.id = id;
                    endB.pos = words.back().phones.back().end;
                    endB.text = QString();
                    phonemeLayer.boundaries.push_back(std::move(endB));
                }
                layers.push_back(std::move(phonemeLayer));

                QMetaObject::invokeMethod(this, [this, sliceId, layers]() {
                    applyFaResult(sliceId, layers);
                }, Qt::QueuedConnection);
            }
            ++processed;
        }
        QMetaObject::invokeMethod(this, [this, processed]() {
            m_faRunning = false;
            dsfw::widgets::ToastNotification::show(
                this, dsfw::widgets::ToastType::Info,
                QStringLiteral("批量强制对齐完成: %1 个切片").arg(processed), 3000);
        }, Qt::QueuedConnection);
    });
}

} // namespace dstools
