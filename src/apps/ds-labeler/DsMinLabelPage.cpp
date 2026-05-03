#include "DsMinLabelPage.h"
#include "DsLabelerKeys.h"
#include "ProjectDataSource.h"
#include "SliceListPanel.h"

#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSplitter>
#include <QVBoxLayout>
#include <QtConcurrent>

#include <Asr.h>

#include <dsfw/PipelineContext.h>
#include <dsfw/Theme.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dstools/DsProject.h>
#include <dstools/DsTextTypes.h>
#include <dstools/FileStatusDelegate.h>

namespace dstools {

struct DsMinLabelPage::AsrEngine {
    std::unique_ptr<LyricFA::Asr> engine;

    bool initialized() const { return engine && engine->initialized(); }

    void create(const QString &modelPath, FunAsr::ExecutionProvider provider, int deviceId) {
        engine = std::make_unique<LyricFA::Asr>(modelPath, provider, deviceId);
    }

    bool recognize(const std::filesystem::path &filepath, std::string &msg) const {
        return engine->recognize(filepath, msg);
    }
};

DsMinLabelPage::DsMinLabelPage(QWidget *parent)
    : QWidget(parent), m_settings("DsLabeler/MinLabel") {
    m_editor = new Minlabel::MinLabelEditor(this);
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
    m_playAction = new QAction(QStringLiteral("播放/停止"), this);

    m_shortcutManager = new dstools::widgets::ShortcutManager(&m_settings, this);
    m_shortcutManager->bind(m_prevAction, DsLabelerKeys::NavigationPrev,
                            QStringLiteral("上一个切片"), QStringLiteral("导航"));
    m_shortcutManager->bind(m_nextAction, DsLabelerKeys::NavigationNext,
                            QStringLiteral("下一个切片"), QStringLiteral("导航"));
    m_shortcutManager->bind(m_playAction, DsLabelerKeys::PlaybackPlay,
                            QStringLiteral("播放/停止"), QStringLiteral("播放"));
    m_shortcutManager->applyAll();

    connect(m_sliceList, &SliceListPanel::sliceSelected,
            this, &DsMinLabelPage::onSliceSelected);
    connect(m_editor, &Minlabel::MinLabelEditor::dataChanged,
            this, [this]() { m_dirty = true; });
    connect(m_prevAction, &QAction::triggered, this, [this]() {
        m_sliceList->selectPrev();
    });
    connect(m_nextAction, &QAction::triggered, this, [this]() {
        m_sliceList->selectNext();
    });
    connect(m_playAction, &QAction::triggered, this, [this]() {
        if (m_editor->playWidget())
            m_editor->playWidget()->setPlaying(!m_editor->playWidget()->isPlaying());
    });
}

DsMinLabelPage::~DsMinLabelPage() = default;

void DsMinLabelPage::setDataSource(ProjectDataSource *source) {
    m_source = source;
    m_sliceList->setDataSource(source);
}

void DsMinLabelPage::onSliceSelected(const QString &sliceId) {
    if (!maybeSave())
        return;

    m_currentSliceId = sliceId;
    m_settings.set(DsLabelerKeys::LastMinLabelSlice, sliceId);

    if (!m_source)
        return;

    auto result = m_source->loadSlice(sliceId);
    if (result) {
        const auto &doc = result.value();
        QString labContent;
        QString rawText;
        for (const auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("grapheme")) {
                QStringList parts;
                for (const auto &b : layer.boundaries) {
                    if (!b.text.isEmpty())
                        parts << b.text;
                }
                labContent = parts.join(QStringLiteral(" "));
            } else if (layer.name == QStringLiteral("raw_text")) {
                QStringList parts;
                for (const auto &b : layer.boundaries) {
                    if (!b.text.isEmpty())
                        parts << b.text;
                }
                rawText = parts.join(QStringLiteral(" "));
            }
        }
        m_editor->loadData(labContent, rawText);
    } else {
        m_editor->loadData({}, {});
    }

    const QString audio = m_source->audioPath(sliceId);
    m_editor->setAudioFile(audio);

    m_dirty = false;
    emit sliceChanged(sliceId);
}

