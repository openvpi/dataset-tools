#include "DsPitchLabelerPage.h"
#include "ProjectDataSource.h"
#include "SliceListPanel.h"

#include <DSFile.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

#include <dsfw/Theme.h>

namespace dstools {

DsPitchLabelerPage::DsPitchLabelerPage(QWidget *parent) : QWidget(parent) {
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

    connect(m_sliceList, &SliceListPanel::sliceSelected,
            this, &DsPitchLabelerPage::onSliceSelected);
    connect(m_editor->saveAction(), &QAction::triggered,
            this, [this]() { saveCurrentSlice(); });
}

void DsPitchLabelerPage::setDataSource(ProjectDataSource *source) {
    m_source = source;
    m_sliceList->setDataSource(source);
}

void DsPitchLabelerPage::onSliceSelected(const QString &sliceId) {
    if (!maybeSave())
        return;

    m_currentSliceId = sliceId;
    m_currentFile.reset();

    if (!m_source) {
        m_editor->clear();
        return;
    }

    const QString audioPath = m_source->audioPath(sliceId);

    // Try to load existing .ds file from dstext
    auto result = m_source->loadSlice(sliceId);
    if (result) {
        const auto &doc = result.value();
        // Build DSFile from dstext curves/layers
        auto file = std::make_shared<pitchlabeler::DSFile>();

        // Extract pitch curve
        for (const auto &curve : doc.curves) {
            if (curve.name == QStringLiteral("pitch")) {
                file->f0.values = curve.values;
                file->f0.timestep = curve.timestep;
            }
        }

        // Extract note layers
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

    // Load audio
    if (!audioPath.isEmpty()) {
        m_editor->loadAudio(audioPath, 0.0);
    }

    emit sliceChanged(sliceId);
}

bool DsPitchLabelerPage::saveCurrentSlice() {
    if (m_currentSliceId.isEmpty() || !m_source || !m_currentFile)
        return true;

    // Save pitch/MIDI data back to dstext
    auto result = m_source->loadSlice(m_currentSliceId);
    DsTextDocument doc;
    if (result)
        doc = std::move(result.value());

    // Update pitch curve
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

    // Update MIDI layer
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

    // Mark pitch_review in editedSteps
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

bool DsPitchLabelerPage::maybeSave() {
    if (!m_currentFile)
        return true;

    // Check if editor has unsaved changes via undo stack
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

QMenuBar *DsPitchLabelerPage::createMenuBar(QWidget *parent) {
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

    auto *processMenu = bar->addMenu(QStringLiteral("处理(&P)"));
    processMenu->addAction(QStringLiteral("提取音高 (当前切片)"),
                           this, &DsPitchLabelerPage::onExtractPitch);
    processMenu->addAction(QStringLiteral("提取 MIDI (当前切片)"),
                           this, &DsPitchLabelerPage::onExtractMidi);
    processMenu->addSeparator();
    processMenu->addAction(QStringLiteral("批量提取音高 + MIDI..."),
                           this, &DsPitchLabelerPage::onBatchExtract);

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

QWidget *DsPitchLabelerPage::createStatusBarContent(QWidget *parent) {
    auto *container = new QWidget(parent);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *sliceLabel = new QLabel(QStringLiteral("未选择切片"), container);
    auto *posLabel = new QLabel(QStringLiteral("00:00.000"), container);
    auto *zoomLabel = new QLabel(QStringLiteral("100%"), container);
    layout->addWidget(sliceLabel, 1);
    layout->addWidget(posLabel);
    layout->addWidget(zoomLabel);

    connect(this, &DsPitchLabelerPage::sliceChanged, sliceLabel, [sliceLabel](const QString &id) {
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

    return container;
}

QString DsPitchLabelerPage::windowTitle() const {
    QString title = QStringLiteral("DsLabeler — 音高标注");
    if (!m_currentSliceId.isEmpty())
        title += QStringLiteral(" — ") + m_currentSliceId;
    return title;
}

bool DsPitchLabelerPage::hasUnsavedChanges() const {
    return m_editor->undoStack() && !m_editor->undoStack()->isClean();
}

void DsPitchLabelerPage::onActivated() {
    m_sliceList->refresh();
}

bool DsPitchLabelerPage::onDeactivating() {
    return maybeSave();
}

void DsPitchLabelerPage::onExtractPitch() {
    QMessageBox::information(this, QStringLiteral("提取音高"),
                             QStringLiteral("音高提取功能将在推理库集成后实现。"));
}

void DsPitchLabelerPage::onExtractMidi() {
    QMessageBox::information(this, QStringLiteral("提取 MIDI"),
                             QStringLiteral("MIDI 转录功能将在推理库集成后实现。"));
}

void DsPitchLabelerPage::onBatchExtract() {
    QMessageBox::information(this, QStringLiteral("批量提取"),
                             QStringLiteral("批量提取功能将在推理库集成后实现。"));
}

} // namespace dstools
