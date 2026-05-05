#include "MinLabelPage.h"
#include "SliceListPanel.h"
#include "ModelConfigHelper.h"
#include "ISettingsBackend.h"

#include <dstools/IEditorDataSource.h>

#include <QLabel>
#include <QIcon>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <QHBoxLayout>
#include <QtConcurrent>

#include "AsrPipeline.h"
#include "FunAsrModelProvider.h"

#include <dsfw/IModelManager.h>
#include <dsfw/IModelProvider.h>
#include <dsfw/ServiceLocator.h>
#include <dsfw/Theme.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dstools/DsTextTypes.h>
#include <dstools/ExecutionProvider.h>
#include <dstools/ModelManager.h>

namespace dstools {

MinLabelPage::MinLabelPage(QWidget *parent)
    : EditorPageBase("MinLabel", parent) {
    m_editor = new Minlabel::MinLabelEditor(this);

    setupBaseLayout(m_editor);
    setupNavigationActions();

    m_playAction = new QAction(QStringLiteral("播放/停止"), this);
    m_playAction->setIcon(QIcon(QStringLiteral(":/icons/play.svg")));
    m_asrAction = new QAction(QStringLiteral("ASR 识别当前曲目"), this);
    connect(m_asrAction, &QAction::triggered, this, &MinLabelPage::onRunAsr);

    static const dstools::SettingsKey<QString> kPlaybackPlay("Shortcuts/play", "F5");
    static const dstools::SettingsKey<QString> kAsrRun("Shortcuts/asr", "R");

    shortcutManager()->bind(m_playAction, kPlaybackPlay,
                            QStringLiteral("播放/停止"), QStringLiteral("播放"));
    shortcutManager()->bind(m_asrAction, kAsrRun,
                            QStringLiteral("ASR 识别"), QStringLiteral("处理"));
    shortcutManager()->applyAll();
    shortcutManager()->updateTooltips();
    shortcutManager()->setEnabled(false);

    connect(m_editor, &Minlabel::MinLabelEditor::dataChanged,
            this, [this]() { m_dirty = true; });
    connect(m_playAction, &QAction::triggered, this, [this]() {
        if (m_editor->playWidget())
            m_editor->playWidget()->setPlaying(!m_editor->playWidget()->isPlaying());
    });
}

MinLabelPage::~MinLabelPage() = default;

// ── EditorPageBase hooks ──────────────────────────────────────────────────────

QString MinLabelPage::windowTitlePrefix() const {
    return QStringLiteral("歌词标注");
}

bool MinLabelPage::isDirty() const {
    return m_dirty;
}

bool MinLabelPage::saveCurrentSlice() {
    if (currentSliceId().isEmpty() || !source() || !m_dirty)
        return true;

    auto result = source()->loadSlice(currentSliceId());
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

    auto saveResult = source()->saveSlice(currentSliceId(), doc);
    if (!saveResult) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                             QString::fromStdString(saveResult.error()));
        return false;
    }

    m_dirty = false;
    return true;
}

void MinLabelPage::onSliceSelectedImpl(const QString &sliceId) {
    auto result = source()->loadSlice(sliceId);
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

    const QString audio = source()->validatedAudioPath(sliceId);
    m_editor->setAudioFile(audio.isEmpty() ? QString() : audio);

    m_dirty = false;
}

void MinLabelPage::onAutoInfer() {
    updateProgress();

    if (source() && !currentSliceId().isEmpty()) {
        auto dirty = source()->dirtyLayers(currentSliceId());
        if (dirty.contains(QStringLiteral("grapheme"))) {
            source()->clearDirtyLayers(currentSliceId(), {QStringLiteral("grapheme")});
            dsfw::widgets::ToastNotification::show(
                this, dsfw::widgets::ToastType::Info,
                QStringLiteral("歌词层已更新"), 3000);
        }
    }
}

void MinLabelPage::onDeactivatedImpl() {
    m_asr = nullptr;
}

// ── IPageActions ──────────────────────────────────────────────────────────────