bool DsMinLabelPage::saveCurrentSlice() {
    if (m_currentSliceId.isEmpty() || !m_source || !m_dirty)
        return true;

    auto result = m_source->loadSlice(m_currentSliceId);
    DsTextDocument doc;
    if (result)
        doc = std::move(result.value());

    IntervalLayer *graphemeLayer = nullptr;
    for (auto &layer : doc.layers) {
        if (layer.name == QStringLiteral("grapheme")) {
            graphemeLayer = &layer;
            break;
        }
    }
    if (!graphemeLayer) {
        doc.layers.push_back({});
        graphemeLayer = &doc.layers.back();
        graphemeLayer->name = QStringLiteral("grapheme");
        graphemeLayer->type = QStringLiteral("text");
    }

    graphemeLayer->boundaries.clear();
    const QString lab = m_editor->labContent();
    if (!lab.isEmpty()) {
        const auto parts = lab.split(QChar(' '), Qt::SkipEmptyParts);
        int id = 1;
        for (const auto &part : parts) {
            Boundary b;
            b.id = id++;
            b.text = part;
            graphemeLayer->boundaries.push_back(std::move(b));
        }
    }

    auto saveResult = m_source->saveSlice(m_currentSliceId, doc);
    if (!saveResult) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                             QString::fromStdString(saveResult.error()));
        return false;
    }

    m_dirty = false;
    return true;
}

bool DsMinLabelPage::maybeSave() {
    if (!m_dirty)
        return true;

    auto ret = QMessageBox::question(
        this, QStringLiteral("未保存的更改"),
        QStringLiteral("当前切片已修改，是否保存？"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
        return saveCurrentSlice();
    if (ret == QMessageBox::Discard) {
        m_dirty = false;
        return true;
    }
    return false;
}

QMenuBar *DsMinLabelPage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);

    auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(QStringLiteral("保存"), QKeySequence::Save, this, [this]() { saveCurrentSlice(); });
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("退出(&X)"), this, [this]() {
        if (auto *w = window()) w->close();
    });

    auto *editMenu = bar->addMenu(QStringLiteral("编辑(&E)"));
    editMenu->addAction(m_prevAction);
    editMenu->addAction(m_nextAction);
    editMenu->addSeparator();
    {
        auto *shortcutAction = editMenu->addAction(QStringLiteral("快捷键设置..."));
        connect(shortcutAction, &QAction::triggered, this, [this]() {
            m_shortcutManager->showEditor(this);
        });
    }

    auto *playMenu = bar->addMenu(QStringLiteral("播放(&P)"));
    playMenu->addAction(m_playAction);

    auto *processMenu = bar->addMenu(QStringLiteral("处理(&R)"));
    processMenu->addAction(QStringLiteral("ASR 识别当前曲目"), this, &DsMinLabelPage::onRunAsr);
    processMenu->addSeparator();
    processMenu->addAction(QStringLiteral("批量 ASR..."), this, &DsMinLabelPage::onBatchAsr);

    auto *viewMenu = bar->addMenu(QStringLiteral("视图(&V)"));
    dsfw::Theme::instance().populateThemeMenu(viewMenu);

    return bar;
}

QWidget *DsMinLabelPage::createStatusBarContent(QWidget *parent) {
    auto *container = new QWidget(parent);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *sliceLabel = new QLabel(QStringLiteral("未选择切片"), container);
    auto *progressLabel = new QLabel(container);
    layout->addWidget(sliceLabel, 1);
    layout->addWidget(progressLabel);

    connect(this, &DsMinLabelPage::sliceChanged, sliceLabel, [sliceLabel](const QString &id) {
        sliceLabel->setText(id.isEmpty() ? QStringLiteral("未选择切片") : id);
    });

    return container;
}

QString DsMinLabelPage::windowTitle() const {
    QString title = QStringLiteral("DsLabeler — 歌词标注");
    if (!m_currentSliceId.isEmpty())
        title += QStringLiteral(" — ") + m_currentSliceId;
    return title;
}

bool DsMinLabelPage::hasUnsavedChanges() const {
    return m_dirty;
}

