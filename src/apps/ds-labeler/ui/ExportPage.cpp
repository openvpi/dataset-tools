#include "ExportPage.h"

#include "AutoCompleteService.h"
#include "CompositeInferenceService.h"
#include "EnginePool.h"
#include "ExportService.h"
#include "InferBridge.h"
#include "SlicePreviewModel.h"
#include "core/ProjectDataSource.h"

#include <dsfw/widgets/FilePathSelector.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>
#include <dsfw/Log.h>
#include <dsfw/PipelineContext.h>
#include <dsfw/PipelineRunner.h>
#include <dsfw/Theme.h>
#include <dstools/CsvAdapter.h>
#include <dsfw/Constants.h>
#include <dstools/DsDocument.h>
#include <dstools/DsProject.h>
#include <dstools/DsTextTypes.h>
#include <dsfw/ExecutionProvider.h>
#include <dstools/PhNumCalculator.h>
#include <dstools/ProjectPaths.h>

namespace dstools {

    ExportPage::ExportPage(QWidget *parent) : QWidget(parent) {
        m_phNumCalc = std::make_unique<PhNumCalculator>();
        m_previewData = std::make_shared<SlicePreviewModel>();
        buildUi();
    }

    ExportPage::~ExportPage() = default;

    void ExportPage::buildUi() {
        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(24, 24, 24, 24);

        auto *titleLabel = new QLabel(QStringLiteral("导出 DiffSinger 数据集"), this);
        auto titleFont = titleLabel->font();
        titleFont.setPointSize(16);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        mainLayout->addWidget(titleLabel);
        mainLayout->addSpacing(12);

        m_tabWidget = new QTabWidget(this);

        auto *settingsWidget = new QWidget(this);
        auto *settingsLayout = new QVBoxLayout(settingsWidget);
        settingsLayout->setContentsMargins(0, 12, 0, 0);

        auto *dirLayout = new QHBoxLayout;
        auto *dirLabel = new QLabel(QStringLiteral("目标文件夹:"), this);
        m_outputDir = new QLineEdit(this);
        m_btnBrowse = new QPushButton(QStringLiteral("浏览"), this);
        dirLayout->addWidget(dirLabel);
        dirLayout->addWidget(m_outputDir, 1);
        dirLayout->addWidget(m_btnBrowse);
        settingsLayout->addLayout(dirLayout);
        settingsLayout->addSpacing(12);

        auto *contentGroup = new QGroupBox(QStringLiteral("导出内容"), this);
        auto *contentLayout = new QVBoxLayout(contentGroup);
        m_chkCsv = new QCheckBox(QStringLiteral("transcriptions.csv"), contentGroup);
        m_chkCsv->setChecked(true);
        m_chkDs = new QCheckBox(QStringLiteral("ds/ 文件夹 (.ds 训练文件)"), contentGroup);
        m_chkDs->setChecked(true);
        m_chkWavs = new QCheckBox(QStringLiteral("wavs/ 文件夹"), contentGroup);
        m_chkWavs->setChecked(true);
        contentLayout->addWidget(m_chkCsv);
        contentLayout->addWidget(m_chkDs);
        contentLayout->addWidget(m_chkWavs);
        settingsLayout->addWidget(contentGroup);
        settingsLayout->addSpacing(12);

        auto *audioGroup = new QGroupBox(QStringLiteral("音频设置"), this);
        auto *audioLayout = new QFormLayout(audioGroup);
        m_sampleRate = new QSpinBox(audioGroup);
        m_sampleRate->setRange(8000, 96000);
        m_sampleRate->setValue(constants::kDefaultSampleRate);
        m_sampleRate->setSuffix(QStringLiteral(" Hz"));
        audioLayout->addRow(QStringLiteral("采样率:"), m_sampleRate);
        m_chkResample = new QCheckBox(QStringLiteral("需要时重采样（使用 soxr）"), audioGroup);
        audioLayout->addRow(m_chkResample);
        settingsLayout->addWidget(audioGroup);
        settingsLayout->addSpacing(12);

        auto *advGroup = new QGroupBox(QStringLiteral("高级"), this);
        auto *advLayout = new QFormLayout(advGroup);
        m_hopSize = new QSpinBox(advGroup);
        m_hopSize->setRange(64, 2048);
        m_hopSize->setValue(512);
        advLayout->addRow(QStringLiteral("hop_size:"), m_hopSize);
        m_chkIncludeDiscarded = new QCheckBox(QStringLiteral("包含丢弃的切片"), advGroup);
        advLayout->addRow(m_chkIncludeDiscarded);
        m_chkAutoComplete = new QCheckBox(QStringLiteral("自动补全缺失步骤 (FA/ph_num/F0/MIDI)"), advGroup);
        m_chkAutoComplete->setChecked(true);
        m_chkAutoComplete->setToolTip(QStringLiteral("导出前自动运行缺失的推理步骤：\n"
                                                     "• 缺少 phoneme 层 → 运行 HubertFA\n"
                                                     "• 缺少 ph_num 层 → 计算 ph_num\n"
                                                     "• 缺少 pitch 曲线 → 运行 RMVPE\n"
                                                     "• 缺少 midi 层 → 运行 GAME"));
        advLayout->addRow(m_chkAutoComplete);
        settingsLayout->addWidget(advGroup);
        settingsLayout->addSpacing(12);

        m_validationLabel = new QLabel(this);
        m_validationLabel->setWordWrap(true);
        m_validationLabel->setObjectName(QStringLiteral("validationLabel"));
        m_validationLabel->setStyleSheet(QStringLiteral("#validationLabel { padding: 8px; border-radius: 4px; }"));
        settingsLayout->addWidget(m_validationLabel);
        settingsLayout->addSpacing(12);

        auto *btnLayout = new QHBoxLayout;
        btnLayout->addStretch();
        m_btnExport = new QPushButton(QStringLiteral("开始导出"), this);
        m_btnExport->setMinimumSize(160, 40);
        m_btnExport->setEnabled(false);
        btnLayout->addWidget(m_btnExport);
        btnLayout->addStretch();
        settingsLayout->addLayout(btnLayout);
        settingsLayout->addSpacing(12);

        m_progressBar = new QProgressBar(this);
        m_progressBar->setVisible(false);
        settingsLayout->addWidget(m_progressBar);

        m_statusLabel = new QLabel(this);
        m_statusLabel->setAlignment(Qt::AlignCenter);
        m_statusLabel->setObjectName(QStringLiteral("statusLabel"));
        m_statusLabel->setStyleSheet(QStringLiteral("#statusLabel { color: palette(text); opacity: 0.6; }"));
        settingsLayout->addWidget(m_statusLabel);

        settingsLayout->addStretch();

        m_tabWidget->addTab(settingsWidget, QStringLiteral("导出设置"));

        buildScanProgressTab();

        buildPreviewTab();

        mainLayout->addWidget(m_tabWidget);

        connect(m_btnBrowse, &QPushButton::clicked, this, &ExportPage::onBrowseOutput);
        connect(m_btnExport, &QPushButton::clicked, this, &ExportPage::onExport);
        connect(m_outputDir, &QLineEdit::textChanged, this, [this]() { updateExportButton(); });
        connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
            if (index == 2)
                refreshPreview();
        });
    }

    void ExportPage::buildPreviewTab() {
        auto *previewWidget = new QWidget(this);
        auto *previewLayout = new QVBoxLayout(previewWidget);
        previewLayout->setContentsMargins(0, 0, 0, 0);

        auto *toolbarLayout = new QHBoxLayout();
        toolbarLayout->setContentsMargins(4, 4, 4, 4);

        m_previewModel = new QStandardItemModel(this);

        m_sortProxy = new QSortFilterProxyModel(this);
        m_sortProxy->setSourceModel(m_previewModel);
        m_sortProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_sortProxy->setFilterKeyColumn(-1);

        m_previewTable = new QTableView(this);
        m_previewTable->setObjectName(QStringLiteral("csvPreviewTable"));
        m_previewTable->setModel(m_sortProxy);
        m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_previewTable->setAlternatingRowColors(true);
        m_previewTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_previewTable->setSortingEnabled(true);
        m_previewTable->horizontalHeader()->setStretchLastSection(true);

        auto *filterLabel = new QLabel(tr("Filter:"), this);
        toolbarLayout->addWidget(filterLabel);

        m_filterEdit = new QLineEdit(this);
        m_filterEdit->setPlaceholderText(tr("输入文本过滤..."));
        m_filterEdit->setClearButtonEnabled(true);
        m_filterEdit->setMaximumWidth(250);
        connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
            m_sortProxy->setFilterFixedString(text);
            bool hasFilter = !text.isEmpty();
            if (m_clearFilterBtn)
                m_clearFilterBtn->setVisible(hasFilter);
        });
        toolbarLayout->addWidget(m_filterEdit);

        // Column filter toolbar
        auto *colFilterBtn = new QToolButton(this);
        colFilterBtn->setText(tr("▤ Columns"));
        colFilterBtn->setPopupMode(QToolButton::InstantPopup);
        auto *colMenu = new QMenu(colFilterBtn);
        const QStringList colNames = {
            QStringLiteral("name"), QStringLiteral("ph_seq"), QStringLiteral("ph_dur"),
            QStringLiteral("ph_num"), QStringLiteral("note_seq"), QStringLiteral("note_dur"),
            QStringLiteral("word_span"), QStringLiteral("dirty"),
        };
        for (int i = 0; i < colNames.size(); ++i) {
            auto *act = colMenu->addAction(colNames[i]);
            act->setCheckable(true);
            act->setChecked(true);
            act->setData(i);
            connect(act, &QAction::toggled, this, [this, i](bool checked) {
                toggleColumnFilter(i);
            });
        }
        colMenu->addSeparator();
        colMenu->addAction(tr("Show All"), this, [this]() { clearFilters(); });
        colFilterBtn->setMenu(colMenu);
        toolbarLayout->addWidget(colFilterBtn);

        auto *clearFilterBtn = new QPushButton(tr("Clear Filter"));
        clearFilterBtn->setVisible(false);
        connect(clearFilterBtn, &QPushButton::clicked, this, [this, clearFilterBtn]() {
            clearFilters();
            clearFilterBtn->setVisible(false);
        });
        toolbarLayout->addWidget(clearFilterBtn);
        m_clearFilterBtn = clearFilterBtn;

        m_loadingLabel = new QLabel(this);
        m_loadingLabel->setAlignment(Qt::AlignCenter);
        m_loadingLabel->setStyleSheet(QStringLiteral("color: palette(text); opacity: 0.5; padding: 4px;"));
        m_loadingLabel->setVisible(false);
        toolbarLayout->addWidget(m_loadingLabel);

        toolbarLayout->addStretch();

        previewLayout->addLayout(toolbarLayout);
        previewLayout->addWidget(m_previewTable);

        m_tabWidget->addTab(previewWidget, QStringLiteral("预览数据"));
    }

    void ExportPage::buildScanProgressTab() {
        auto *scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);

        m_coverageWidget = new QWidget(this);
        auto *coverageLayout = new QVBoxLayout(m_coverageWidget);
        coverageLayout->setContentsMargins(0, 12, 0, 0);

        auto *summaryRow = new QHBoxLayout;
        m_summaryLabel = new QLabel(this);
        m_summaryLabel->setWordWrap(true);
        m_summaryLabel->setObjectName(QStringLiteral("summaryLabel"));
        m_summaryLabel->setStyleSheet(QStringLiteral("#summaryLabel { font-size: 13px; padding: 4px 0; }"));
        summaryRow->addWidget(m_summaryLabel);
        summaryRow->addStretch();
        auto *btnRescan = new QPushButton(tr("重新扫描"), this);
        btnRescan->setFixedWidth(100);
        connect(btnRescan, &QPushButton::clicked, this, &ExportPage::runValidation);
        summaryRow->addWidget(btnRescan);
        coverageLayout->addLayout(summaryRow);

        static const QStringList kColNames = {
            QStringLiteral("name"),
            QStringLiteral("ph_seq"),
            QStringLiteral("ph_dur"),
            QStringLiteral("ph_num"),
            QStringLiteral("note_seq"),
            QStringLiteral("note_dur"),
            QStringLiteral("note_slur"),
        };
        for (const auto &colName : kColNames) {
            auto *row = new QHBoxLayout;
            auto *label = new QLabel(colName, this);
            label->setMinimumWidth(80);
            label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            label->setCursor(Qt::PointingHandCursor);
            label->setProperty("coverageColumn", colName);
            label->installEventFilter(this);
            row->addWidget(label);

            auto *bar = new QProgressBar(this);
            bar->setRange(0, 100);
            bar->setValue(0);
            bar->setTextVisible(false);
            bar->setMaximumHeight(16);
            bar->setObjectName(QStringLiteral("coverageBar"));
            bar->setCursor(Qt::PointingHandCursor);
            bar->setProperty("coverageColumn", colName);
            bar->installEventFilter(this);
            row->addWidget(bar, 1);

            auto *pctLabel = new QLabel(QStringLiteral("—"), this);
            pctLabel->setMinimumWidth(50);
            pctLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pctLabel->setCursor(Qt::PointingHandCursor);
            pctLabel->setProperty("coverageColumn", colName);
            pctLabel->installEventFilter(this);
            row->addWidget(pctLabel);

            coverageLayout->addLayout(row);

            ColumnCoverage cc;
            cc.name = colName;
            cc.bar = bar;
            cc.pctLabel = pctLabel;
            m_columnCoverages.push_back(cc);
        }

        coverageLayout->addStretch();
        m_coverageWidget->setVisible(false);

        scrollArea->setWidget(m_coverageWidget);
        m_tabWidget->addTab(scrollArea, QStringLiteral("扫描进度"));
    }

    void ExportPage::refreshPreview() {
        m_previewModel->clear();

        if (!m_source)
            return;

        QStringList headers = {
            QStringLiteral("name"),     QStringLiteral("ph_seq"),
            QStringLiteral("ph_dur"),   QStringLiteral("ph_num"),
            QStringLiteral("note_seq"), QStringLiteral("note_dur"),
            QStringLiteral("word_span"), QStringLiteral("dirty"),
        };
        m_previewModel->setHorizontalHeaderLabels(headers);

        if (m_previewLoading)
            return;

        m_previewLoading = true;
        if (m_loadingLabel) {
            m_loadingLabel->setText(tr("⏳ 正在加载预览数据..."));
            m_loadingLabel->setVisible(true);
        }

        m_previewData->setDataSource(m_source);
        m_previewData->invalidate();

        QPointer<ExportPage> guard(this);
        auto preview = m_previewData;  // shared_ptr copy — safe across threads

        (void)QtConcurrent::run([guard, preview]() {
            preview->refresh();

            if (!guard)
                return;

            const auto &rows = preview->rows();

            QMetaObject::invokeMethod(
                guard.data(),
                [guard, rows]() {
                    if (guard) {
                        guard->populatePreviewModel(rows);
                        guard->m_previewLoading = false;
                        if (guard->m_loadingLabel)
                            guard->m_loadingLabel->setVisible(false);
                    }
                },
                Qt::QueuedConnection);
        });
    }

    void ExportPage::setDataSource(ProjectDataSource *source) {
        m_source = source;
        m_previewData->invalidate();
        updateExportButton();
    }

    void ExportPage::setEnginePool(std::unique_ptr<EnginePool> pool) { m_enginePool = std::move(pool); }

    void ExportPage::onBrowseOutput() {
        dsfw::widgets::FilePathSelector selector(
            {dsfw::widgets::FilePathSelector::Mode::OpenDirectory, QStringLiteral("选择导出目录")}, this);
        const QString dir = selector.exec();
        if (!dir.isEmpty()) {
            m_outputDir->setText(dir);
            updateExportButton();
        }
    }

    void ExportPage::updateExportButton() {
        bool hasOutput = !m_outputDir->text().trimmed().isEmpty();
        bool hasSource = m_source != nullptr;
        bool hasReadySlices = m_readyForCsv > 0;

        bool ok = hasSource && hasOutput && hasReadySlices;
        m_btnExport->setEnabled(ok);

        if (!hasSource)
            m_btnExport->setToolTip(QStringLiteral("请先打开工程"));
        else if (!hasOutput)
            m_btnExport->setToolTip(QStringLiteral("请选择目标文件夹"));
        else if (!hasReadySlices)
            m_btnExport->setToolTip(QStringLiteral("没有切片包含 grapheme 层，无法导出"));
        else
            m_btnExport->setToolTip({});
    }

    void ExportPage::ensureEngines() {
        if (!m_enginePool)
            return;

        m_enginePool->acquire<HFA::HFA>(QStringLiteral("phoneme_alignment"));
        m_enginePool->acquire<Rmvpe::Rmvpe>(QStringLiteral("pitch_extraction"));
        m_enginePool->acquire<Game::Game>(QStringLiteral("midi_transcription"));
    }

    void ExportPage::autoCompleteSlice(const QString &sliceId) {
    if (!m_source)
        return;

    auto result = m_source->loadSlice(sliceId);
    if (!result)
        return;

    QString audioPath = m_source->audioPath(sliceId);
        auto hfa = m_enginePool ? m_enginePool->acquire<HFA::HFA>(QStringLiteral("phoneme_alignment")) : nullptr;
        auto rmvpe = m_enginePool ? m_enginePool->acquire<Rmvpe::Rmvpe>(QStringLiteral("pitch_extraction")) : nullptr;
        auto game = m_enginePool ? m_enginePool->acquire<Game::Game>(QStringLiteral("midi_transcription")) : nullptr;
        dstools::CompositeInferenceService inferService(hfa.get(), rmvpe.get(), game.get());
        auto outcome = dstools::autoCompleteSlice(std::move(result.value()), audioPath,
                                                  &inferService, m_phNumCalc.get());
    if (outcome.modified) {
        if (auto res = m_source->saveSlice(sliceId, outcome.doc); !res.ok())
            DSFW_LOG_ERROR("export", ("Failed to save slice after autoComplete: " + sliceId.toStdString() + " - " + res.error()).c_str());
    }
}

    void ExportPage::populatePreviewModel(const std::vector<SlicePreviewRow> &rows) {
        m_previewModel->setRowCount(0);

        auto warnColor = dsfw::Theme::instance().palette().warning;
        auto dirtyBg = warnColor;
        dirtyBg.setAlpha(40);
        auto missingBg = warnColor;
        missingBg.setAlpha(60);

        for (const auto &row : rows) {
            QList<QStandardItem *> items;
            auto *nameItem = new QStandardItem(row.name);
            items.append(nameItem);
            items.append(new QStandardItem(row.phSeq));
            items.append(new QStandardItem(row.phDur));
            items.append(new QStandardItem(row.phNum));
            items.append(new QStandardItem(row.noteSeq));
            items.append(new QStandardItem(row.noteDur));
            items.append(new QStandardItem(row.wordSpan));

            auto *dirtyItem = new QStandardItem(
                row.dirty.isEmpty() ? QStringLiteral("✓") : row.dirty);
            if (!row.dirty.isEmpty()) {
                dirtyItem->setBackground(dirtyBg);
                dirtyItem->setToolTip(tr("该切片存在脏层，上游修改后未重算"));
                nameItem->setBackground(dirtyBg);
            }
            items.append(dirtyItem);

            if (row.hasMissing) {
                for (auto *item : items)
                    item->setBackground(missingBg);
            }

            m_previewModel->appendRow(items);
        }

        m_previewTable->resizeColumnsToContents();
    }

    void ExportPage::toggleColumnFilter(int column) {
        if (!m_previewTable || column < 0)
            return;
        bool hidden = !m_previewTable->isColumnHidden(column);
        m_previewTable->setColumnHidden(column, hidden);
    }

    void ExportPage::applyCoverageFilter(const QString &columnName) {
        if (!m_previewModel)
            return;

        int colIdx = -1;
        const QStringList colNames = {
            QStringLiteral("name"), QStringLiteral("ph_seq"), QStringLiteral("ph_dur"),
            QStringLiteral("ph_num"), QStringLiteral("note_seq"), QStringLiteral("note_dur"),
            QStringLiteral("word_span"), QStringLiteral("dirty"),
        };
        for (int i = 0; i < colNames.size(); ++i) {
            if (colNames[i] == columnName) {
                colIdx = i;
                break;
            }
        }
        if (colIdx < 0)
            return;

        for (int row = 0; row < m_previewModel->rowCount(); ++row) {
            auto *item = m_previewModel->item(row, colIdx);
            bool empty = !item || item->text().isEmpty() || item->text() == QStringLiteral("-");
            m_previewTable->setRowHidden(row, !empty);
        }

        if (m_clearFilterBtn)
            m_clearFilterBtn->setVisible(true);
    }

    void ExportPage::clearFilters() {
        if (!m_previewTable)
            return;
        for (int col = 0; col < m_previewModel->columnCount(); ++col)
            m_previewTable->setColumnHidden(col, false);
        for (int row = 0; row < m_previewModel->rowCount(); ++row)
            m_previewTable->setRowHidden(row, false);
        if (m_sortProxy)
            m_sortProxy->setFilterFixedString(QString());
        if (m_filterEdit)
            m_filterEdit->clear();
        if (m_clearFilterBtn)
            m_clearFilterBtn->setVisible(false);
    }

    bool ExportPage::eventFilter(QObject *obj, QEvent *event) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                auto colName = obj->property("coverageColumn").toString();
                if (!colName.isEmpty()) {
                    applyCoverageFilter(colName);
                    return true;
                }
            }
        }
        return QWidget::eventFilter(obj, event);
    }

    void ExportPage::onExport() {
        if (!m_source) {
            QMessageBox::warning(this, QStringLiteral("导出"), QStringLiteral("请先打开工程。"));
            return;
        }

        const auto allSliceIds = m_source->sliceIds();
        if (allSliceIds.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("导出"), QStringLiteral("没有可导出的切片。"));
            return;
        }

        QStringList sliceIds;
        int skippedDiscarded = 0;
        if (!m_chkIncludeDiscarded->isChecked()) {
            for (const auto &sid : allSliceIds) {
                auto *ctx = m_source->context(sid);
                if (ctx && ctx->status == PipelineContext::Status::Discarded) {
                    ++skippedDiscarded;
                    continue;
                }
                sliceIds.append(sid);
            }
        } else {
            sliceIds = allSliceIds;
        }

        if (sliceIds.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("导出"),
                                 tr("没有可导出的切片。%1 个切片已被丢弃，可勾选「包含丢弃的切片」来导出。")
                                     .arg(skippedDiscarded));
            return;
        }

        const QString outputDir = m_outputDir->text().trimmed();
        QDir().mkpath(outputDir);

        // Auto-complete missing steps if enabled (runs inference in background)
        if (m_chkAutoComplete->isChecked()) {
            ensureEngines();

            m_progressBar->setVisible(true);
            m_progressBar->setRange(0, sliceIds.size());

            QStringList summary;
            if (m_missingFa > 0)
                summary.append(tr("音素对齐 %1 个").arg(m_missingFa));
            if (m_missingPitch > 0)
                summary.append(tr("F0 提取 %1 个").arg(m_missingPitch));
            if (m_missingMidi > 0)
                summary.append(tr("MIDI 提取 %1 个").arg(m_missingMidi));
            if (m_missingPhNum > 0)
                summary.append(tr("发音数计算 %1 个").arg(m_missingPhNum));
            if (m_dirtyCount > 0)
                summary.append(tr("脏数据修复 %1 个").arg(m_dirtyCount));

            if (!summary.isEmpty())
                m_statusLabel->setText(tr("补全任务: %1").arg(summary.join(QStringLiteral(", "))));
            else
                m_statusLabel->setText(tr("所有切片已就绪，正在准备导出..."));
            m_btnExport->setEnabled(false);

            // Capture engine pointers and source for use in background thread.
            auto hfa = m_enginePool ? m_enginePool->acquire<HFA::HFA>(QStringLiteral("phoneme_alignment")) : nullptr;
            auto rmvpe = m_enginePool ? m_enginePool->acquire<Rmvpe::Rmvpe>(QStringLiteral("pitch_extraction")) : nullptr;
            auto game = m_enginePool ? m_enginePool->acquire<Game::Game>(QStringLiteral("midi_transcription")) : nullptr;
            auto *phNumCalc = m_phNumCalc.get();
            auto enginesAlive = m_enginePool ? m_enginePool->aliveToken(QStringLiteral("export")) : nullptr;
            auto *src = m_source;
            QPointer<ExportPage> guard(this);

            // Run inference loop in background; save + continue export on main thread
            (void) QtConcurrent::run([guard, hfa, rmvpe, game, phNumCalc, enginesAlive, src, sliceIds, outputDir]() {
                // Map of sliceId → modified DsTextDocument (only non-empty docs saved)
                std::map<QString, DsTextDocument> modifiedDocs;

                int processed = 0;
                for (const auto &sliceId : sliceIds) {
                    if (!guard)
                        return;
                    if (!enginesAlive || !*enginesAlive)
                        return;

                    try {
                        auto result = src->loadSlice(sliceId);
                        if (!result) {
                            ++processed;
                            QMetaObject::invokeMethod(
                                guard.data(),
                                [guard, processed, sliceId]() {
                                    if (guard) {
                                        guard->m_progressBar->setValue(processed);
                                        guard->m_statusLabel->setText(
                                            QStringLiteral("正在补全 ( %1/%2 ): %3")
                                                .arg(processed)
                                                .arg(guard->m_progressBar->maximum())
                                                .arg(sliceId));
                                    }
                                },
                                Qt::QueuedConnection);
                            continue;
                        }

                        DsTextDocument doc = std::move(result.value());
                        QString audioPath = src->audioPath(sliceId);
                        dstools::CompositeInferenceService inferService(hfa.get(), rmvpe.get(), game.get());
                        auto outcome = dstools::autoCompleteSlice(std::move(doc), audioPath, &inferService, phNumCalc);
                        if (outcome.modified)
                            modifiedDocs[sliceId] = std::move(outcome.doc);

                        ++processed;
                        // Update progress on main thread
                        QMetaObject::invokeMethod(
                            guard.data(),
                            [guard, processed, sliceId]() {
                                if (guard) {
                                    guard->m_progressBar->setValue(processed);
                                    guard->m_statusLabel->setText(
                                        QStringLiteral("正在补全 ( %1/%2 ): %3")
                                            .arg(processed)
                                            .arg(guard->m_progressBar->maximum())
                                            .arg(sliceId));
                                }
                            },
                            Qt::QueuedConnection);
                    } catch (const std::exception &e) {
                        DSFW_LOG_ERROR("infer",
                                       ("Inference failed for " + sliceId.toStdString() + ": " + e.what()).c_str());
                        ++processed;
                        QMetaObject::invokeMethod(
                            guard.data(),
                            [guard, processed]() {
                                if (guard) {
                                    guard->m_progressBar->setValue(processed);
                                }
                            },
                            Qt::QueuedConnection);
                    }
                }

                // All inference done — save results and continue export on main thread
                QMetaObject::invokeMethod(
                    guard.data(),
                    [guard, modifiedDocs = std::move(modifiedDocs), sliceIds, outputDir]() mutable {
                        if (!guard)
                            return;

                        // Save modified docs
                        int saved = 0;
                        for (auto &[sliceId, doc] : modifiedDocs) {
                            if (auto res = guard->m_source->saveSlice(sliceId, doc); !res.ok())
                                DSFW_LOG_ERROR("export", ("Failed to save slice in batch export: " + sliceId.toStdString() + " - " + res.error()).c_str());
                            else
                                ++saved;
                        }

                        guard->m_progressBar->setVisible(false);
                        if (saved > 0)
                            guard->m_statusLabel->setText(
                                tr("补全完成: %1 个切片有缺失数据已填充").arg(saved));
                        else
                            guard->m_statusLabel->setText(tr("无需补全，所有切片已就绪"));

                        // Continue with export
                        guard->continueExport(sliceIds, outputDir);
                    },
                    Qt::QueuedConnection);
            });

            // Return early — the rest of export continues via continueExport()
            return;
        }

        // If autoComplete is off, proceed directly to export
        continueExport(sliceIds, outputDir);
    }

    void ExportPage::continueExport(const QStringList &sliceIds, const QString &outputDir) {
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, sliceIds.size());
        m_progressBar->setValue(0);

        m_statusLabel->setText(QStringLiteral("正在导出..."));
        m_btnExport->setEnabled(false);

        const QString wavsDir = ProjectPaths::wavsDir(outputDir);
        const QString dsDir = ProjectPaths::dsDir(outputDir);
        if (m_chkWavs->isChecked())
            QDir().mkpath(wavsDir);
        if (m_chkDs->isChecked())
            QDir().mkpath(dsDir);

        bool hasExistingFiles = false;
        for (const auto &sliceId : sliceIds) {
            if (m_chkWavs->isChecked()) {
                QString wavPath = QDir(wavsDir).filePath(sliceId + QStringLiteral(".wav"));
                if (QFile::exists(wavPath)) { hasExistingFiles = true; break; }
            }
            if (m_chkDs->isChecked()) {
                QString dsPath = QDir(dsDir).filePath(sliceId + QStringLiteral(".ds"));
                if (QFile::exists(dsPath)) { hasExistingFiles = true; break; }
            }
        }

        if (hasExistingFiles) {
            QString backupDir = outputDir + QStringLiteral("/backup_")
                        + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
            QDir().mkpath(backupDir);
            m_statusLabel->setText(tr("正在备份已有文件到 %1...").arg(
                QFileInfo(backupDir).fileName()));

            bool chkWavsChecked = m_chkWavs->isChecked();
            bool chkDsChecked = m_chkDs->isChecked();
            auto alive = m_enginePool ? m_enginePool->aliveToken(QStringLiteral("export")) : nullptr;
            QPointer<ExportPage> self(this);

            QtConcurrent::run([alive, sliceIds, backupDir, wavsDir, dsDir,
                               chkWavsChecked, chkDsChecked]() {
                if (!alive || !*alive)
                    return;
                int backedUp = 0;
                for (const auto &sliceId : sliceIds) {
                    if (chkWavsChecked) {
                        QString wavPath = QDir(wavsDir).filePath(sliceId + QStringLiteral(".wav"));
                        if (QFile::exists(wavPath)) {
                            QFile::copy(wavPath, QDir(backupDir).filePath(sliceId + QStringLiteral(".wav")));
                            ++backedUp;
                        }
                    }
                    if (chkDsChecked) {
                        QString dsPath = QDir(dsDir).filePath(sliceId + QStringLiteral(".ds"));
                        if (QFile::exists(dsPath)) {
                            QFile::copy(dsPath, QDir(backupDir).filePath(sliceId + QStringLiteral(".ds")));
                            ++backedUp;
                        }
                    }
                }
                DSFW_LOG_INFO("export", ("Backed up " + std::to_string(backedUp) + " files to " + backupDir.toStdString()).c_str());
            });

            QMetaObject::invokeMethod(self, [this, sliceIds, outputDir, wavsDir, dsDir]() {
                performExport(sliceIds, outputDir, wavsDir, dsDir);
            }, Qt::QueuedConnection);
            return;
        }

        performExport(sliceIds, outputDir, wavsDir, dsDir);
    }

    void ExportPage::performExport(const QStringList &sliceIds, const QString &outputDir,
                                   const QString &wavsDir, const QString &dsDir) {
        std::vector<TranscriptionRow> csvRows;
        int processed = 0;
        int errors = 0;
        QStringList exportErrors;
        for (const auto &sliceId : sliceIds) {
            m_statusLabel->setText(QStringLiteral("正在导出: %1").arg(sliceId));
            m_progressBar->setValue(++processed);

            auto result = m_source->loadSlice(sliceId);
            if (!result) {
                ++errors;
                continue;
            }

            const DsTextDocument &doc = result.value();
            auto *ctx = m_source->context(sliceId);

            if (m_chkWavs->isChecked()) {
                QString srcAudio = m_source->audioPath(sliceId);
                if (QFile::exists(srcAudio)) {
                    QString dstName = sliceId + QStringLiteral(".wav");
                    QString dstPath = QDir(wavsDir).filePath(dstName);
                    if (!QFile::copy(srcAudio, dstPath)) {
                        if (!QFile::remove(dstPath) || !QFile::copy(srcAudio, dstPath)) {
                            ++errors;
                            if (exportErrors.size() < 3)
                                exportErrors.append(sliceId + QStringLiteral(": WAV copy failed"));
                        }
                    }
                }
            }

            if (m_chkDs->isChecked() && ctx) {
                if (ctx->editedSteps.contains(QStringLiteral("pitch_review"))) {
                    DsDocument dsDoc;
                    nlohmann::json sentence;
                    sentence["name"] = sliceId.toStdString();
                    for (const auto &layer : doc.layers) {
                        std::vector<double> positions;
                        std::vector<std::string> texts;
                        for (const auto &b : layer.boundaries) {
                            positions.push_back(b.pos);
                            texts.push_back(b.text.toStdString());
                        }
                        if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive)) {
                            sentence["ph_seq"] = texts;
                            std::vector<double> durs;
                            for (size_t i = 1; i < positions.size(); ++i)
                                durs.push_back(positions[i] - positions[i - 1]);
                            sentence["ph_dur"] = durs;
                        } else if (layer.name.contains(QStringLiteral("note"), Qt::CaseInsensitive)) {
                            sentence["note_seq"] = texts;
                        }
                    }
                    dsDoc.addRawSentence(sentence.dump());
                    auto saveResult = dsDoc.saveFile(QDir(dsDir).filePath(sliceId + QStringLiteral(".ds")));
                    if (!saveResult) {
                        ++errors;
                        if (exportErrors.size() < 3)
                            exportErrors.append(sliceId + QStringLiteral(": ") + QString::fromStdString(saveResult.error()));
                    }
                }
            }

            if (m_chkCsv->isChecked()) {
                TranscriptionRow row;
                row.name = sliceId;

                const IntervalLayer *phonemeLayer = nullptr;
                const IntervalLayer *graphemeLayer = nullptr;
                const IntervalLayer *midiLayer = nullptr;
                const IntervalLayer *phNumLayer = nullptr;
                for (const auto &layer : doc.layers) {
                    if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive))
                        phonemeLayer = &layer;
                    else if (layer.name == QStringLiteral("grapheme"))
                        graphemeLayer = &layer;
                    else if (layer.name.contains(QStringLiteral("midi"), Qt::CaseInsensitive)
                             || layer.name.contains(QStringLiteral("note"), Qt::CaseInsensitive))
                        midiLayer = &layer;
                    else if (layer.name == QStringLiteral("ph_num"))
                        phNumLayer = &layer;
                }

                if (phonemeLayer) {
                    QStringList phs, durs;
                    const size_t phCount = phonemeLayer->boundaries.size();
                    for (size_t i = 0; i < phCount; ++i) {
                        phs << phonemeLayer->boundaries[i].text;
                        if (i + 1 < phCount) {
                            double dur = usToSec(phonemeLayer->boundaries[i + 1].pos - phonemeLayer->boundaries[i].pos);
                            durs << QString::number(dur, 'f', 6);
                        }
                    }
                    row.phSeq = phs.join(' ');
                    row.phDur = durs.join(' ');

                    if (graphemeLayer && !graphemeLayer->boundaries.empty()) {
                        QStringList spans;
                        int wordIdx = 1;
                        for (size_t i = 0; i < phCount; ++i) {
                            const auto &ph = phonemeLayer->boundaries[i];
                            if (ph.text.isEmpty())
                                continue;
                            while (wordIdx < static_cast<int>(graphemeLayer->boundaries.size()) &&
                                   ph.pos >= graphemeLayer->boundaries[wordIdx].pos) {
                                ++wordIdx;
                            }
                            spans << QString::number(wordIdx);
                        }
                        row.wordSpan = spans.join(' ');
                    }
                }

                if (phNumLayer) {
                    QStringList nums;
                    for (const auto &b : phNumLayer->boundaries) {
                        if (!b.text.isEmpty())
                            nums << b.text;
                    }
                    row.phNum = nums.join(' ');
                }

                if (midiLayer) {
                    QStringList notes;
                    QStringList noteDurs;
                    for (const auto &b : midiLayer->boundaries) {
                        QJsonParseError err;
                        auto jdoc = QJsonDocument::fromJson(b.text.toUtf8(), &err);
                        if (err.error == QJsonParseError::NoError && jdoc.isObject()) {
                            auto obj = jdoc.object();
                            notes << obj["n"].toString();
                            double durSec = usToSec(static_cast<TimePos>(obj["d"].toDouble()));
                            noteDurs << QString::number(durSec, 'f', 6);
                        } else {
                            notes << b.text;
                            noteDurs << QStringLiteral("0");
                        }
                    }
                    row.noteSeq = notes.join(' ');
                    row.noteDur = noteDurs.join(' ');
                }

                if (!row.phSeq.isEmpty())
                    csvRows.push_back(std::move(row));
            }
        }

        if (m_chkCsv->isChecked() && !csvRows.empty()) {
            auto writeResult = CsvAdapter::writeRows(outputDir + QStringLiteral("/transcriptions.csv"), csvRows);
            if (!writeResult.ok()) {
                ++errors;
                if (exportErrors.size() < 3)
                    exportErrors.append(QStringLiteral("CSV: ") + QString::fromStdString(writeResult.error()));
            }
        }

        m_progressBar->setVisible(false);
        m_statusLabel->setText(QStringLiteral("导出完成: %1 个切片 → %2").arg(processed).arg(outputDir));
        m_btnExport->setEnabled(true);

        if (errors > 0) {
            QString msg = QStringLiteral("导出完成，但有 %1 个错误。").arg(errors);
            if (!exportErrors.isEmpty()) {
                msg += QStringLiteral("\n\n错误详情（前 %1 条）:\n").arg(exportErrors.size());
                for (int i = 0; i < static_cast<int>(exportErrors.size()); ++i)
                    msg += QStringLiteral("  %1. %2\n").arg(i + 1).arg(exportErrors[i]);
            }
            msg += QStringLiteral("\n输出目录:\n%1").arg(outputDir);
            QMessageBox::warning(this, QStringLiteral("导出完成"), msg);
        } else {
            QMessageBox::information(this, QStringLiteral("导出完成"),
                                     QStringLiteral("成功导出 %1 个切片到:\n%2").arg(processed).arg(outputDir));
        }
    }

    QMenuBar *ExportPage::createMenuBar(QWidget *parent) {
        auto *bar = new QMenuBar(parent);
        auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
        fileMenu->addAction(QStringLiteral("退出(&X)"), this, [this]() {
            if (auto *w = window())
                w->close();
        });
        return bar;
    }

    QString ExportPage::windowTitle() const {
        return QStringLiteral("DsLabeler — 导出");
    }

    void ExportPage::onActivated() {
        runValidation();
        updateExportButton();
    }

    void ExportPage::onDeactivated() {
        if (m_enginePool)
            m_enginePool->shutdown();
        m_phNumCalc.reset();
    }

    void ExportPage::runValidation() {
        m_readyForCsv = 0;
        m_readyForDs = 0;
        m_dirtyCount = 0;
        m_totalSlices = 0;
        m_missingFa = 0;
        m_missingPitch = 0;
        m_missingMidi = 0;
        m_missingPhNum = 0;
        m_readySlices = 0;
        m_missingLayerSlices = 0;
        m_dirtySlices = 0;

        for (auto &cc : m_columnCoverages)
            cc.count = 0;

        if (!m_source) {
            m_validationLabel->setText(QStringLiteral("请先打开工程。"));
            m_validationLabel->setStyleSheet(
                QStringLiteral("#validationLabel { padding: 8px; border-radius: 4px; color: #6F737A; }"));
            m_coverageWidget->setVisible(false);
            return;
        }

        auto &colCov = m_columnCoverages;

        // Delegate to centralized ExportService for all validation logic
        auto result = dstools::ExportService::validate(m_source);

        m_readyForCsv = result.readyForCsv;
        m_readyForDs = result.readyForDs;
        m_dirtyCount = result.dirtyCount;
        m_totalSlices = result.totalSlices;
        m_missingFa = result.missingFa;
        m_missingPitch = result.missingPitch;
        m_missingMidi = result.missingMidi;
        m_missingPhNum = result.missingPhNum;
        m_readySlices = result.readySlices;
        m_missingLayerSlices = result.missingLayerSlices;
        m_dirtySlices = result.dirtySlices;

        for (int i = 0; i < 7; ++i)
            colCov[i].count = result.colCoverage[i];

        for (auto &cc : m_columnCoverages) {
            int pct = m_totalSlices > 0 ? (cc.count * 100 / m_totalSlices) : 0;
            cc.bar->setValue(pct);
            cc.pctLabel->setText(QStringLiteral("%1%").arg(pct));
            QString color;
            if (pct >= 90)
                color = QStringLiteral("#4CAF50");
            else if (pct >= 50)
                color = QStringLiteral("#FF9800");
            else
                color = QStringLiteral("#F44336");
            cc.bar->setStyleSheet(
                QStringLiteral("QProgressBar#coverageBar::chunk { background-color: %1; border-radius: 2px; }")
                    .arg(color));
        }

        m_summaryLabel->setText(
            tr("%1 切片  |  %2 就绪  |  %3 缺层  |  %4 脏层")
                .arg(m_totalSlices)
                .arg(m_readySlices)
                .arg(m_missingLayerSlices)
                .arg(m_dirtySlices));
        m_coverageWidget->setVisible(true);

        QStringList lines;
        lines.append(QStringLiteral("共 %1 个活动切片").arg(m_totalSlices));
        lines.append(QStringLiteral("  • CSV 就绪（含 grapheme 层）: %1/%2").arg(m_readyForCsv).arg(m_totalSlices));
        lines.append(QStringLiteral("  • .ds 就绪（含 pitch_review）: %1/%2").arg(m_readyForDs).arg(m_totalSlices));

        if (m_missingFa > 0 || m_missingPhNum > 0 || m_missingPitch > 0 || m_missingMidi > 0) {
            lines.append(QStringLiteral("  缺失步骤:"));
            if (m_missingFa > 0)
                lines.append(QStringLiteral("    • FA (phoneme): %1 个切片").arg(m_missingFa));
            if (m_missingPhNum > 0)
                lines.append(QStringLiteral("    • ph_num: %1 个切片").arg(m_missingPhNum));
            if (m_missingPitch > 0)
                lines.append(QStringLiteral("    • F0 (pitch): %1 个切片").arg(m_missingPitch));
            if (m_missingMidi > 0)
                lines.append(QStringLiteral("    • MIDI: %1 个切片").arg(m_missingMidi));

            if (m_chkAutoComplete->isChecked())
                lines.append(QStringLiteral("  ✓ 自动补全已启用，导出时将自动运行缺失步骤"));
        }

        if (m_dirtyCount > 0)
            lines.append(QStringLiteral("  ⚠ %1 个切片存在脏层（上游修改未重算）").arg(m_dirtyCount));

        QString color;
        if (m_readyForCsv == 0)
            color = QStringLiteral("color: #cc3333;");
        else if (m_readyForCsv < m_totalSlices || m_dirtyCount > 0)
            color = QStringLiteral("color: #cc8800;");
        else
            color = QStringLiteral("color: #338833;");

        m_validationLabel->setText(lines.join(QStringLiteral("\n")));
        m_validationLabel->setStyleSheet(
            QStringLiteral("#validationLabel { padding: 8px; border-radius: 4px; %1 }").arg(color));
    }

} // namespace dstools
