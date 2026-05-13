#include "MinLabelPage.h"
#include "BatchProcessDialog.h"
#include "ISettingsBackend.h"
#include "ModelConfigHelper.h"
#include "SliceListPanel.h"

#include <dstools/IEditorDataSource.h>

#include <dsfw/Log.h>

#include <cpp-pinyin/Pinyin.h>

#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QIcon>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <QRegularExpression>
#include <QTimer>
#include <QtConcurrent>

#include "AsrPipeline.h"
#include "FunAsrModelProvider.h"
#include "MatchLyric.h"

#include <QLabel>
#include <dsfw/IModelManager.h>
#include <dsfw/IModelProvider.h>
#include <dsfw/Theme.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dstools/DsTextTypes.h>
#include <dstools/ExecutionProvider.h>
#include <dstools/ModelManager.h>

namespace dstools {

    MinLabelPage::MinLabelPage(QWidget *parent) : EditorPageBase("MinLabel", parent) {
        m_editor = new Minlabel::MinLabelEditor(this);

        setupBaseLayout(m_editor);
        setupNavigationActions();

        m_playAction = new QAction(tr("Play/Pause"), this);
        m_playAction->setIcon(QIcon(QStringLiteral(":/icons/play.svg")));
        m_asrAction = new QAction(tr("ASR Recognize Current"), this);
        connect(m_asrAction, &QAction::triggered, this, &MinLabelPage::onRunAsr);

        m_lyricFaAction = new QAction(tr("LyricFA Match Current"), this);
        connect(m_lyricFaAction, &QAction::triggered, this, &MinLabelPage::onRunLyricFa);

        static const dstools::SettingsKey<QString> kPlaybackPlay("Shortcuts/playPause", "Space");
        static const dstools::SettingsKey<QString> kAsrRun("Shortcuts/asr", "R");
        static const dstools::SettingsKey<QString> kLyricFaRun("Shortcuts/lyricfa", "L");

        shortcutManager()->bind(m_playAction, kPlaybackPlay, tr("Play/Pause"), tr("Playback"));
        shortcutManager()->bind(m_asrAction, kAsrRun, tr("ASR Recognize"), tr("Processing"));
        shortcutManager()->bind(m_lyricFaAction, kLyricFaRun, tr("LyricFA Match"), tr("Processing"));
        shortcutManager()->applyAll();
        shortcutManager()->updateTooltips();
        shortcutManager()->setEnabled(false);

        connect(m_editor, &Minlabel::MinLabelEditor::dataChanged, this, [this]() {
            m_dirty = true;
            updateDirtyIndicator();
        });
        connect(m_playAction, &QAction::triggered, this, [this]() {
            if (m_editor->playWidget())
                m_editor->playWidget()->setPlaying(!m_editor->playWidget()->isPlaying());
        });
    }

    MinLabelPage::~MinLabelPage() = default;

    // ── Batch processing (P-12 template) ─────────────────────────────────────

    bool MinLabelPage::isBatchRunning() const {
        return m_batchRunning;
    }

    void MinLabelPage::setBatchRunning(bool running) {
        m_batchRunning = running;
        m_asrRunning = running;
    }

    std::shared_ptr<std::atomic<bool>> MinLabelPage::batchAliveToken() const {
        return aliveToken(QStringLiteral("asr")).token();
    }

    bool MinLabelPage::hasExistingResult(const QString &sliceId) const {
        if (!source())
            return false;
        auto result = source()->loadSlice(sliceId);
        if (!result)
            return false;
        for (const auto &layer : result.value().layers) {
            if (layer.name == QStringLiteral("grapheme") && !layer.boundaries.empty())
                return true;
        }
        return false;
    }

    BatchSliceResult MinLabelPage::processSlice(const QString &sliceId) {
        if (!aliveToken(QStringLiteral("asr")).isValid())
            return {BatchSliceResult::Error, tr("ASR model invalidated")};

        QString audioPath = source()->validatedAudioPath(sliceId);
        std::string msg;
        bool ok = m_asr->recognize(audioPath.toStdWString(), msg);
        if (ok) {
            m_pendingAsrText = QString::fromUtf8(msg);
            return {BatchSliceResult::Processed,
                    tr("[OK] %1: \"%2\"").arg(sliceId, m_pendingAsrText.left(80))};
        }
        return {BatchSliceResult::Error,
                tr("[ERROR] %1: recognition failed - %2").arg(sliceId, QString::fromStdString(msg))};
    }

