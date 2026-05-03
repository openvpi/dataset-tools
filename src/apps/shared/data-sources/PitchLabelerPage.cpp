#include "PitchLabelerPage.h"
#include "SliceListPanel.h"
#include "ModelConfigHelper.h"
#include "ISettingsBackend.h"

#include <dstools/IEditorDataSource.h>

#include <DSFile.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QtConcurrent>

#include <rmvpe-infer/Rmvpe.h>

#include <dsfw/CommonKeys.h>
#include <dsfw/Theme.h>
#include <dsfw/widgets/ToastNotification.h>
#include <dstools/DsTextTypes.h>
#include <dstools/ExecutionProvider.h>

namespace dstools {

PitchLabelerPage::PitchLabelerPage(QWidget *parent)
    : QWidget(parent), m_settings("PitchLabeler") {
    m_editor = new pitchlabeler::PitchEditor(this);
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

    static const dstools::SettingsKey<QString> kShortcutSave("Shortcuts/save", "Ctrl+S");
    static const dstools::SettingsKey<QString> kShortcutUndo("Shortcuts/undo", "Ctrl+Z");
    static const dstools::SettingsKey<QString> kShortcutRedo("Shortcuts/redo", "Ctrl+Y");
    static const dstools::SettingsKey<QString> kShortcutZoomIn("Shortcuts/zoomIn", "Ctrl+=");
    static const dstools::SettingsKey<QString> kShortcutZoomOut("Shortcuts/zoomOut", "Ctrl+-");
    static const dstools::SettingsKey<QString> kShortcutZoomReset("Shortcuts/zoomReset", "Ctrl+0");
    static const dstools::SettingsKey<QString> kShortcutABCompare("Shortcuts/abCompare", "Ctrl+B");

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
    m_shortcutManager->bind(m_editor->zoomInAction(), kShortcutZoomIn,
                            QStringLiteral("放大"), QStringLiteral("视图"));
    m_shortcutManager->bind(m_editor->zoomOutAction(), kShortcutZoomOut,
                            QStringLiteral("缩小"), QStringLiteral("视图"));
    m_shortcutManager->bind(m_editor->zoomResetAction(), kShortcutZoomReset,
                            QStringLiteral("重置缩放"), QStringLiteral("视图"));
    m_shortcutManager->bind(m_editor->abCompareAction(), kShortcutABCompare,
                            QStringLiteral("A/B 比较"), QStringLiteral("工具"));
    m_shortcutManager->applyAll();

    connect(m_sliceList, &SliceListPanel::sliceSelected,
            this, &PitchLabelerPage::onSliceSelected);
    connect(m_editor->saveAction(), &QAction::triggered,
            this, [this]() { saveCurrentSlice(); });
    connect(m_prevAction, &QAction::triggered, this, [this]() {
        m_sliceList->selectPrev();
    });
    connect(m_nextAction, &QAction::triggered, this, [this]() {
        m_sliceList->selectNext();
    });
}

PitchLabelerPage::~PitchLabelerPage() = default;

void PitchLabelerPage::setDataSource(IEditorDataSource *source,
                                      ISettingsBackend *settingsBackend) {
    m_source = source;
    m_settingsBackend = settingsBackend;
    m_sliceList->setDataSource(source);
}

void PitchLabelerPage::onSliceSelected(const QString &sliceId) {
    if (!maybeSave())
        return;

    m_currentSliceId = sliceId;
    m_currentFile.reset();

    if (!m_source) {
        m_editor->clear();
        return;
    }

    const QString audioPath = m_source->audioPath(sliceId);

    auto result = m_source->loadSlice(sliceId);
    if (result) {
        const auto &doc = result.value();
        auto file = std::make_shared<pitchlabeler::DSFile>();

        for (const auto &curve : doc.curves) {
            if (curve.name == QStringLiteral("pitch")) {
                file->f0.values = curve.values;
                file->f0.timestep = curve.timestep;
            }
        }

        for (const auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("midi")) {
                for (const auto &b : layer.boundaries) {
                    pitchlabeler::Note note;
                    note.start = b.pos;
                    note.name = b.text;
                    file->notes.push_back(std::move(note));
                }
            }
        }

        if (!file->f0.values.empty() || !file->notes.empty()) {
            m_currentFile = file;
            m_editor->loadDSFile(file);
        }
    }