QMenuBar *MinLabelPage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);

    auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(QStringLiteral("保存"), QKeySequence::Save, this, [this]() { saveCurrentSlice(); });
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("退出(&X)"), this, [this]() {
        if (auto *w = window()) w->close();
    });

    auto *editMenu = bar->addMenu(QStringLiteral("编辑(&E)"));
    editMenu->addAction(prevAction());
    editMenu->addAction(nextAction());
    editMenu->addSeparator();
    {
        auto *shortcutAction = editMenu->addAction(QStringLiteral("快捷键设置..."));
        connect(shortcutAction, &QAction::triggered, this, [this]() {
            shortcutManager()->showEditor(this);
        });
    }

    auto *playMenu = bar->addMenu(QStringLiteral("播放(&P)"));
    playMenu->addAction(m_playAction);

    auto *processMenu = bar->addMenu(QStringLiteral("处理(&R)"));
    processMenu->addAction(m_asrAction);
    processMenu->addSeparator();
    processMenu->addAction(QStringLiteral("批量 ASR..."), this, &MinLabelPage::onBatchAsr);

    auto *viewMenu = bar->addMenu(QStringLiteral("视图(&V)"));
    dsfw::Theme::instance().populateThemeMenu(viewMenu);

    return bar;
}

QWidget *MinLabelPage::createStatusBarContent(QWidget *parent) {
    auto *container = new QWidget(parent);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *sliceLabel = new QLabel(QStringLiteral("未选择切片"), container);
    auto *progressLabel = new QLabel(container);
    layout->addWidget(sliceLabel, 1);
    layout->addWidget(progressLabel);

    connect(this, &MinLabelPage::sliceChanged, sliceLabel, [sliceLabel](const QString &id) {
        sliceLabel->setText(id.isEmpty() ? QStringLiteral("未选择切片") : id);
    });

    return container;
}

bool MinLabelPage::supportsDragDrop() const {
    return true;
}