    void MinLabelPage::applyBatchResult(const QString &sliceId, const BatchSliceResult &result) {
        Q_UNUSED(result)
        setAsrResult(sliceId, m_pendingAsrText);
        if (m_batchAutoG2P)
            batchG2P(sliceId);
    }

    // ── EditorPageBase hooks ──────────────────────────────────────────────────────

    QString MinLabelPage::windowTitlePrefix() const {
        return tr("Lyrics Labeling");
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
            static const QRegularExpression cjkRe(
                QStringLiteral("[\\x{4E00}-\\x{9FFF}\\x{3400}-\\x{4DBF}\\x{F900}-\\x{FAFF}]"));
            if (cjkRe.match(lab).hasMatch()) {
                auto btn = QMessageBox::warning(
                    this, tr("Content Warning"),
                    tr("The result contains Chinese characters, which may not be valid phonetic content. "
                       "Save anyway?"),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (btn != QMessageBox::Yes)
                    return false;
            }

            const auto parts = lab.split(QChar(' '), Qt::SkipEmptyParts);
            int id = 1;
            for (const auto &part : parts) {
                Boundary b;
                b.id = id++;
                b.text = part;
                graphemeLayer->boundaries.push_back(std::move(b));
            }
        }

        IntervalLayer *rawTextLayer = nullptr;
        for (auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("raw_text")) {
                rawTextLayer = &layer;
                break;
            }
        }
        if (!rawTextLayer) {
            doc.layers.push_back({});
            rawTextLayer = &doc.layers.back();
            rawTextLayer->name = QStringLiteral("raw_text");
            rawTextLayer->type = QStringLiteral("text");
        }

        rawTextLayer->boundaries.clear();
        const QString raw = m_editor->rawText();
        if (!raw.isEmpty()) {
            const auto rawParts = raw.split(QChar(' '), Qt::SkipEmptyParts);
            int rid = 1;
            for (const auto &part : rawParts) {
                Boundary b;
                b.id = rid++;
                b.text = part;
                rawTextLayer->boundaries.push_back(std::move(b));
            }
        }

        auto saveResult = source()->saveSlice(currentSliceId(), doc);
        if (!saveResult) {
            QMessageBox::warning(this, tr("Save Failed"), QString::fromStdString(saveResult.error()));
            return false;
        }