    if (!audioPath.isEmpty()) {
        m_editor->loadAudio(audioPath, 0.0);
    }

    emit sliceChanged(sliceId);
}

bool PitchLabelerPage::saveCurrentSlice() {
    if (m_currentSliceId.isEmpty() || !m_source || !m_currentFile)
        return true;

    auto result = m_source->loadSlice(m_currentSliceId);
    DsTextDocument doc;
    if (result)
        doc = std::move(result.value());

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
    pitchCurve->values = m_currentFile->f0.values;
    pitchCurve->timestep = m_currentFile->f0.timestep;

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
    for (const auto &note : m_currentFile->notes) {
        Boundary b;
        b.id = id++;
        b.pos = note.start;
        b.text = note.name;
        midiLayer->boundaries.push_back(std::move(b));
    }

    if (!doc.meta.editedSteps.contains(QStringLiteral("pitch_review")))
        doc.meta.editedSteps.append(QStringLiteral("pitch_review"));

    auto saveResult = m_source->saveSlice(m_currentSliceId, doc);
    if (!saveResult) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
                             QString::fromStdString(saveResult.error()));
        return false;
    }

    return true;
}

bool PitchLabelerPage::maybeSave() {
    if (!m_currentFile)
        return true;

    if (m_editor->undoStack() && !m_editor->undoStack()->isClean()) {
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
    return true;
}

QMenuBar *PitchLabelerPage::createMenuBar(QWidget *parent) {
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
    processMenu->addAction(QStringLiteral("提取音高 (当前切片)"),
                           this, &PitchLabelerPage::onExtractPitch);
    processMenu->addAction(QStringLiteral("提取 MIDI (当前切片)"),
                           this, &PitchLabelerPage::onExtractMidi);
    processMenu->addAction(QStringLiteral("计算 ph_num (当前切片)"),
                           this, &PitchLabelerPage::onAddPhNum);
    processMenu->addSeparator();
    processMenu->addAction(QStringLiteral("批量提取音高 + MIDI..."),
                           this, &PitchLabelerPage::onBatchExtract);

    auto *viewMenu = bar->addMenu(QStringLiteral("视图(&V)"));
    viewMenu->addAction(m_editor->zoomInAction());
    viewMenu->addAction(m_editor->zoomOutAction());
    viewMenu->addAction(m_editor->zoomResetAction());
    viewMenu->addSeparator();
    dsfw::Theme::instance().populateThemeMenu(viewMenu);

    auto *toolsMenu = bar->addMenu(QStringLiteral("工具(&T)"));
    toolsMenu->addAction(m_editor->abCompareAction());

    return bar;
}

QWidget *PitchLabelerPage::createStatusBarContent(QWidget *parent) {
    auto *container = new QWidget(parent);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *sliceLabel = new QLabel(QStringLiteral("未选择切片"), container);
    auto *posLabel = new QLabel(QStringLiteral("00:00.000"), container);
    auto *zoomLabel = new QLabel(QStringLiteral("100%"), container);
    auto *noteLabel = new QLabel(QStringLiteral("音符: 0"), container);
    layout->addWidget(sliceLabel, 1);
    layout->addWidget(posLabel);
    layout->addWidget(zoomLabel);
    layout->addWidget(noteLabel);

    connect(this, &PitchLabelerPage::sliceChanged, sliceLabel, [sliceLabel](const QString &id) {
        sliceLabel->setText(id.isEmpty() ? QStringLiteral("未选择切片") : id);
    });
    connect(m_editor, &pitchlabeler::PitchEditor::positionChanged,
            this, [posLabel](double sec) {
        int minutes = static_cast<int>(sec) / 60;
        double seconds = sec - minutes * 60;
        posLabel->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
                                          .arg(seconds, 6, 'f', 3, QChar('0')));
    });
    connect(m_editor, &pitchlabeler::PitchEditor::zoomChanged,
            this, [zoomLabel](int percent) {
        zoomLabel->setText(QString::number(percent) + "%");
    });
    connect(m_editor, &pitchlabeler::PitchEditor::noteCountChanged,
            this, [noteLabel](int count) {
        noteLabel->setText(QStringLiteral("音符: %1").arg(count));
    });

    return container;
}

