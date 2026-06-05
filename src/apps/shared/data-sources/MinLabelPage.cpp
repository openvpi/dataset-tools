#include "MinLabelPage.h"

#include "AsrPipeline.h"
#include "BatchProcessDialog.h"
#include "FunAsrModelProvider.h"
#include "AppSettingsBackend.h"
#include "MatchLyric.h"
#include "ModelConfigHelper.h"
#include "Keys.h"
#include "SliceListPanel.h"
#include "../settings/Keys.h"
#include "EnginePool.h"

#include <dsfw/widgets/FilePathSelector.h>

#include <dstools/DsKeys.h>

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QRegularExpression>
#include <QtConcurrent>
#include <cpp-pinyin/Pinyin.h>
#include <dstools/ModelManager.h>
#include <dsfw/IModelProvider.h>
#include <dsfw/Theme.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dstools/DsTextTypes.h>
#include <dstools/ExecutionProvider.h>
#include <dstools/ModelManager.h>

namespace dstools {

    MinLabelPage::MinLabelPage(QWidget *parent) : EditorPageBase("MinLabel", parent) {
        setBatchTaskKey(QStringLiteral("asr"));
        m_editor = new Minlabel::MinLabelEditor(this);

        setupBaseLayout(m_editor);

        setupNavigationActions();

        m_playAction = new QAction(tr("Play/Pause"), this);
        m_playAction->setIcon(QIcon(QStringLiteral(":/icons/play.svg")));
        m_asrAction = new QAction(tr("ASR Recognize Current"), this);
        connect(m_asrAction, &QAction::triggered, this, &MinLabelPage::onRunAsr);

        m_lyricFaAction = new QAction(tr("LyricFA Match Current"), this);
        connect(m_lyricFaAction, &QAction::triggered, this, &MinLabelPage::onRunLyricFa);

        shortcutManager()->bind(m_playAction, dstools::settings::kShortcutPlayPause, tr("Play/Pause"), tr("Playback"));
        shortcutManager()->bind(m_asrAction, dstools::settings::kShortcutASR, tr("ASR Recognize"), tr("Processing"));
        shortcutManager()->bind(m_lyricFaAction, dstools::settings::kShortcutLyricFA, tr("LyricFA Match"), tr("Processing"));
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

        m_undoAction = new QAction(tr("Undo"), this);
        m_redoAction = new QAction(tr("Redo"), this);

        connect(m_undoAction, &QAction::triggered, this, [this]() {
            auto *focused = QApplication::focusWidget();
            if (auto *textEdit = qobject_cast<QPlainTextEdit *>(focused))
                textEdit->undo();
            else if (auto *lineEdit = qobject_cast<QLineEdit *>(focused))
                lineEdit->undo();
        });
        connect(m_redoAction, &QAction::triggered, this, [this]() {
            auto *focused = QApplication::focusWidget();
            if (auto *textEdit = qobject_cast<QPlainTextEdit *>(focused))
                textEdit->redo();
            else if (auto *lineEdit = qobject_cast<QLineEdit *>(focused))
                lineEdit->redo();
        });

        m_undoAction->setEnabled(false);
        m_redoAction->setEnabled(false);

        if (auto *contentText = m_editor->textWidget()->contentText) {
            connect(contentText, &QPlainTextEdit::undoAvailable, this, [this](bool) { updateUndoRedoState(); });
            connect(contentText, &QPlainTextEdit::redoAvailable, this, [this](bool) { updateUndoRedoState(); });
        }
        if (auto *wordsText = m_editor->textWidget()->wordsText) {
            connect(wordsText, &QLineEdit::textChanged, this, [this]() { updateUndoRedoState(); });
        }

        m_editor->setPlaceholderText(tr("Select slices in the table above, then enter lyrics here.\n"
                                        "The top pane shows your selected regions; "
                                        "the bottom pane is where you transcribe."),
                                     tr("Enter raw lyrics here..."));
    }

    MinLabelPage::~MinLabelPage() = default;