bool DsMinLabelPage::supportsDragDrop() const {
    return true;
}

void DsMinLabelPage::handleDragEnter(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void DsMinLabelPage::handleDrop(QDropEvent *event) {
    const QMimeData *mime = event->mimeData();
    if (!mime->hasUrls())
        return;

    auto urls = mime->urls();
    for (const auto &url : urls) {
        if (url.isLocalFile()) {
            QString path = url.toLocalFile();
            if (path.endsWith(QLatin1String(".dsproj"), Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

dstools::widgets::ShortcutManager *DsMinLabelPage::shortcutManager() const {
    return m_shortcutManager;
}

void DsMinLabelPage::onActivated() {
    m_sliceList->refresh();
    updateProgress();

    // Restore last selected slice from settings, or select first
    if (m_currentSliceId.isEmpty()) {
        QString lastSlice = m_settings.get(DsLabelerKeys::LastMinLabelSlice);
        if (!lastSlice.isEmpty()) {
            m_sliceList->setCurrentSlice(lastSlice);
        } else if (m_sliceList->sliceCount() > 0) {
            m_sliceList->setCurrentSlice(m_sliceList->currentSliceId());
        }
    }

    if (m_source && !m_currentSliceId.isEmpty()) {
        auto *ctx = m_source->context(m_currentSliceId);
        if (ctx && ctx->dirty.contains(QStringLiteral("grapheme"))) {
            ctx->dirty.removeAll(QStringLiteral("grapheme"));
            m_source->saveContext(m_currentSliceId);
            dsfw::widgets::ToastNotification::show(
                this, dsfw::widgets::ToastType::Info,
                QStringLiteral("歌词层已更新"), 3000);
        }
    }
}

bool DsMinLabelPage::onDeactivating() {
    return maybeSave();
}

void DsMinLabelPage::onShutdown() {
    m_shortcutManager->saveAll();
}

void DsMinLabelPage::updateProgress() {
    if (!m_source)
        return;

    const auto ids = m_source->sliceIds();
    int total = ids.size();
    int completed = 0;
    for (const auto &id : ids) {
        auto result = m_source->loadSlice(id);
        if (result) {
            for (const auto &layer : result.value().layers) {
                if (layer.name == QStringLiteral("grapheme") && !layer.boundaries.empty()) {
                    ++completed;
                    break;
                }
            }
        }
    }
    m_sliceList->setProgress(completed, total);
}

void DsMinLabelPage::ensureAsrEngine() {
    if (m_asrEngine && m_asrEngine->initialized())
        return;

    if (!m_source || !m_source->project())
        return;

    auto it = m_source->project()->defaults().taskModels.find(QStringLiteral("asr"));
    if (it == m_source->project()->defaults().taskModels.end())
        return;

    const auto &config = it->second;
    if (config.modelPath.isEmpty())
        return;

    auto provider = FunAsr::ExecutionProvider::CPU;
#ifdef ONNXRUNTIME_ENABLE_DML
    if (config.provider == QStringLiteral("dml"))
        provider = FunAsr::ExecutionProvider::DML;
#endif

    if (!m_asrEngine)
        m_asrEngine = std::make_unique<AsrEngine>();
    m_asrEngine->create(config.modelPath, provider, config.deviceId);
}

void DsMinLabelPage::onRunAsr() {
    if (m_currentSliceId.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("ASR"),
                                 QStringLiteral("请先选择一个切片。"));
        return;
    }

    ensureAsrEngine();
    if (!m_asrEngine || !m_asrEngine->initialized()) {
        QMessageBox::warning(this, QStringLiteral("ASR"),
                             QStringLiteral("ASR 模型未加载。请在工程设置中配置 ASR 模型路径。"));
        return;
    }

    if (m_asrRunning) {
        QMessageBox::information(this, QStringLiteral("ASR"),
                                 QStringLiteral("ASR 正在运行中，请稍候。"));
        return;
    }

    runAsrForSlice(m_currentSliceId);
}

void DsMinLabelPage::runAsrForSlice(const QString &sliceId) {
    if (!m_source)
        return;

    QString audioPath = m_source->audioPath(sliceId);
    if (audioPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("ASR"),
                             QStringLiteral("当前切片没有音频文件。"));
        return;
    }

    m_asrRunning = true;
    auto *engine = m_asrEngine.get();

    (void) QtConcurrent::run([engine, audioPath, sliceId, this]() {
        std::string msg;
        bool ok = engine->recognize(audioPath.toStdWString(), msg);
        QString text;
        if (ok)
            text = QString::fromUtf8(msg);

        QMetaObject::invokeMethod(this, [this, sliceId, text, ok]() {
            m_asrRunning = false;
            if (ok) {
                setAsrResult(sliceId, text);
                dsfw::widgets::ToastNotification::show(
                    this, dsfw::widgets::ToastType::Info,
                    QStringLiteral("ASR 识别完成"), 3000);
            } else {
                QMessageBox::warning(this, QStringLiteral("ASR"),
                                     QStringLiteral("ASR 识别失败。"));
            }
        }, Qt::QueuedConnection);
    });
}

void DsMinLabelPage::onBatchAsr() {
    if (!m_source) {
        QMessageBox::warning(this, QStringLiteral("批量 ASR"),
                             QStringLiteral("请先打开工程。"));
        return;
    }

    ensureAsrEngine();
    if (!m_asrEngine || !m_asrEngine->initialized()) {
        QMessageBox::warning(this, QStringLiteral("批量 ASR"),
                             QStringLiteral("ASR 模型未加载。请在工程设置中配置 ASR 模型路径。"));
        return;
    }

    if (m_asrRunning) {
        QMessageBox::information(this, QStringLiteral("批量 ASR"),
                                 QStringLiteral("ASR 正在运行中，请稍候。"));
        return;
    }

    const auto ids = m_source->sliceIds();
    if (ids.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("批量 ASR"),
                                 QStringLiteral("没有可处理的切片。"));
        return;
    }

    m_asrRunning = true;
    auto *engine = m_asrEngine.get();
    auto *source = m_source;

    (void) QtConcurrent::run([engine, source, ids, this]() {
        int processed = 0;
        for (const auto &sliceId : ids) {
            QString audioPath = source->audioPath(sliceId);
            if (audioPath.isEmpty())
                continue;

            std::string msg;
            bool ok = engine->recognize(audioPath.toStdWString(), msg);
            if (ok) {
                QString text = QString::fromUtf8(msg);
                QMetaObject::invokeMethod(this, [this, sliceId, text]() {
                    setAsrResult(sliceId, text);
                }, Qt::QueuedConnection);
            }
            ++processed;
        }
        QMetaObject::invokeMethod(this, [this, processed]() {
            m_asrRunning = false;
            dsfw::widgets::ToastNotification::show(
                this, dsfw::widgets::ToastType::Info,
                QStringLiteral("批量 ASR 完成: %1 个切片").arg(processed), 3000);
        }, Qt::QueuedConnection);
    });
}

void DsMinLabelPage::setAsrResult(const QString &sliceId, const QString &text) {
    if (!m_source)
        return;

    auto result = m_source->loadSlice(sliceId);
    DsTextDocument doc;
    if (result)
        doc = std::move(result.value());

    IntervalLayer *graphemeLayer = nullptr;
    for (auto &layer : doc.layers) {
        if (layer.name == QStringLiteral("grapheme")) {
            graphemeLayer = &layer;
            break;
        }
    }
    if (!graphemeLayer) {
        doc.layers.push_back({});
        graphemeLayer = &doc.layers.back();
        graphemeLayer->name = QStringLiteral("grapheme");
        graphemeLayer->type = QStringLiteral("text");
    }

    graphemeLayer->boundaries.clear();
    const auto parts = text.split(QChar(' '), Qt::SkipEmptyParts);
    int id = 1;
    for (const auto &part : parts) {
        Boundary b;
        b.id = id++;
        b.text = part;
        graphemeLayer->boundaries.push_back(std::move(b));
    }

    (void) m_source->saveSlice(sliceId, doc);

    if (sliceId == m_currentSliceId) {
        m_editor->loadData(text, {});
        m_dirty = true;
    }

    updateProgress();
}

} // namespace dstools