QString PitchLabelerPage::windowTitle() const {
    QString title = QStringLiteral("音高标注");
    if (!m_currentSliceId.isEmpty())
        title += QStringLiteral(" — ") + m_currentSliceId;
    return title;
}

bool PitchLabelerPage::hasUnsavedChanges() const {
    return m_editor->undoStack() && !m_editor->undoStack()->isClean();
}

void PitchLabelerPage::onActivated() {
    m_sliceList->refresh();

    if (m_source && !m_currentSliceId.isEmpty()) {
        auto dirty = m_source->dirtyLayers(m_currentSliceId);
        QStringList pitchDirtyLayers = {QStringLiteral("pitch"),
                                        QStringLiteral("ph_num"),
                                        QStringLiteral("midi")};
        QStringList affected;
        for (const auto &layer : pitchDirtyLayers) {
            if (dirty.contains(layer))
                affected << layer;
        }
        if (!affected.isEmpty()) {
            m_source->clearDirtyLayers(m_currentSliceId, affected);
            dsfw::widgets::ToastNotification::show(
                this, dsfw::widgets::ToastType::Warning,
                QStringLiteral("以下层数据已过期: %1").arg(affected.join(QStringLiteral(", "))),
                3000);
        }
    }
}

bool PitchLabelerPage::onDeactivating() {
    return maybeSave();
}

void PitchLabelerPage::onShutdown() {
    m_shortcutManager->saveAll();
}

dstools::widgets::ShortcutManager *PitchLabelerPage::shortcutManager() const {
    return m_shortcutManager;
}

void PitchLabelerPage::ensureRmvpeEngine() {
    if (m_rmvpe && m_rmvpe->is_open())
        return;

    auto config = readModelConfig(m_settingsBackend, QStringLiteral("pitch_extraction"));
    if (config.modelPath.isEmpty())
        return;

    auto provider = dstools::infer::ExecutionProvider::CPU;
#ifdef ONNXRUNTIME_ENABLE_DML
    if (config.provider == QStringLiteral("dml"))
        provider = dstools::infer::ExecutionProvider::DML;
#endif

    m_rmvpe = std::make_unique<Rmvpe::Rmvpe>();
    auto result = m_rmvpe->load(config.modelPath.toStdWString(), provider, config.deviceId);
    if (!result) {
        m_rmvpe.reset();
    }
}

void PitchLabelerPage::ensureGameEngine() {
    if (m_game && m_game->isOpen())
        return;

    auto config = readModelConfig(m_settingsBackend, QStringLiteral("midi_transcription"));
    if (config.modelPath.isEmpty())
        return;

    auto provider = dstools::infer::ExecutionProvider::CPU;
#ifdef ONNXRUNTIME_ENABLE_DML
    if (config.provider == QStringLiteral("dml"))
        provider = dstools::infer::ExecutionProvider::DML;
#endif

    m_game = std::make_unique<Game::Game>();
    auto result = m_game->load(config.modelPath.toStdWString(), provider, config.deviceId);
    if (!result) {
        m_game.reset();
    }
}