    void MinLabelPage::updateUndoRedoState() {
        auto *contentText = m_editor->textWidget()->contentText;
        auto *wordsText = m_editor->textWidget()->wordsText;
        bool hasUndo =
            (contentText && contentText->document()->isUndoAvailable()) || (wordsText && wordsText->isUndoAvailable());
        bool hasRedo =
            (contentText && contentText->document()->isRedoAvailable()) || (wordsText && wordsText->isRedoAvailable());
        m_undoAction->setEnabled(hasUndo);
        m_redoAction->setEnabled(hasRedo);
    }

    bool MinLabelPage::hasExistingResult(const QString &sliceId) const {
        if (!source())
            return false;
        auto result = source()->loadSlice(sliceId);
        if (!result)
            return false;
        for (const auto &layer : result.value().layers) {
            if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme) && !layer.boundaries.empty())
                return true;
        }
        return false;
    }

    bool MinLabelPage::prepareSliceInput(const QString &sliceId, QString &skipReason) {
        if (!m_batchIsLyricFA)
            return EditorPageBase::prepareSliceInput(sliceId, skipReason);

        if (!source())
            return false;

        auto result = source()->loadSlice(sliceId);
        if (!result) {
            skipReason = tr("load failed");
            return false;
        }

        QString asrText;
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

        if (asrText.isEmpty()) {
            skipReason = tr("no input text");
            return false;
        }

        m_pendingLyricFaInput = asrText;
        return true;
    }

    BatchSliceResult MinLabelPage::processSlice(const QString &sliceId) {
        if (m_batchIsLyricFA) {
            auto *matchLyric = m_matchLyric.get();
            QString matchedText;
            QString msg;
            if (matchLyric->matchText(sliceId, m_pendingLyricFaInput, matchedText, msg)) {
                m_pendingLyricFaText = matchedText;
                return {BatchSliceResult::Processed, tr("[OK] %1: \"%2\"").arg(sliceId, matchedText.left(80))};
            }
            return {BatchSliceResult::Error, tr("[ERROR] %1: %2").arg(sliceId, msg.left(80))};
        }

        if (!aliveToken(QStringLiteral("asr")).isValid())
            return {BatchSliceResult::Error, tr("ASR model invalidated")};

        QString audioPath = source()->validatedAudioPath(sliceId);
        std::string msg;
        bool ok = m_asr->recognize(audioPath.toStdWString(), msg);
        if (ok) {
            m_pendingAsrText = QString::fromUtf8(msg);
            return {BatchSliceResult::Processed, tr("[OK] %1: \"%2\"").arg(sliceId, m_pendingAsrText.left(80))};
        }
        return {BatchSliceResult::Error,
                tr("[ERROR] %1: recognition failed - %2").arg(sliceId, QString::fromStdString(msg))};
    }

    void MinLabelPage::applyBatchResult(const QString &sliceId, const BatchSliceResult &result) {
        Q_UNUSED(result)
        if (m_batchIsLyricFA) {
            applyLyricFaResult(sliceId, m_pendingLyricFaText);
            return;
        }
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
        if (!result) {
            DSFW_LOG_ERROR(
                "minlabel",
                ("Failed to load slice for save: " + currentSliceId().toStdString() + " - " + result.error()).c_str());
            return false;
        }
        DsTextDocument doc = std::move(result.value());

        IntervalLayer *graphemeLayer = nullptr;
        for (auto &layer : doc.layers) {
            if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme)) {
                graphemeLayer = &layer;
                break;
            }
        }
        if (!graphemeLayer) {
            doc.layers.push_back({});
            graphemeLayer = &doc.layers.back();
            graphemeLayer->name = QString::fromUtf8(dstools::keys::layers::grapheme);
            graphemeLayer->type = QStringLiteral("text");
        }

        graphemeLayer->boundaries.clear();
        const QString lab = m_editor->labContent();
        if (!lab.isEmpty()) {
            static const QRegularExpression cjkRe(
                QStringLiteral("[\\x{4E00}-\\x{9FFF}\\x{3400}-\\x{4DBF}\\x{F900}-\\x{FAFF}]"));
            if (cjkRe.match(lab).hasMatch()) {
                DSFW_LOG_WARN("minlabel", "CJK characters detected in lyrics content");
                dsfw::widgets::ToastNotification::show(
                    this, dsfw::widgets::ToastType::Warning,
                    tr("TEXT contains Chinese characters, which may not be valid phonetic content."), 5000);
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
        markLayersModified({QString::fromUtf8(dstools::keys::layers::grapheme)});
        setLayerManuallyEdited(QString::fromUtf8(dstools::keys::layers::grapheme), true);
        addEditedStep(QStringLiteral("label_review"));
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
                if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme)) {
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

    void MinLabelPage::onAutoInferProcessDirty(const QStringList &dirty) {
        if (dirty.contains(QString::fromUtf8(dstools::keys::layers::grapheme))) {
            source()->clearDirtyLayers(currentSliceId(), {QString::fromUtf8(dstools::keys::layers::grapheme)});
        }
    }

    void MinLabelPage::onDeactivatedImpl() {
        enginePool()->invalidate(QStringLiteral("asr"));
        aliveToken(QStringLiteral("match_lyric")).invalidate();
        m_asr.reset();
    }

    // ── IPageActions ──────────────────────────────────────────────────────────────

    QMenuBar *MinLabelPage::createMenuBar(QWidget *parent) {
        auto *bar = new QMenuBar(parent);

        auto *fileMenu = addStandardFileMenu(bar, nullptr);
        // Override: use our own Save action instead
        fileMenu->clear();
        fileMenu->addAction(tr("Save"), QKeySequence::Save, this, [this]() { saveCurrentSlice(); });
        fileMenu->addSeparator();
        fileMenu->addAction(tr("E&xit"), this, [this]() {
            if (auto *w = window())
                w->close();
        });

        addStandardEditMenu(bar, m_undoAction, m_redoAction);

        auto *playMenu = bar->addMenu(tr("&Playback"));
        playMenu->addAction(m_playAction);

        auto *processMenu = bar->addMenu(tr("P&rocessing"));
        processMenu->addAction(m_asrAction);
        processMenu->addSeparator();
        processMenu->addAction(tr("Batch ASR..."), this, &MinLabelPage::onBatchAsr);
        processMenu->addSeparator();
        processMenu->addAction(m_lyricFaAction);
        processMenu->addAction(tr("Batch LyricFA..."), this, &MinLabelPage::onBatchLyricFa);

        addStandardViewMenu(bar);

        return bar;
    }

    QWidget *MinLabelPage::createStatusBarContent(QWidget *parent) {
        auto *container = new QWidget(parent);
        buildStandardStatusBar(container);
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
        EditorPageBase::updateProgress({QString::fromUtf8(dstools::keys::layers::grapheme)});
    }

    void MinLabelPage::onEngineInvalidated(const QString &taskKey) {
        enginePool()->invalidate(taskKey);
        if (taskKey == QStringLiteral("asr")) {
            m_asr.reset();
            DSFW_LOG_WARN("infer", "ASR task cancelled: model invalidated");
        }
    }

    void MinLabelPage::onRunAsr() {
        if (currentSliceId().isEmpty()) {
            QMessageBox::information(this, tr("ASR"), tr("Please select a slice first."));
            return;
        }

        m_asr = enginePool()->acquire<LyricFA::Asr>(QStringLiteral("asr"));
        if (!enginePool()->isOpen<LyricFA::Asr>(QStringLiteral("asr"))) {
            QMessageBox::warning(this, tr("ASR"),
                                 tr("ASR model not loaded. Please configure the ASR model path in Settings."));
            return;
        }

        if (isBatchRunning()) {
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

        setBatchRunning(true);
        DSFW_LOG_INFO("infer", ("ASR started: " + sliceId.toStdString()).c_str());
        std::weak_ptr<LyricFA::Asr> weakAsr = m_asr;

        runAsyncTask<QString>(
            QStringLiteral("asr"), sliceId,
            [weakAsr, audioPath](const std::shared_ptr<std::atomic<bool>> &) -> Result<QString> {
                auto asr = weakAsr.lock();
                if (!asr)
                    return Err<QString>("ASR engine is null");
                std::string msg;
                if (asr->recognize(audioPath.toStdWString(), msg))
                    return QString::fromUtf8(msg);
                return Err<QString>(std::move(msg));
            },
            [this](const QString &sliceId, const Result<QString> &result) {
                setBatchRunning(false);
                if (result) {
                    setAsrResult(sliceId, result.value());
                    DSFW_LOG_INFO("infer", ("ASR completed: " + sliceId.toStdString() + " - \"" +
                                            result.value().left(50).toStdString() + "\"")
                                               .c_str());
                    dsfw::widgets::ToastNotification::show(this, dsfw::widgets::ToastType::Info,
                                                           tr("ASR recognition completed"), 3000);
                } else {
                    DSFW_LOG_ERROR("infer", ("ASR failed: " + sliceId.toStdString() + " - " + result.error()).c_str());
                    QMessageBox::warning(this, tr("ASR"),
                                         tr("ASR recognition failed: %1").arg(QString::fromStdString(result.error())));
                }
            });
    }

    void MinLabelPage::onBatchAsr() {
        if (!source()) {
            QMessageBox::warning(this, tr("Batch ASR"), tr("Please open a project first."));
            return;
        }

        m_asr = enginePool()->acquire<LyricFA::Asr>(QStringLiteral("asr"));
        if (!enginePool()->isOpen<LyricFA::Asr>(QStringLiteral("asr"))) {
            QMessageBox::warning(this, tr("Batch ASR"),
                                 tr("ASR model not loaded. Please configure the ASR model path in Settings."));
            return;
        }

        if (isBatchRunning()) {
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

        if (auto res = source()->saveSlice(sliceId, doc); !res.ok())
            DSFW_LOG_ERROR("minlabel",
                           ("Failed to save slice after G2P: " + sliceId.toStdString() + " - " + res.error()).c_str());

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
        const auto pinyinStr = g2pMan.hanziToPinyin(rawText.toUtf8().toStdString(), Pinyin::ManTone::TONE3,
                                                    Pinyin::Error::Default, false, false);

        const QString phonemeStr = QString::fromStdString(pinyinStr.toStdStr()).trimmed();
        if (phonemeStr.isEmpty())
            return;

        // Write grapheme layer
        IntervalLayer *graphemeLayer = nullptr;
        for (auto &layer : doc.layers) {
            if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme)) {
                graphemeLayer = &layer;
                break;
            }
        }
        if (!graphemeLayer) {
            doc.layers.push_back({});
            graphemeLayer = &doc.layers.back();
            graphemeLayer->name = QString::fromUtf8(dstools::keys::layers::grapheme);
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

        if (auto res = source()->saveSlice(sliceId, doc); !res.ok())
            DSFW_LOG_ERROR(
                "minlabel",
                ("Failed to save slice after batch G2P: " + sliceId.toStdString() + " - " + res.error()).c_str());

        if (sliceId == currentSliceId()) {
            m_editor->loadData(phonemeStr, m_editor->rawText());
            m_dirty = true;
        }
    }

    // ── LyricFA ──────────────────────────────────────────────────────────────────

    void MinLabelPage::ensureLyricLibrary() {
        if (m_matchLyric && m_matchLyric->isInitialized())
            return;

        QString lyricDir = settings().get(settings::kLyricDir);

        if (lyricDir.isEmpty() || !QDir(lyricDir).exists()) {
            dsfw::widgets::FilePathSelector selector(
                {dsfw::widgets::FilePathSelector::Mode::OpenDirectory, tr("Select Lyric Library Directory"), {}, {}, lyricDir},
                this);
            lyricDir = selector.exec();
            if (lyricDir.isEmpty())
                return;
            settings().set(settings::kLyricDir, lyricDir);
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

        m_batchIsLyricFA = true;
        setBatchTaskKey(QStringLiteral("match_lyric"));

        BatchConfig config;
        config.dialogTitle = tr("Batch LyricFA");
        config.skipExistingLabel = tr("Skip slices with existing lyrics");
        config.defaultSkipExisting = true;

        runBatchProcess(config, ids);

        m_batchIsLyricFA = false;
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

        auto saveResult = source()->saveSlice(sliceId, doc);
        if (!saveResult) {
            DSFW_LOG_ERROR("minlabel", ("Failed to save slice in batch Lyric FA: " + sliceId.toStdString() + " - " +
                                        saveResult.error())
                                           .c_str());
        }

        if (sliceId == currentSliceId()) {
            m_editor->textWidget()->wordsText->setText(matchedText);
            m_editor->autoG2P();
            m_dirty = true;
            updateDirtyIndicator();
        }

        updateProgress();
    }

} // namespace dstools
