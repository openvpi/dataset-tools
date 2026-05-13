#include "ExportPage.h"
#include "core/ProjectDataSource.h"

#include <dsfw/Log.h>
#include <dsfw/PipelineContext.h>
#include <dsfw/Theme.h>
#include <dstools/DsDocument.h>
#include <dstools/DsProject.h>
#include <dstools/DsTextTypes.h>
#include <dstools/ExecutionProvider.h>
#include <dstools/PhNumCalculator.h>
#include <dstools/ProjectPaths.h>
#include <dstools/TranscriptionCsv.h>

#include <game-infer/Game.h>
#include <hubert-infer/Hfa.h>
#include <rmvpe-infer/Rmvpe.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace dstools {

    ExportPage::ExportPage(QWidget *parent) : QWidget(parent) {
        m_phNumCalc = std::make_unique<PhNumCalculator>();
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
        mainLayout->addSpacing(16);

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
        m_sampleRate->setValue(44100);
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

        buildPreviewTab();

        mainLayout->addWidget(m_tabWidget);

        connect(m_btnBrowse, &QPushButton::clicked, this, &ExportPage::onBrowseOutput);
        connect(m_btnExport, &QPushButton::clicked, this, &ExportPage::onExport);
        connect(m_outputDir, &QLineEdit::textChanged, this, [this]() { updateExportButton(); });
        connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
            if (index == 1)
                refreshPreview();
        });
    }

    void ExportPage::buildPreviewTab() {
        auto *previewWidget = new QWidget(this);
        auto *previewLayout = new QVBoxLayout(previewWidget);
        previewLayout->setContentsMargins(0, 0, 0, 0);

        m_previewModel = new QStandardItemModel(this);
        m_previewTable = new QTableView(this);
        m_previewTable->setObjectName(QStringLiteral("csvPreviewTable"));
        m_previewTable->setModel(m_previewModel);
        m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_previewTable->setAlternatingRowColors(true);
        m_previewTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        previewLayout->addWidget(m_previewTable);

        m_tabWidget->addTab(previewWidget, QStringLiteral("预览数据"));
    }

    void ExportPage::refreshPreview() {
        m_previewModel->clear();

        if (!m_source)
            return;

        QStringList headers = {
            QStringLiteral("name"),   QStringLiteral("ph_seq"), QStringLiteral("ph_dur"),
            QStringLiteral("ph_num"), QStringLiteral("input_type"),
        };
        m_previewModel->setHorizontalHeaderLabels(headers);

        const auto sliceIds = m_source->sliceIds();
        for (const auto &sliceId : sliceIds) {
            auto *ctxPtr = m_source->context(sliceId);
            if (!ctxPtr || ctxPtr->status != PipelineContext::Status::Active)
                continue;
            const auto &ctx = *ctxPtr;

            QList<QStandardItem *> items;

            auto *nameItem = new QStandardItem(sliceId);
            items.append(nameItem);

            QString phSeq, phDur, phNum, inputType;

            bool hasPhoneme = ctx.layers.contains("phoneme");
            bool hasPhNum = ctx.layers.contains("ph_num");

            if (hasPhoneme) {
                const auto &phonemeLayer = ctx.layers.at("phoneme");
                if (phonemeLayer.contains("boundaries") && phonemeLayer["boundaries"].is_array()) {
                    QStringList phs;
                    QStringList durs;
                    const auto &bounds = phonemeLayer["boundaries"];
                    for (size_t i = 0; i + 1 < bounds.size(); ++i) {
                        double startUs = bounds[i].get<double>();
                        double endUs = bounds[i + 1].get<double>();
                        double durSec = (endUs - startUs) / 1e6;
                        QString label = phonemeLayer.contains("labels") && i < phonemeLayer["labels"].size()
                                            ? QString::fromStdString(phonemeLayer["labels"][i].get<std::string>())
                                            : QStringLiteral("?");
                        phs.append(label);
                        durs.append(QString::number(durSec, 'f', 6));
                    }
                    phSeq = phs.join(' ');
                    phDur = durs.join(' ');
                }
            }

            if (hasPhNum) {
                const auto &phNumLayer = ctx.layers.at("ph_num");
                if (phNumLayer.contains("values") && phNumLayer["values"].is_array()) {
                    QStringList nums;
                    for (const auto &v : phNumLayer["values"])
                        nums.append(QString::number(v.get<int>()));
                    phNum = nums.join(' ');
                }
            }

            inputType = ctx.layers.contains("phoneme") ? "phoneme" : "grapheme";

            // Fall back to DsTextDocument data if ctx.layers is incomplete
            if ((!hasPhoneme || phSeq.isEmpty()) || !hasPhNum) {
                auto docResult = m_source->loadSlice(sliceId);
                if (docResult) {
                    const auto &doc = docResult.value();

                    if (phSeq.isEmpty()) {
                        for (const auto &layer : doc.layers) {
                            if (layer.name == QStringLiteral("phoneme")) {
                                QStringList phs;
                                QStringList durs;
                                for (size_t i = 0; i + 1 < layer.boundaries.size(); ++i) {
                                    double durSec = usToSec(layer.boundaries[i + 1].pos - layer.boundaries[i].pos);
                                    QString label =
                                        i < layer.boundaries.size() ? layer.boundaries[i].text : QStringLiteral("?");
                                    phs.append(label);
                                    durs.append(QString::number(durSec, 'f', 6));
                                }
                                if (!phs.isEmpty()) {
                                    phSeq = phs.join(' ');
                                    phDur = durs.join(' ');
                                    inputType = QStringLiteral("phoneme");
                                }
                                break;
                            }
                        }
                    }
                }
            }

            items.append(new QStandardItem(phSeq));
            items.append(new QStandardItem(phDur));
            items.append(new QStandardItem(phNum));
            items.append(new QStandardItem(inputType));

            bool hasMissing = phSeq.isEmpty() || phDur.isEmpty();
            if (hasMissing) {
                auto warnColor = dsfw::Theme::instance().palette().warning;
                warnColor.setAlpha(60);
                for (auto *item : items) {
                    item->setBackground(warnColor);
                }
            }

            m_previewModel->appendRow(items);
        }

        m_previewTable->resizeColumnsToContents();
    }

    void ExportPage::setDataSource(ProjectDataSource *source) {
        m_source = source;
        updateExportButton();
    }

    void ExportPage::onBrowseOutput() {
        const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择导出目录"));
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
        if (!m_source || !m_source->project())
            return;

        const auto &defaults = m_source->project()->defaults();

        bool anyEngineLoaded = false;

        // HubertFA
        if (!m_hfa) {
            auto it = defaults.taskModels.find(QStringLiteral("phoneme_alignment"));
            if (it != defaults.taskModels.end() && !it->second.modelPath.isEmpty()) {
                auto provider = dstools::infer::ExecutionProvider::CPU;
#ifdef ONNXRUNTIME_ENABLE_DML
                if (it->second.provider == QStringLiteral("dml"))
                    provider = dstools::infer::ExecutionProvider::DML;
#endif
                m_hfa = std::make_unique<HFA::HFA>();
                auto result = m_hfa->load(it->second.modelPath.toStdWString(), provider, it->second.deviceId);
                if (!result)
                    m_hfa.reset();
                else
                    anyEngineLoaded = true;
            }
        } else {
            anyEngineLoaded = true;
        }

        // RMVPE
        if (!m_rmvpe) {
            auto it = defaults.taskModels.find(QStringLiteral("pitch_extraction"));
            if (it != defaults.taskModels.end() && !it->second.modelPath.isEmpty()) {
                auto provider = dstools::infer::ExecutionProvider::CPU;
#ifdef ONNXRUNTIME_ENABLE_DML
                if (it->second.provider == QStringLiteral("dml"))
                    provider = dstools::infer::ExecutionProvider::DML;
#endif
                m_rmvpe = std::make_unique<Rmvpe::Rmvpe>();
                auto result = m_rmvpe->load(it->second.modelPath.toStdWString(), provider, it->second.deviceId);
                if (!result)
                    m_rmvpe.reset();
                else
                    anyEngineLoaded = true;
            }
        } else {
            anyEngineLoaded = true;
        }

        // GAME
        if (!m_game) {
            auto it = defaults.taskModels.find(QStringLiteral("midi_transcription"));
            if (it != defaults.taskModels.end() && !it->second.modelPath.isEmpty()) {
                auto provider = Game::ExecutionProvider::CPU;
#ifdef ONNXRUNTIME_ENABLE_DML
                if (it->second.provider == QStringLiteral("dml"))
                    provider = Game::ExecutionProvider::DML;
#endif
                m_game = std::make_unique<Game::Game>();
                auto result = m_game->loadModel(it->second.modelPath.toStdWString(), provider, it->second.deviceId);
                if (!result)
                    m_game.reset();
                else
                    anyEngineLoaded = true;
            }
        } else {
            anyEngineLoaded = true;
        }

        if (anyEngineLoaded && !m_enginesAlive)
            m_enginesAlive = std::make_shared<std::atomic<bool>>(true);
    }

    void ExportPage::autoCompleteSlice(const QString &sliceId) {
        if (!m_source)
            return;

        auto result = m_source->loadSlice(sliceId);
        if (!result)
            return;

        DsTextDocument doc = std::move(result.value());

        bool hasPhoneme = false;
        bool hasPhNum = false;
        bool hasPitch = false;
        bool hasMidi = false;
        bool hasGrapheme = false;
        QStringList graphemeTexts;
        QStringList phonemeTexts;

        for (const auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("grapheme")) {
                hasGrapheme = true;
                for (const auto &b : layer.boundaries) {
                    if (!b.text.isEmpty())
                        graphemeTexts << b.text;
                }
            } else if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive)) {
                hasPhoneme = true;
                for (const auto &b : layer.boundaries) {
                    if (!b.text.isEmpty())
                        phonemeTexts << b.text;
                }
            } else if (layer.name == QStringLiteral("ph_num")) {
                hasPhNum = true;
            } else if (layer.name == QStringLiteral("midi")) {
                hasMidi = true;
            }
        }

        for (const auto &curve : doc.curves) {
            if (curve.name == QStringLiteral("pitch"))
                hasPitch = true;
        }

        QString audioPath = m_source->audioPath(sliceId);

        // Run FA if missing phoneme layer
        if (!hasPhoneme && hasGrapheme && m_hfa && m_hfa->isOpen() && !audioPath.isEmpty()) {
            std::string lyricsText;
            for (const auto &text : graphemeTexts) {
                if (!lyricsText.empty())
                    lyricsText += " ";
                lyricsText += text.toStdString();
            }

            HFA::WordList words;
            std::vector<std::string> nonSpeechPh = {"AP", "SP"};
            auto faResult = m_hfa->recognize(audioPath.toStdWString(), "zh", nonSpeechPh, lyricsText, words);
            if (faResult) {
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

                bool replaced = false;
                for (auto &layer : doc.layers) {
                    if (layer.name == QStringLiteral("phoneme")) {
                        layer = phonemeLayer;
                        replaced = true;
                        break;
                    }
                }
                if (!replaced)
                    doc.layers.push_back(std::move(phonemeLayer));

                hasPhoneme = true;
                phonemeTexts.clear();
                for (const auto &layer : doc.layers) {
                    if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive)) {
                        for (const auto &b : layer.boundaries) {
                            if (!b.text.isEmpty())
                                phonemeTexts << b.text;
                        }
                    }
                }
            }
        }

        // Run add_ph_num if missing
        if (hasPhoneme && !hasPhNum && !phonemeTexts.isEmpty()) {
            QString phNumStr;
            QString error;
            if (m_phNumCalc->calculate(phonemeTexts.join(' '), phNumStr, error)) {
                IntervalLayer phNumLayer;
                phNumLayer.name = QStringLiteral("ph_num");
                phNumLayer.type = QStringLiteral("attribute");
                const auto parts = phNumStr.split(QChar(' '), Qt::SkipEmptyParts);
                int id = 1;
                for (const auto &part : parts) {
                    Boundary b;
                    b.id = id++;
                    b.text = part;
                    phNumLayer.boundaries.push_back(std::move(b));
                }

                bool replaced = false;
                for (auto &layer : doc.layers) {
                    if (layer.name == QStringLiteral("ph_num")) {
                        layer = phNumLayer;
                        replaced = true;
                        break;
                    }
                }
                if (!replaced)
                    doc.layers.push_back(std::move(phNumLayer));
            }
        }

        // Run RMVPE if missing pitch
        if (!hasPitch && m_rmvpe && m_rmvpe->is_open() && !audioPath.isEmpty()) {
            std::vector<Rmvpe::RmvpeRes> results;
            auto rmvpeResult = m_rmvpe->get_f0(audioPath.toStdWString(), 0.03f, results, nullptr);
            if (rmvpeResult && !results.empty()) {
                const auto &res = results[0];
                std::vector<int32_t> f0Mhz(res.f0.size());
                for (size_t i = 0; i < res.f0.size(); ++i)
                    f0Mhz[i] = static_cast<int32_t>(res.f0[i] * 1000.0f);

                float timestep = 0.005f;

                CurveLayer *pitchCurve = nullptr;
                for (auto &curve : doc.curves) {
                    if (curve.name == QStringLiteral("pitch")) {
                        pitchCurve = &curve;
                        break;
                    }
                }
                if (!pitchCurve) {
                    doc.curves.push_back({});
                    pitchCurve = &doc.curves.back();
                    pitchCurve->name = QStringLiteral("pitch");
                }
                pitchCurve->values = f0Mhz;
                pitchCurve->timestep = timestep;
            }
        }

        // Run GAME if missing midi
        if (!hasMidi && m_game && m_game->isOpen() && !audioPath.isEmpty()) {
            std::vector<Game::GameNote> notes;
            auto gameResult = m_game->getNotes(audioPath.toStdWString(), notes, nullptr);
            if (gameResult && !notes.empty()) {
                IntervalLayer *midiLayer = nullptr;
                for (auto &layer : doc.layers) {
                    if (layer.name == QStringLiteral("midi")) {
                        midiLayer = &layer;
                        break;
                    }
                }
                if (!midiLayer) {
                    doc.layers.push_back({});
                    midiLayer = &doc.layers.back();
                    midiLayer->name = QStringLiteral("midi");
                    midiLayer->type = QStringLiteral("note");
                }

                midiLayer->boundaries.clear();
                int id = 1;
                for (const auto &note : notes) {
                    Boundary b;
                    b.id = id++;
                    b.pos = static_cast<int64_t>(note.onset * 1000000.0);
                    int midiNote = static_cast<int>(note.pitch + 0.5);
                    int octave = midiNote / 12 - 1;
                    int pc = midiNote % 12;
                    static const char *pcNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
                    b.text = QStringLiteral("%1%2").arg(pcNames[pc]).arg(octave);
                    midiLayer->boundaries.push_back(std::move(b));
                }
            }
        }

        (void) m_source->saveSlice(sliceId, doc);
    }

    void ExportPage::onExport() {
        if (!m_source) {
            QMessageBox::warning(this, QStringLiteral("导出"), QStringLiteral("请先打开工程。"));
            return;
        }

        const auto sliceIds = m_source->sliceIds();
        if (sliceIds.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("导出"), QStringLiteral("没有可导出的切片。"));
            return;
        }

        const QString outputDir = m_outputDir->text().trimmed();
        QDir().mkpath(outputDir);

        // Auto-complete missing steps if enabled (runs inference in background)
        if (m_chkAutoComplete->isChecked()) {
            ensureEngines();

            m_progressBar->setVisible(true);
            m_progressBar->setRange(0, sliceIds.size());
            m_statusLabel->setText(QStringLiteral("正在补全缺失步骤..."));
            m_btnExport->setEnabled(false);

            // Capture engine pointers and source for use in background thread.
            // Engines are owned by ExportPage and persist until deactivation.
            auto *hfa = m_hfa.get();
            auto *rmvpe = m_rmvpe.get();
            auto *game = m_game.get();
            auto *phNumCalc = m_phNumCalc.get();
            auto enginesAlive = m_enginesAlive;
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
                                [guard, processed]() {
                                    if (guard) {
                                        guard->m_progressBar->setValue(processed);
                                        guard->m_statusLabel->setText(QStringLiteral("补全: %1/%2")
                                                                          .arg(processed)
                                                                          .arg(guard->m_progressBar->maximum()));
                                    }
                                },
                                Qt::QueuedConnection);
                            continue;
                        }

                        DsTextDocument doc = std::move(result.value());
                        bool modified = false;
                        bool hasPhoneme = false;
                        bool hasPhNum = false;
                        bool hasPitch = false;
                        bool hasMidi = false;
                        bool hasGrapheme = false;
                        QStringList graphemeTexts;
                        QStringList phonemeTexts;

                        for (const auto &layer : doc.layers) {
                            if (layer.name == QStringLiteral("grapheme")) {
                                hasGrapheme = true;
                                for (const auto &b : layer.boundaries) {
                                    if (!b.text.isEmpty())
                                        graphemeTexts << b.text;
                                }
                            } else if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive)) {
                                hasPhoneme = true;
                                for (const auto &b : layer.boundaries) {
                                    if (!b.text.isEmpty())
                                        phonemeTexts << b.text;
                                }
                            } else if (layer.name == QStringLiteral("ph_num")) {
                                hasPhNum = true;
                            } else if (layer.name == QStringLiteral("midi")) {
                                hasMidi = true;
                            }
                        }

                        for (const auto &curve : doc.curves) {
                            if (curve.name == QStringLiteral("pitch"))
                                hasPitch = true;
                        }

                        QString audioPath = src->audioPath(sliceId);

                        // Run FA if missing phoneme layer
                        if (!hasPhoneme && hasGrapheme && hfa && hfa->isOpen() && !audioPath.isEmpty()) {
                            std::string lyricsText;
                            for (const auto &text : graphemeTexts) {
                                if (!lyricsText.empty())
                                    lyricsText += " ";
                                lyricsText += text.toStdString();
                            }

                            HFA::WordList words;
                            std::vector<std::string> nonSpeechPh = {"AP", "SP"};
                            auto faResult =
                                hfa->recognize(audioPath.toStdWString(), "zh", nonSpeechPh, lyricsText, words);
                            if (faResult) {
                                IntervalLayer phonemeLayer;
                                phonemeLayer.name = QStringLiteral("phoneme");
                                phonemeLayer.type = QStringLiteral("interval");
                                int id = 1;
                                for (const auto &word : words) {
                                    if (word.text == "SP" || word.text == "AP")
                                        continue;
                                    for (const auto &phone : word.phones) {
                                        if (phone.text == "SP" || phone.text == "AP")
                                            continue;
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

                                bool replaced = false;
                                for (auto &layer : doc.layers) {
                                    if (layer.name == QStringLiteral("phoneme")) {
                                        layer = std::move(phonemeLayer);
                                        replaced = true;
                                        break;
                                    }
                                }
                                if (!replaced)
                                    doc.layers.push_back(std::move(phonemeLayer));
                                modified = true;
                                hasPhoneme = true;

                                phonemeTexts.clear();
                                for (const auto &layer : doc.layers) {
                                    if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive)) {
                                        for (const auto &b : layer.boundaries) {
                                            if (!b.text.isEmpty())
                                                phonemeTexts << b.text;
                                        }
                                    }
                                }
                            }
                        }

                        // Run add_ph_num if missing
                        if (hasPhoneme && !hasPhNum && !phonemeTexts.isEmpty() && phNumCalc) {
                            QString phNumStr;
                            QString error;
                            if (phNumCalc->calculate(phonemeTexts.join(' '), phNumStr, error)) {
                                IntervalLayer phNumLayer;
                                phNumLayer.name = QStringLiteral("ph_num");
                                phNumLayer.type = QStringLiteral("attribute");
                                const auto parts = phNumStr.split(QChar(' '), Qt::SkipEmptyParts);
                                int id = 1;
                                for (const auto &part : parts) {
                                    Boundary b;
                                    b.id = id++;
                                    b.text = part;
                                    phNumLayer.boundaries.push_back(std::move(b));
                                }

                                bool replaced = false;
                                for (auto &layer : doc.layers) {
                                    if (layer.name == QStringLiteral("ph_num")) {
                                        layer = std::move(phNumLayer);
                                        replaced = true;
                                        break;
                                    }
                                }
                                if (!replaced)
                                    doc.layers.push_back(std::move(phNumLayer));
                                modified = true;
                            }
                        }

                        // Run RMVPE if missing pitch
                        if (!hasPitch && rmvpe && rmvpe->is_open() && !audioPath.isEmpty()) {
                            std::vector<Rmvpe::RmvpeRes> results;
                            auto rmvpeResult = rmvpe->get_f0(audioPath.toStdWString(), 0.03f, results, nullptr);
                            if (rmvpeResult && !results.empty()) {
                                const auto &res = results[0];
                                std::vector<int32_t> f0Mhz(res.f0.size());
                                for (size_t i = 0; i < res.f0.size(); ++i)
                                    f0Mhz[i] = static_cast<int32_t>(res.f0[i] * 1000.0f);

                                float timestep = 0.005f;
                                CurveLayer *pitchCurve = nullptr;
                                for (auto &curve : doc.curves) {
                                    if (curve.name == QStringLiteral("pitch")) {
                                        pitchCurve = &curve;
                                        break;
                                    }
                                }
                                if (!pitchCurve) {
                                    doc.curves.push_back({});
                                    pitchCurve = &doc.curves.back();
                                    pitchCurve->name = QStringLiteral("pitch");
                                }
                                pitchCurve->values = std::move(f0Mhz);
                                pitchCurve->timestep = timestep;
                                modified = true;
                            }
                        }

                        // Run GAME if missing midi
                        if (!hasMidi && game && game->isOpen() && !audioPath.isEmpty()) {
                            std::vector<Game::GameNote> notes;
                            auto gameResult = game->getNotes(audioPath.toStdWString(), notes, nullptr);
                            if (gameResult && !notes.empty()) {
                                IntervalLayer *midiLayer = nullptr;
                                for (auto &layer : doc.layers) {
                                    if (layer.name == QStringLiteral("midi")) {
                                        midiLayer = &layer;
                                        break;
                                    }
                                }
                                if (!midiLayer) {
                                    doc.layers.push_back({});
                                    midiLayer = &doc.layers.back();
                                    midiLayer->name = QStringLiteral("midi");
                                    midiLayer->type = QStringLiteral("note");
                                }

                                midiLayer->boundaries.clear();
                                int id = 1;
                                for (const auto &note : notes) {
                                    Boundary b;
                                    b.id = id++;
                                    b.pos = static_cast<int64_t>(note.onset * 1000000.0);
                                    int midiNote = static_cast<int>(note.pitch + 0.5);
                                    int octave = midiNote / 12 - 1;
                                    int pc = midiNote % 12;
                                    static const char *pcNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                                                    "F#", "G",  "G#", "A",  "A#", "B"};
                                    b.text = QStringLiteral("%1%2").arg(pcNames[pc]).arg(octave);
                                    midiLayer->boundaries.push_back(std::move(b));
                                }
                                modified = true;
                            }
                        }

                        if (modified)
                            modifiedDocs[sliceId] = std::move(doc);

                        ++processed;
                        // Update progress on main thread
                        QMetaObject::invokeMethod(
                            guard.data(),
                            [guard, processed]() {
                                if (guard) {
                                    guard->m_progressBar->setValue(processed);
                                    guard->m_statusLabel->setText(QStringLiteral("补全: %1/%2")
                                                                      .arg(processed)
                                                                      .arg(guard->m_progressBar->maximum()));
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
                        for (auto &[sliceId, doc] : modifiedDocs)
                            (void) guard->m_source->saveSlice(sliceId, doc);

                        guard->m_progressBar->setVisible(false);

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

        std::vector<TranscriptionRow> csvRows;
        int exported = 0;
        int errors = 0;
        for (const auto &sliceId : sliceIds) {
            m_statusLabel->setText(QStringLiteral("正在导出: %1").arg(sliceId));
            m_progressBar->setValue(++exported);

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
                    QString dstPath = wavsDir + QStringLiteral("/") + dstName;
                    if (!QFile::copy(srcAudio, dstPath)) {
                        if (!QFile::remove(dstPath) || !QFile::copy(srcAudio, dstPath))
                            ++errors;
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
                    dsDoc.sentences().push_back(sentence);
                    auto saveResult = dsDoc.saveFile(dsDir + QStringLiteral("/") + sliceId + QStringLiteral(".ds"));
                    if (!saveResult)
                        ++errors;
                }
            }

            if (m_chkCsv->isChecked()) {
                TranscriptionRow row;
                row.name = sliceId;
                for (const auto &layer : doc.layers) {
                    if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive)) {
                        QStringList phs, durs;
                        for (size_t i = 0; i < layer.boundaries.size(); ++i) {
                            phs << layer.boundaries[i].text;
                            if (i + 1 < layer.boundaries.size()) {
                                double dur = layer.boundaries[i + 1].pos - layer.boundaries[i].pos;
                                durs << QString::number(dur, 'f', 5);
                            }
                        }
                        row.phSeq = phs.join(' ');
                        row.phDur = durs.join(' ');
                    }
                    if (layer.name.contains(QStringLiteral("note"), Qt::CaseInsensitive)) {
                        QStringList notes;
                        for (const auto &b : layer.boundaries)
                            notes << b.text;
                        row.noteSeq = notes.join(' ');
                    }
                }
                if (!row.phSeq.isEmpty())
                    csvRows.push_back(std::move(row));
            }
        }

        if (m_chkCsv->isChecked() && !csvRows.empty()) {
            QString csvError;
            if (!TranscriptionCsv::write(outputDir + QStringLiteral("/transcriptions.csv"), csvRows, csvError))
                ++errors;
        }

        m_progressBar->setVisible(false);
        m_statusLabel->setText(QStringLiteral("导出完成: %1 个切片 → %2").arg(exported).arg(outputDir));
        m_btnExport->setEnabled(true);

        if (errors > 0) {
            QMessageBox::warning(
                this, QStringLiteral("导出完成"),
                QStringLiteral("导出完成，但有 %1 个错误。\n输出目录:\n%2").arg(errors).arg(outputDir));
        } else {
            QMessageBox::information(this, QStringLiteral("导出完成"),
                                     QStringLiteral("成功导出 %1 个切片到:\n%2").arg(exported).arg(outputDir));
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
        if (m_enginesAlive)
            m_enginesAlive->store(false);
        m_hfa.reset();
        m_rmvpe.reset();
        m_game.reset();
        m_phNumCalc.reset();
        m_enginesAlive.reset();
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

        if (!m_source) {
            m_validationLabel->setText(QStringLiteral("请先打开工程。"));
            m_validationLabel->setStyleSheet(
                QStringLiteral("#validationLabel { padding: 8px; border-radius: 4px; color: #6F737A; }"));
            return;
        }

        const auto ids = m_source->sliceIds();
        m_totalSlices = ids.size();

        if (m_totalSlices == 0) {
            m_validationLabel->setText(QStringLiteral("没有活动切片。"));
            m_validationLabel->setStyleSheet(
                QStringLiteral("#validationLabel { padding: 8px; border-radius: 4px; color: #6F737A; }"));
            return;
        }

        for (const auto &id : ids) {
            auto result = m_source->loadSlice(id);
            if (!result)
                continue;

            const auto &doc = result.value();

            bool hasGrapheme = false;
            bool hasPhoneme = false;
            bool hasPhNum = false;
            bool hasPitch = false;
            bool hasMidi = false;

            for (const auto &layer : doc.layers) {
                if (layer.name == QStringLiteral("grapheme"))
                    hasGrapheme = true;
                else if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive))
                    hasPhoneme = true;
                else if (layer.name == QStringLiteral("ph_num"))
                    hasPhNum = true;
                else if (layer.name == QStringLiteral("midi"))
                    hasMidi = true;
            }
            for (const auto &curve : doc.curves) {
                if (curve.name == QStringLiteral("pitch"))
                    hasPitch = true;
            }

            if (hasGrapheme)
                ++m_readyForCsv;

            if (doc.meta.editedSteps.contains(QStringLiteral("pitch_review")))
                ++m_readyForDs;

            if (hasGrapheme && !hasPhoneme)
                ++m_missingFa;
            if (hasPhoneme && !hasPhNum)
                ++m_missingPhNum;
            if (!hasPitch)
                ++m_missingPitch;
            if (!hasMidi)
                ++m_missingMidi;

            auto *ctx = m_source->context(id);
            if (ctx && !ctx->dirty.isEmpty())
                ++m_dirtyCount;
        }

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