void PitchLabelerPage::onExtractPitch() {
    if (m_currentSliceId.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提取音高"),
                                 QStringLiteral("请先选择一个切片。"));
        return;
    }

    ensureRmvpeEngine();
    if (!m_rmvpe || !m_rmvpe->is_open()) {
        QMessageBox::warning(this, QStringLiteral("提取音高"),
                             QStringLiteral("RMVPE 模型未加载。请在设置中配置模型路径。"));
        return;
    }

    if (m_inferRunning) {
        QMessageBox::information(this, QStringLiteral("提取音高"),
                                 QStringLiteral("推理正在运行中，请稍候。"));
        return;
    }

    runPitchExtraction(m_currentSliceId);
}

void PitchLabelerPage::onExtractMidi() {
    if (m_currentSliceId.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提取 MIDI"),
                                 QStringLiteral("请先选择一个切片。"));
        return;
    }

    ensureGameEngine();
    if (!m_game || !m_game->isOpen()) {
        QMessageBox::warning(this, QStringLiteral("提取 MIDI"),
                             QStringLiteral("GAME 模型未加载。请在设置中配置模型路径。"));
        return;
    }

    if (m_inferRunning) {
        QMessageBox::information(this, QStringLiteral("提取 MIDI"),
                                 QStringLiteral("推理正在运行中，请稍候。"));
        return;
    }

    runMidiTranscription(m_currentSliceId);
}

void PitchLabelerPage::onAddPhNum() {
    if (m_currentSliceId.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("计算 ph_num"),
                                 QStringLiteral("请先选择一个切片。"));
        return;
    }
    runAddPhNum(m_currentSliceId);
}

void PitchLabelerPage::runPitchExtraction(const QString &sliceId) {
    if (!m_source)
        return;

    QString audioPath = m_source->audioPath(sliceId);
    if (audioPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提取音高"),
                             QStringLiteral("当前切片没有音频文件。"));
        return;
    }

    m_inferRunning = true;
    auto *rmvpe = m_rmvpe.get();

    (void) QtConcurrent::run([rmvpe, audioPath, sliceId, this]() {
        std::vector<Rmvpe::RmvpeRes> results;
        auto result = rmvpe->get_f0(audioPath.toStdWString(), 0.03f, results, nullptr);

        QMetaObject::invokeMethod(this, [this, sliceId, result = std::move(result),
                                          results = std::move(results)]() {
            m_inferRunning = false;
            if (result && !results.empty()) {
                const auto &res = results[0];
                std::vector<int32_t> f0Mhz(res.f0.size());
                for (size_t i = 0; i < res.f0.size(); ++i) {
                    f0Mhz[i] = static_cast<int32_t>(res.f0[i] * 1000.0f);
                }
                float timestep = res.f0.size() > 1 && res.offset >= 0
                                     ? (1.0f / 16000.0f)
                                     : 0.005f;
                applyPitchResult(sliceId, f0Mhz, timestep);
                dsfw::widgets::ToastNotification::show(
                    this, dsfw::widgets::ToastType::Info,
                    QStringLiteral("音高提取完成"), 3000);
            } else {
                QMessageBox::warning(this, QStringLiteral("提取音高"),
                                     QStringLiteral("音高提取失败。"));
            }
        }, Qt::QueuedConnection);
    });
}

void PitchLabelerPage::runMidiTranscription(const QString &sliceId) {
    if (!m_source)
        return;

    QString audioPath = m_source->audioPath(sliceId);
    if (audioPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提取 MIDI"),
                             QStringLiteral("当前切片没有音频文件。"));
        return;
    }

    m_inferRunning = true;
    auto *game = m_game.get();

    (void) QtConcurrent::run([game, audioPath, sliceId, this]() {
        std::vector<Game::GameNote> notes;
        auto result = game->getNotes(audioPath.toStdWString(), notes, nullptr);

        QMetaObject::invokeMethod(this, [this, sliceId, result = std::move(result),
                                          notes = std::move(notes)]() {
            m_inferRunning = false;
            if (result) {
                applyMidiResult(sliceId, notes);
                dsfw::widgets::ToastNotification::show(
                    this, dsfw::widgets::ToastType::Info,
                    QStringLiteral("MIDI 转录完成"), 3000);
            } else {
                QMessageBox::warning(this, QStringLiteral("提取 MIDI"),
                                     QStringLiteral("MIDI 转录失败。"));
            }
        }, Qt::QueuedConnection);
    });
}