        m_dirty = false;
        updateDirtyIndicator();
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
        updateDirtyIndicator();
    }

    void MinLabelPage::onAutoInfer() {
        updateProgress();

        if (source() && !currentSliceId().isEmpty()) {
            auto dirty = source()->dirtyLayers(currentSliceId());
            if (dirty.contains(QStringLiteral("grapheme"))) {
                source()->clearDirtyLayers(currentSliceId(), {QStringLiteral("grapheme")});
                dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info, tr("Lyrics layer updated"),
                                                       3000);
            }
        }
    }

    void MinLabelPage::onDeactivatedImpl() {
        aliveToken(QStringLiteral("asr")).invalidate();
        aliveToken(QStringLiteral("match_lyric")).invalidate();
        m_asr = nullptr;
    }

    // ── IPageActions ──────────────────────────────────────────────────────────────

    QMenuBar *MinLabelPage::createMenuBar(QWidget *parent) {
        auto *bar = new QMenuBar(parent);

        auto *fileMenu = bar->addMenu(tr("&File"));
        fileMenu->addAction(tr("Save"), QKeySequence::Save, this, [this]() { saveCurrentSlice(); });
        fileMenu->addSeparator();
        fileMenu->addAction(tr("E&xit"), this, [this]() {
            if (auto *w = window())
                w->close();
        });

        auto *editMenu = bar->addMenu(tr("&Edit"));
        editMenu->addAction(prevAction());
        editMenu->addAction(nextAction());
        editMenu->addSeparator();
        {
            auto *shortcutAction = editMenu->addAction(tr("Shortcut Settings..."));
            connect(shortcutAction, &QAction::triggered, this, [this]() { shortcutManager()->showEditor(this); });
        }

        auto *playMenu = bar->addMenu(tr("&Playback"));
        playMenu->addAction(m_playAction);

        auto *processMenu = bar->addMenu(tr("P&rocessing"));
        processMenu->addAction(m_asrAction);
        processMenu->addSeparator();
        processMenu->addAction(tr("Batch ASR..."), this, &MinLabelPage::onBatchAsr);
        processMenu->addSeparator();
        processMenu->addAction(m_lyricFaAction);
        processMenu->addAction(tr("Batch LyricFA..."), this, &MinLabelPage::onBatchLyricFa);

        auto *viewMenu = bar->addMenu(tr("&View"));
        dsfw::Theme::instance().populateThemeMenu(viewMenu);

        return bar;
    }

    QWidget *MinLabelPage::createStatusBarContent(QWidget *parent) {
        auto *container = new QWidget(parent);
        StatusBarBuilder builder(container);

        auto *sliceLabel = builder.addLabel(tr("No slice selected"), 1);
        builder.addLabel(QString());

        builder.connect(this, &MinLabelPage::sliceChanged, sliceLabel, [this, sliceLabel](const QString &id) {
            sliceLabel->setText(id.isEmpty() ? tr("No slice selected") : id);
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

        auto *mgr = ensureModelManager();
        if (!mgr)
            return;

        if (!aliveToken(QStringLiteral("asr")).isValid()) {
            connect(mgr, &IModelManager::modelInvalidated, this, &MinLabelPage::onModelInvalidated);
        }

        auto [mm, typeId] = loadModelForTask(QStringLiteral("asr"));
        if (!mm || !typeId.isValid())
            return;

        auto *provider = mm->provider(typeId);
        auto *asrProvider = dynamic_cast<FunAsrModelProvider *>(provider);
        if (asrProvider && asrProvider->asr() && asrProvider->asr()->initialized()) {
            m_asr = asrProvider->asr();
            aliveToken(QStringLiteral("asr")).create();
        }
    }

    void MinLabelPage::onModelInvalidated(const QString &taskKey) {
        aliveToken(taskKey).invalidate();
        if (taskKey == QStringLiteral("asr")) {
            m_asr = nullptr;
            DSFW_LOG_WARN("infer", "ASR task cancelled: model invalidated");
        }
    }

    void MinLabelPage::ensureAsrEngineAsync(std::function<void()> onReady) {
        if (m_asr && m_asr->initialized()) {
            if (onReady)
                onReady();
            return;
        }
        QPointer<MinLabelPage> guard(this);
        QTimer::singleShot(0, this, [this, guard, onReady = std::move(onReady)]() {
            if (!guard)
                return;
            ensureAsrEngine();
            if (m_asr && m_asr->initialized() && onReady)
                onReady();
        });
    }

    void MinLabelPage::onRunAsr() {
        if (currentSliceId().isEmpty()) {
            QMessageBox::information(this, tr("ASR"), tr("Please select a slice first."));
            return;
        }

        ensureAsrEngine();
        if (!m_asr || !m_asr->initialized()) {
            QMessageBox::warning(this, tr("ASR"),
                                 tr("ASR model not loaded. Please configure the ASR model path in Settings."));
            return;
        }

        if (m_asrRunning) {
            QMessageBox::information(this, tr("ASR"), tr("ASR is running, please wait."));
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
                QMessageBox::warning(this, tr("ASR"), tr("Current slice has no audio file."));
            return;
        }

        m_asrRunning = true;
        DSFW_LOG_INFO("infer", ("ASR started: " + sliceId.toStdString()).c_str());
        auto *asr = m_asr;
        auto asrAlive = aliveToken(QStringLiteral("asr")).token();
        QPointer<MinLabelPage> guard(this);

        (void) QtConcurrent::run([asr, asrAlive, audioPath, sliceId, guard]() {
            if (!asrAlive || !*asrAlive)
                return;
            if (!asr)
                return;
            std::string msg;
            bool ok = false;

            try {
                ok = asr->recognize(audioPath.toStdWString(), msg);
            } catch (const std::exception &e) {
                DSFW_LOG_ERROR("infer", ("ASR exception: " + sliceId.toStdString() + " - " + e.what()).c_str());
                msg = std::string("Exception: ") + e.what();
            }

            QString text;
            if (ok)
                text = QString::fromUtf8(msg);

            if (!guard)
                return;

            QMetaObject::invokeMethod(
                guard.data(),
                [=]() {
                    if (!guard)
                        return;
                    guard->m_asrRunning = false;
                    if (ok) {
                        guard->setAsrResult(sliceId, text);
                        DSFW_LOG_INFO("infer", ("ASR completed: " + sliceId.toStdString() + " - \"" +
                                                text.left(50).toStdString() + "\"")
                                                   .c_str());
                        dsfw::widgets::ToastNotification::show(guard.data(), dsfw::widgets::ToastType::Info,
                                                               tr("ASR recognition completed"), 3000);
                    } else {
                        DSFW_LOG_ERROR("infer", ("ASR failed: " + sliceId.toStdString() + " - " + msg).c_str());
                        QMessageBox::warning(guard.data(), tr("ASR"),
                                             tr("ASR recognition failed: %1").arg(QString::fromUtf8(msg)));
                    }
                },
                Qt::QueuedConnection);
        });
    }

    void MinLabelPage::onBatchAsr() {
        if (!source()) {
            QMessageBox::warning(this, tr("Batch ASR"), tr("Please open a project first."));
            return;
        }

        ensureAsrEngine();
        if (!m_asr || !m_asr->initialized()) {
            QMessageBox::warning(this, tr("Batch ASR"),
                                 tr("ASR model not loaded. Please configure the ASR model path in Settings."));
            return;
        }

        if (m_asrRunning) {
            QMessageBox::information(this, tr("Batch ASR"), tr("ASR is running, please wait."));
            return;
        }

        const auto ids = source()->sliceIds();
        if (ids.isEmpty()) {
            QMessageBox::information(this, tr("Batch ASR"), tr("No slices to process."));
            return;
        }

        m_batchAutoG2P = false;

        BatchConfig config;
        config.dialogTitle = tr("Batch ASR");
        config.skipExistingLabel = tr("Skip slices with existing lyrics");
        config.defaultSkipExisting = true;

        runBatchProcess(config, ids, [this](BatchProcessDialog *dlg) {
            auto *autoG2PBox = new QCheckBox(tr("自动 G2P (拼音) 到结果"), dlg);
            autoG2PBox->setChecked(false);
            dlg->addParamWidget(autoG2PBox);
            QPointer<MinLabelPage> guard(this);
            connect(dlg, &BatchProcessDialog::started, this, [this, guard, autoG2PBox]() {
                if (guard)
                    m_batchAutoG2P = autoG2PBox->isChecked();
            });
        });
    }

    void MinLabelPage::setAsrResult(const QString &sliceId, const QString &text) {
        if (!source())
            return;

        auto result = source()->loadSlice(sliceId);
        DsTextDocument doc;
        if (result)
            doc = std::move(result.value());

        IntervalLayer *rawTextLayer = nullptr;
        for (auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("raw_text")) {
                rawTextLayer = &layer;
                break;
            }
        }
        if (!rawTextLayer) {
            doc.layers.push_back({});
            rawTextLayer = &doc.layers.back();
            rawTextLayer->name = QStringLiteral("raw_text");
            rawTextLayer->type = QStringLiteral("text");
        }

        rawTextLayer->boundaries.clear();
        const auto parts = text.split(QChar(' '), Qt::SkipEmptyParts);
        int id = 1;
        for (const auto &part : parts) {
            Boundary b;
            b.id = id++;
            b.text = part;
            rawTextLayer->boundaries.push_back(std::move(b));
        }

        (void) source()->saveSlice(sliceId, doc);

        if (sliceId == currentSliceId()) {
            m_editor->textWidget()->wordsText->setText(text);
            m_editor->autoG2P();
            m_dirty = true;
        }

        updateProgress();
    }

    void MinLabelPage::batchG2P(const QString &sliceId) {
        if (!source())
            return;

        auto result = source()->loadSlice(sliceId);
        if (!result)
            return;

        auto doc = std::move(result.value());

        // Collect text from raw_text layer
        QStringList rawParts;
        for (const auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("raw_text")) {
                for (const auto &b : layer.boundaries) {
                    if (!b.text.isEmpty())
                        rawParts.append(b.text);
                }
                break;
            }
        }
        if (rawParts.isEmpty())
            return;

        QString rawText = rawParts.join(QStringLiteral(" "));

        // Run Pinyin G2P
        static Pinyin::Pinyin g2pMan;
        const auto pinyinStr = g2pMan.hanziToPinyin(
            rawText.toUtf8().toStdString(),
            Pinyin::ManTone::TONE3,
            Pinyin::Error::Default, false, false);

        const QString phonemeStr = QString::fromStdString(pinyinStr.toStdStr()).trimmed();
        if (phonemeStr.isEmpty())
            return;

        // Write grapheme layer
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
        const auto parts = phonemeStr.split(QChar(' '), Qt::SkipEmptyParts);
        int id = 1;
        for (const auto &part : parts) {
            Boundary b;
            b.id = id++;
            b.text = part;
            graphemeLayer->boundaries.push_back(std::move(b));
        }

        (void) source()->saveSlice(sliceId, doc);

        if (sliceId == currentSliceId()) {
            m_editor->loadData(phonemeStr, m_editor->rawText());
            m_dirty = true;
        }
    }

    // ── LyricFA ──────────────────────────────────────────────────────────────────

    void MinLabelPage::ensureLyricLibrary() {
        if (m_matchLyric && m_matchLyric->isInitialized())
            return;

        static const dstools::SettingsKey<QString> kLyricDir("LyricFA/libraryPath", {});
        QString lyricDir = settings().get(kLyricDir);

        if (lyricDir.isEmpty() || !QDir(lyricDir).exists()) {
            lyricDir = QFileDialog::getExistingDirectory(this, tr("Select Lyric Library Directory"), lyricDir);
            if (lyricDir.isEmpty())
                return;
            settings().set(kLyricDir, lyricDir);
        }

        if (!m_matchLyric)
            m_matchLyric = std::make_unique<LyricFA::MatchLyric>();

        m_matchLyric->initLyric(lyricDir);

        if (!m_matchLyric->isInitialized()) {
            QMessageBox::warning(this, tr("LyricFA"), tr("No lyric files found in the selected directory."));
            m_matchLyric.reset();
            aliveToken(QStringLiteral("match_lyric")).invalidate();
        } else {
            aliveToken(QStringLiteral("match_lyric")).create();
        }
    }

    void MinLabelPage::onRunLyricFa() {
        if (currentSliceId().isEmpty()) {
            QMessageBox::information(this, tr("LyricFA"), tr("Please select a slice first."));
            return;
        }

        ensureLyricLibrary();
        if (!m_matchLyric || !m_matchLyric->isInitialized())
            return;

        const QString asrText = m_editor->rawText();
        if (asrText.isEmpty()) {
            QMessageBox::information(this, tr("LyricFA"), tr("No input text to match. Run ASR first."));
            return;
        }

        QString matchedText;
        QString msg;
        if (m_matchLyric->matchText(currentSliceId(), asrText, matchedText, msg)) {
            applyLyricFaResult(currentSliceId(), matchedText);
            DSFW_LOG_INFO("infer", ("LyricFA matched: " + currentSliceId().toStdString() + " -> \"" +
                                    matchedText.left(50).toStdString() + "\"")
                                       .c_str());
            dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info, tr("LyricFA match completed"),
                                                   3000);
        } else {
            DSFW_LOG_WARN("infer",
                          ("LyricFA failed: " + currentSliceId().toStdString() + " - " + msg.toStdString()).c_str());
            QMessageBox::warning(this, tr("LyricFA"), tr("LyricFA match failed: %1").arg(msg));
        }
    }

    void MinLabelPage::onBatchLyricFa() {
        if (!source()) {
            QMessageBox::warning(this, tr("Batch LyricFA"), tr("Please open a project first."));
            return;
        }

        ensureLyricLibrary();
        if (!m_matchLyric || !m_matchLyric->isInitialized())
            return;

        const auto ids = source()->sliceIds();
        if (ids.isEmpty()) {
            QMessageBox::information(this, tr("Batch LyricFA"), tr("No slices to process."));
            return;
        }

        auto *dlg = new BatchProcessDialog(tr("Batch LyricFA"), this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);

        auto *skipExisting = new QCheckBox(tr("Skip slices with existing lyrics"), dlg);
        skipExisting->setChecked(true);
        dlg->addParamWidget(skipExisting);

        dlg->appendLog(tr("Total slices: %1").arg(ids.size()));

        auto *matchLyric = m_matchLyric.get();
        auto matchLyricAlive = aliveToken(QStringLiteral("match_lyric")).token();
        auto *src = source();
        QPointer<MinLabelPage> guard(this);

        connect(dlg, &BatchProcessDialog::started, this, [dlg, matchLyric, matchLyricAlive, src, ids, guard, skipExisting]() {
            if (!guard) {
                dlg->finish(0, 0, 0);
                return;
            }
            bool skip = skipExisting->isChecked();

            (void) QtConcurrent::run([dlg, matchLyric, matchLyricAlive, src, ids, guard, skip]() {
                int processed = 0;
                int skipped = 0;
                int errors = 0;
                int idx = 0;
                try {
                for (const auto &sliceId : ids) {
                    if (dlg->isCancelled())
                        break;
                    if (!matchLyricAlive || !*matchLyricAlive)
                        break;

                    QMetaObject::invokeMethod(
                        dlg, [dlg, idx, total = ids.size()]() { dlg->setProgress(idx, total); }, Qt::QueuedConnection);

                    QString asrText;
                    if (src) {
                        auto result = src->loadSlice(sliceId);
                        if (result) {
                            for (const auto &layer : result.value().layers) {
                                if (layer.name == QStringLiteral("raw_text")) {
                                    QStringList parts;
                                    for (const auto &b : layer.boundaries) {
                                        if (!b.text.isEmpty())
                                            parts << b.text;
                                    }
                                    asrText = parts.join(QStringLiteral(" "));
                                    break;
                                }
                            }
                            if (skip) {
                                for (const auto &layer : result.value().layers) {
                                    if (layer.name == QStringLiteral("grapheme") && !layer.boundaries.empty()) {
                                        asrText.clear();
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    if (asrText.isEmpty()) {
                        ++skipped;
                        QMetaObject::invokeMethod(
                            dlg, [dlg, sliceId]() { dlg->appendLog(tr("[SKIP] %1 (no input text)").arg(sliceId)); },
                            Qt::QueuedConnection);
                        ++idx;
                        continue;
                    }

                    QString matchedText;
                    QString msg;
                    if (matchLyric->matchText(sliceId, asrText, matchedText, msg)) {
                        QMetaObject::invokeMethod(
                            guard.data(),
                            [guard, sliceId, matchedText]() {
                                if (guard)
                                    guard->applyLyricFaResult(sliceId, matchedText);
                            },
                            Qt::QueuedConnection);
                        QMetaObject::invokeMethod(
                            dlg,
                            [dlg, sliceId, matchedText]() {
                                dlg->appendLog(tr("[OK] %1: \"%2\"").arg(sliceId, matchedText.left(80)));
                            },
                            Qt::QueuedConnection);
                        ++processed;
                    } else {
                        ++errors;
                        QMetaObject::invokeMethod(
                            dlg,
                            [dlg, sliceId, msg]() { dlg->appendLog(tr("[ERROR] %1: %2").arg(sliceId, msg.left(80))); },
                            Qt::QueuedConnection);
                    }
                    ++idx;
                }
                } catch (const std::exception &e) {
                    DSFW_LOG_ERROR("infer", ("Batch lyric FA exception: " + std::string(e.what())).c_str());
                    QMetaObject::invokeMethod(
                        dlg, [dlg, eMsg = std::string(e.what())]() {
                            dlg->appendLog(tr("[ERROR] Exception: %1").arg(QString::fromStdString(eMsg)));
                        },
                        Qt::QueuedConnection);
                }
                QMetaObject::invokeMethod(
                    dlg, [dlg, processed, skipped, errors]() { dlg->finish(processed, skipped, errors); },
                    Qt::QueuedConnection);
            });
        });

        dlg->show();
    }

    void MinLabelPage::applyLyricFaResult(const QString &sliceId, const QString &matchedText) {
        if (!source())
            return;

        auto result = source()->loadSlice(sliceId);
        DsTextDocument doc;
        if (result)
            doc = std::move(result.value());

        IntervalLayer *rawTextLayer = nullptr;
        for (auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("raw_text")) {
                rawTextLayer = &layer;
                break;
            }
        }
        if (!rawTextLayer) {
            doc.layers.push_back({});
            rawTextLayer = &doc.layers.back();
            rawTextLayer->name = QStringLiteral("raw_text");
            rawTextLayer->type = QStringLiteral("text");
        }

        rawTextLayer->boundaries.clear();
        const auto parts = matchedText.split(QChar(' '), Qt::SkipEmptyParts);
        int id = 1;
        for (const auto &part : parts) {
            Boundary b;
            b.id = id++;
            b.text = part;
            rawTextLayer->boundaries.push_back(std::move(b));
        }

        (void) source()->saveSlice(sliceId, doc);

        if (sliceId == currentSliceId()) {
            m_editor->textWidget()->wordsText->setText(matchedText);
            m_editor->autoG2P();
            m_dirty = true;
            updateDirtyIndicator();
        }

        updateProgress();
    }

} // namespace dstools