void MinLabelPage::handleDragEnter(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MinLabelPage::handleDrop(QDropEvent *event) {
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

// ── ASR ───────────────────────────────────────────────────────────────────────

void MinLabelPage::updateProgress() {
    if (!source())
        return;

    const auto ids = source()->sliceIds();
    int total = ids.size();
    int completed = 0;
    for (const auto &id : ids) {
        auto result = source()->loadSlice(id);
        if (result) {
            for (const auto &layer : result.value().layers) {
                if (layer.name == QStringLiteral("grapheme") && !layer.boundaries.empty()) {
                    ++completed;
                    break;
                }
            }
        }
    }
    sliceListPanel()->setProgress(completed, total);
}

void MinLabelPage::ensureAsrEngine() {
    if (m_asr && m_asr->initialized())
        return;

    if (!m_modelManager) {
        m_modelManager = ServiceLocator::get<IModelManager>();
        if (m_modelManager) {
            connect(m_modelManager, &IModelManager::modelInvalidated,
                    this, &MinLabelPage::onModelInvalidated);
        }
    }

    if (!m_modelManager)
        return;

    auto *mm = dynamic_cast<ModelManager *>(m_modelManager);
    if (!mm)
        return;

    auto config = readModelConfig(settingsBackend(), QStringLiteral("asr"));
    if (config.modelPath.isEmpty())
        return;

    auto result = mm->loadModel(QStringLiteral("asr"), config, config.deviceId);
    if (!result)
        return;

    auto typeId = registerModelType("asr");
    auto *provider = mm->provider(typeId);
    auto *asrProvider = dynamic_cast<FunAsrModelProvider *>(provider);
    if (asrProvider && asrProvider->asr() && asrProvider->asr()->initialized())
        m_asr = asrProvider->asr();
}

void MinLabelPage::onModelInvalidated(const QString &taskKey) {
    if (taskKey == QStringLiteral("asr"))
        m_asr = nullptr;
}

void MinLabelPage::onRunAsr() {
    if (currentSliceId().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("ASR"),
                                 QStringLiteral("请先选择一个切片。"));
        return;
    }

    ensureAsrEngine();
    if (!m_asr || !m_asr->initialized()) {
        QMessageBox::warning(this, QStringLiteral("ASR"),
                             QStringLiteral("ASR 模型未加载。请在设置中配置 ASR 模型路径。"));
        return;
    }

    if (m_asrRunning) {
        QMessageBox::information(this, QStringLiteral("ASR"),
                                 QStringLiteral("ASR 正在运行中，请稍候。"));
        return;
    }

    runAsrForSlice(currentSliceId());
}

void MinLabelPage::runAsrForSlice(const QString &sliceId) {
    if (!source())
        return;

    QString audioPath = source()->validatedAudioPath(sliceId);
    if (audioPath.isEmpty()) {
        if (source()->audioPath(sliceId).isEmpty())
            QMessageBox::warning(this, QStringLiteral("ASR"),
                                 QStringLiteral("当前切片没有音频文件。"));
        return;
    }

    m_asrRunning = true;
    auto *asr = m_asr;
    QPointer<MinLabelPage> guard(this);

    (void) QtConcurrent::run([asr, audioPath, sliceId, guard]() {
        std::string msg;
        bool ok = asr->recognize(audioPath.toStdWString(), msg);
        QString text;
        if (ok)
            text = QString::fromUtf8(msg);

        if (!guard)
            return;

        QMetaObject::invokeMethod(guard.data(), [guard, sliceId, text, ok]() {
            if (!guard)
                return;
            guard->m_asrRunning = false;
            if (ok) {
                guard->setAsrResult(sliceId, text);
                dsfw::widgets::ToastNotification::show(
                    guard.data(), dsfw::widgets::ToastType::Info,
                    QStringLiteral("ASR 识别完成"), 3000);
            } else {
                QMessageBox::warning(guard.data(), QStringLiteral("ASR"),
                                     QStringLiteral("ASR 识别失败。"));
            }
        }, Qt::QueuedConnection);
    });
}

void MinLabelPage::onBatchAsr() {
    if (!source()) {
        QMessageBox::warning(this, QStringLiteral("批量 ASR"),
                             QStringLiteral("请先打开工程。"));
        return;
    }

    ensureAsrEngine();
    if (!m_asr || !m_asr->initialized()) {
        QMessageBox::warning(this, QStringLiteral("批量 ASR"),
                             QStringLiteral("ASR 模型未加载。请在设置中配置 ASR 模型路径。"));
        return;
    }

    if (m_asrRunning) {
        QMessageBox::information(this, QStringLiteral("批量 ASR"),
                                 QStringLiteral("ASR 正在运行中，请稍候。"));
        return;
    }

    const auto ids = source()->sliceIds();
    if (ids.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("批量 ASR"),
                                 QStringLiteral("没有可处理的切片。"));
        return;
    }

    m_asrRunning = true;
    auto *asr = m_asr;
    auto *src = source();
    QPointer<MinLabelPage> guard(this);

    (void) QtConcurrent::run([asr, src, ids, guard]() {
        int processed = 0;
        int skipped = 0;
        for (const auto &sliceId : ids) {
            QString audioPath = src->validatedAudioPath(sliceId);
            if (audioPath.isEmpty()) {
                ++skipped;
                continue;
            }

            std::string msg;
            bool ok = asr->recognize(audioPath.toStdWString(), msg);
            if (ok) {
                QString text = QString::fromUtf8(msg);
                QMetaObject::invokeMethod(guard.data(), [guard, sliceId, text]() {
                    if (guard)
                        guard->setAsrResult(sliceId, text);
                }, Qt::QueuedConnection);
            }
            ++processed;
        }
        QMetaObject::invokeMethod(guard.data(), [guard, processed, skipped]() {
            if (!guard)
                return;
            guard->m_asrRunning = false;
            QString msg = QStringLiteral("批量 ASR 完成: %1 个切片").arg(processed);
            if (skipped > 0)
                msg += QStringLiteral("，%1 个音频文件缺失已跳过").arg(skipped);
            dsfw::widgets::ToastNotification::show(
                guard.data(), dsfw::widgets::ToastType::Info,
                msg, 3000);
        }, Qt::QueuedConnection);
    });
}

void MinLabelPage::setAsrResult(const QString &sliceId, const QString &text) {
    if (!source())
        return;

    auto result = source()->loadSlice(sliceId);
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

    (void) source()->saveSlice(sliceId, doc);

    if (sliceId == currentSliceId()) {
        m_editor->loadData(text, {});
        m_dirty = true;
    }

    updateProgress();
}

} // namespace dstools