void PitchLabelerPage::runAddPhNum(const QString &sliceId) {
    if (!m_source)
        return;

    auto result = m_source->loadSlice(sliceId);
    if (!result) {
        QMessageBox::warning(this, QStringLiteral("计算 ph_num"),
                             QStringLiteral("无法加载切片数据。"));
        return;
    }

    DsTextDocument doc = std::move(result.value());

    QStringList phSeq;
    for (const auto &layer : doc.layers) {
        if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive)) {
            for (const auto &b : layer.boundaries) {
                if (!b.text.isEmpty())
                    phSeq << b.text;
            }
        }
    }

    if (phSeq.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("计算 ph_num"),
                                 QStringLiteral("当前切片没有音素数据，请先进行强制对齐。"));
        return;
    }

    QString phNumStr;
    QString error;
    if (m_phNumCalc.calculate(phSeq.join(' '), phNumStr, error)) {
        IntervalLayer *phNumLayer = nullptr;
        for (auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("ph_num")) {
                phNumLayer = &layer;
                break;
            }
        }
        if (!phNumLayer) {
            doc.layers.push_back({});
            phNumLayer = &doc.layers.back();
            phNumLayer->name = QStringLiteral("ph_num");
            phNumLayer->type = QStringLiteral("attribute");
        }

        phNumLayer->boundaries.clear();
        const auto parts = phNumStr.split(QChar(' '), Qt::SkipEmptyParts);
        int id = 1;
        for (const auto &part : parts) {
            Boundary b;
            b.id = id++;
            b.text = part;
            phNumLayer->boundaries.push_back(std::move(b));
        }

        (void) m_source->saveSlice(sliceId, doc);

        dsfw::widgets::ToastNotification::show(
            this, dsfw::widgets::ToastType::Info,
            QStringLiteral("ph_num 计算完成"), 3000);
    } else {
        QMessageBox::warning(this, QStringLiteral("计算 ph_num"),
                             QStringLiteral("ph_num 计算失败: %1").arg(error));
    }
}

void PitchLabelerPage::applyPitchResult(const QString &sliceId,
                                            const std::vector<int32_t> &f0,
                                            float timestep) {
    if (!m_source)
        return;

    auto result = m_source->loadSlice(sliceId);
    DsTextDocument doc;
    if (result)
        doc = std::move(result.value());

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
    pitchCurve->values = f0;
    pitchCurve->timestep = timestep;

    (void) m_source->saveSlice(sliceId, doc);

    if (sliceId == m_currentSliceId) {
        if (!m_currentFile)
            m_currentFile = std::make_shared<pitchlabeler::DSFile>();
        m_currentFile->f0.values = f0;
        m_currentFile->f0.timestep = timestep;
        m_editor->loadDSFile(m_currentFile);
    }
}

void PitchLabelerPage::applyMidiResult(const QString &sliceId,
                                           const std::vector<Game::GameNote> &notes) {
    if (!m_source)
        return;

    auto result = m_source->loadSlice(sliceId);
    DsTextDocument doc;
    if (result)
        doc = std::move(result.value());

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
        static const char *pcNames[] = {"C", "C#", "D", "D#", "E", "F",
                                         "F#", "G", "G#", "A", "A#", "B"};
        b.text = QStringLiteral("%1%2").arg(pcNames[pc]).arg(octave);
        midiLayer->boundaries.push_back(std::move(b));
    }

    (void) m_source->saveSlice(sliceId, doc);

    if (sliceId == m_currentSliceId) {
        if (!m_currentFile)
            m_currentFile = std::make_shared<pitchlabeler::DSFile>();
        m_currentFile->notes.clear();
        for (const auto &note : notes) {
            pitchlabeler::Note n;
            n.start = static_cast<int64_t>(note.onset * 1000000.0);
            int midiNote = static_cast<int>(note.pitch + 0.5);
            int octave = midiNote / 12 - 1;
            int pc = midiNote % 12;
            static const char *pcNames[] = {"C", "C#", "D", "D#", "E", "F",
                                             "F#", "G", "G#", "A", "A#", "B"};
            n.name = QStringLiteral("%1%2").arg(pcNames[pc]).arg(octave);
            m_currentFile->notes.push_back(std::move(n));
        }
        m_editor->loadDSFile(m_currentFile);
    }
}

void PitchLabelerPage::onBatchExtract() {
    if (!m_source) {
        QMessageBox::warning(this, QStringLiteral("批量提取"),
                             QStringLiteral("请先打开工程。"));
        return;
    }

    ensureRmvpeEngine();
    ensureGameEngine();

    bool hasRmvpe = m_rmvpe && m_rmvpe->is_open();
    bool hasGame = m_game && m_game->isOpen();

    if (!hasRmvpe && !hasGame) {
        QMessageBox::warning(this, QStringLiteral("批量提取"),
                             QStringLiteral("RMVPE 和 GAME 模型均未加载。请在设置中配置模型路径。"));
        return;
    }

    if (m_inferRunning) {
        QMessageBox::information(this, QStringLiteral("批量提取"),
                                 QStringLiteral("推理正在运行中，请稍候。"));
        return;
    }

    const auto ids = m_source->sliceIds();
    if (ids.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("批量提取"),
                                 QStringLiteral("没有可处理的切片。"));
        return;
    }

    m_inferRunning = true;
    auto *rmvpe = m_rmvpe.get();
    auto *game = m_game.get();
    auto *source = m_source;

    (void) QtConcurrent::run([rmvpe, game, hasRmvpe, hasGame, source, ids, this]() {
        int processed = 0;
        for (const auto &sliceId : ids) {
            QString audioPath = source->audioPath(sliceId);
            if (audioPath.isEmpty())
                continue;

            if (hasRmvpe) {
                std::vector<Rmvpe::RmvpeRes> results;
                auto result = rmvpe->get_f0(audioPath.toStdWString(), 0.03f, results, nullptr);
                if (result && !results.empty()) {
                    const auto &res = results[0];
                    std::vector<int32_t> f0Mhz(res.f0.size());
                    for (size_t i = 0; i < res.f0.size(); ++i) {
                        f0Mhz[i] = static_cast<int32_t>(res.f0[i] * 1000.0f);
                    }
                    float timestep = 0.005f;
                    QMetaObject::invokeMethod(this, [this, sliceId, f0Mhz, timestep]() {
                        applyPitchResult(sliceId, f0Mhz, timestep);
                    }, Qt::QueuedConnection);
                }
            }

            if (hasGame) {
                std::vector<Game::GameNote> notes;
                auto result = game->getNotes(audioPath.toStdWString(), notes, nullptr);
                if (result) {
                    QMetaObject::invokeMethod(this, [this, sliceId, notes = std::move(notes)]() {
                        applyMidiResult(sliceId, notes);
                    }, Qt::QueuedConnection);
                }
            }

            ++processed;
        }
        QMetaObject::invokeMethod(this, [this, processed]() {
            m_inferRunning = false;
            dsfw::widgets::ToastNotification::show(
                this, dsfw::widgets::ToastType::Info,
                QStringLiteral("批量提取完成: %1 个切片").arg(processed), 3000);
        }, Qt::QueuedConnection);
    });
}

} // namespace dstools
