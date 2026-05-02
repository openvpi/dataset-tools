#include "PropertyPanel.h"
#include <dstools/PitchUtils.h>
#include <dstools/TimePos.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFileInfo>

#include <cmath>
#include <numeric>

namespace dstools {
namespace pitchlabeler {
namespace ui {

using dstools::NotePitch;
using dstools::parseNoteName;

// ============================================================================
// Helpers
// ============================================================================

QLabel *PropertyPanel::makeValueLabel(const QString &text) {
    auto *lbl = new QLabel(text);
    lbl->setStyleSheet("color: #D8D8E0; font-size: 11px; border: none;");
    return lbl;
}

QString PropertyPanel::deviationColor(double cents) {
    double abs_c = std::abs(cents);
    if (abs_c <= 5) return "#4ADE80";    // green
    if (abs_c <= 15) return "#FACC15";   // yellow
    return "#F87171";                     // red
}

QString PropertyPanel::slurLabelInContext(const std::vector<Note> &notes, int index) {
    if (index < 0 || index >= static_cast<int>(notes.size()))
        return QString::fromUtf8("—");

    const auto &note = notes[index];
    if (note.isRest()) return QString::fromUtf8("—");

    if (note.slur == 0) {
        // Check if next note is a slur → this is a head
        if (index + 1 < static_cast<int>(notes.size()) && notes[index + 1].slur == 1)
            return QStringLiteral("起始");
        return QStringLiteral("独立");
    }

    // This note has slur=1
    bool isTail = (index + 1 >= static_cast<int>(notes.size())) || (notes[index + 1].slur == 0);
    if (isTail) return QStringLiteral("尾部");
    return QStringLiteral("中间");
}

PropertyPanel::F0Stats PropertyPanel::computeNoteF0Stats(int noteIndex) const {
    F0Stats stats;
    if (!m_dsFile || noteIndex < 0 || noteIndex >= static_cast<int>(m_dsFile->notes.size()))
        return stats;

    const auto &note = m_dsFile->notes[noteIndex];
    if (note.isRest()) return stats;

    const auto &f0 = m_dsFile->f0;
    if (f0.values.empty() || f0.timestep <= 0) return stats;

    // Get F0 samples within the note time range (values are already MIDI)
    int startIdx = static_cast<int>((note.start - m_dsFile->offset) / f0.timestep);
    int endIdx = static_cast<int>((note.end() - m_dsFile->offset) / f0.timestep);
    startIdx = std::max(0, startIdx);
    endIdx = std::min(static_cast<int>(f0.values.size()), endIdx);

    // Parse target pitch
    auto pitch = parseNoteName(note.name);
    if (!pitch.valid) return stats;

    double targetMidi = pitch.midiNumber;

    // Collect voiced samples
    std::vector<double> midiSamples;
    for (int i = startIdx; i < endIdx; ++i) {
        int32_t val = f0.values[i];
        if (val > 0) {
            double hz = mhzToHz(val);
            double midi = 12.0 * std::log2(hz / 440.0) + 69.0;
            midiSamples.push_back(midi);
        }
    }

    if (midiSamples.empty()) return stats;

    stats.sampleCount = static_cast<int>(midiSamples.size());

    // Mean MIDI → mean freq
    double sumMidi = std::accumulate(midiSamples.begin(), midiSamples.end(), 0.0);
    double meanMidi = sumMidi / midiSamples.size();
    stats.meanFreq = 440.0 * std::pow(2.0, (meanMidi - 69.0) / 12.0);

    // Deviation from target in cents
    stats.deviationCents = (meanMidi - targetMidi) * 100.0;

    return stats;
}

// ============================================================================
// Construction
// ============================================================================

PropertyPanel::PropertyPanel(QWidget *parent)
    : QWidget(parent) {
    setupUI();
}

PropertyPanel::~PropertyPanel() = default;

void PropertyPanel::setupUI() {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // -- Header bar (collapsible) --
    m_header = new QWidget();
    m_header->setFixedHeight(28);
    m_header->setStyleSheet("background-color: #22222C; border-top: 1px solid #33333E;");
    auto *headerLayout = new QHBoxLayout(m_header);
    headerLayout->setContentsMargins(8, 0, 8, 0);

    m_toggleBtn = new QToolButton();
    m_toggleBtn->setText(QStringLiteral("▼"));
    m_toggleBtn->setFixedSize(18, 18);
    m_toggleBtn->setStyleSheet(
        "QToolButton { border: none; color: #9898A8; font-size: 10px; }"
        "QToolButton:hover { background-color: #3A3A48; border-radius: 2px; }");
    connect(m_toggleBtn, &QToolButton::clicked, this, &PropertyPanel::toggleCollapse);
    headerLayout->addWidget(m_toggleBtn);

    m_headerLabel = new QLabel(QStringLiteral("属性"));
    m_headerLabel->setStyleSheet("color: #9898A8; font-size: 11px; font-weight: bold; border: none;");
    headerLayout->addWidget(m_headerLabel);
    headerLayout->addStretch();

    outer->addWidget(m_header);

    // -- Content area: 3 columns --
    m_content = new QWidget();
    m_content->setStyleSheet("background-color: #1E1E28;");
    auto *contentLayout = new QHBoxLayout(m_content);
    contentLayout->setContentsMargins(12, 6, 12, 6);
    contentLayout->setSpacing(24);

    const QString formStyle = "QLabel { color: #9898A8; font-size: 11px; border: none; }";

    // --- Left column: note info ---
    auto *leftWidget = new QWidget();
    auto *leftForm = new QFormLayout(leftWidget);
    leftForm->setLabelAlignment(Qt::AlignRight);
    leftForm->setSpacing(3);
    leftWidget->setStyleSheet(formStyle);

    m_lblNoteName = makeValueLabel();
    leftForm->addRow(QStringLiteral("音符:"), m_lblNoteName);

    m_lblDuration = makeValueLabel();
    leftForm->addRow(QStringLiteral("时长:"), m_lblDuration);

    m_lblCentsOffset = makeValueLabel();
    leftForm->addRow(QStringLiteral("音分偏移:"), m_lblCentsOffset);

    m_lblTargetFreq = makeValueLabel();
    leftForm->addRow(QStringLiteral("目标频率:"), m_lblTargetFreq);

    m_lblF0Mean = makeValueLabel();
    leftForm->addRow(QStringLiteral("F0 均值:"), m_lblF0Mean);

    contentLayout->addWidget(leftWidget);

    // --- Middle column: deviation + vibrato ---
    auto *midWidget = new QWidget();
    auto *midForm = new QFormLayout(midWidget);
    midForm->setLabelAlignment(Qt::AlignRight);
    midForm->setSpacing(3);
    midWidget->setStyleSheet(formStyle);

    m_lblF0Deviation = makeValueLabel();
    midForm->addRow(QStringLiteral("F0 偏差:"), m_lblF0Deviation);

    m_lblVibratoFreq = makeValueLabel();
    midForm->addRow(QStringLiteral("颤音频率:"), m_lblVibratoFreq);

    m_lblVibratoAmp = makeValueLabel();
    midForm->addRow(QStringLiteral("颤音幅度:"), m_lblVibratoAmp);

    m_lblSlurStatus = makeValueLabel();
    midForm->addRow(QStringLiteral("连音:"), m_lblSlurStatus);

    contentLayout->addWidget(midWidget);

    // --- Right column: global file stats ---
    auto *rightWidget = new QWidget();
    auto *rightForm = new QFormLayout(rightWidget);
    rightForm->setLabelAlignment(Qt::AlignRight);
    rightForm->setSpacing(3);
    rightWidget->setStyleSheet(formStyle);

    m_lblFileName = makeValueLabel();
    rightForm->addRow(QStringLiteral("文件:"), m_lblFileName);

    m_lblTotalNotes = makeValueLabel();
    rightForm->addRow(QStringLiteral("音符数:"), m_lblTotalNotes);

    m_lblTotalRests = makeValueLabel();
    rightForm->addRow(QStringLiteral("休止数:"), m_lblTotalRests);

    m_lblTotalDuration = makeValueLabel();
    rightForm->addRow(QStringLiteral("总时长:"), m_lblTotalDuration);

    m_lblAvgDeviation = makeValueLabel();
    rightForm->addRow(QStringLiteral("平均偏差:"), m_lblAvgDeviation);

    m_lblF0Timestep = makeValueLabel();
    rightForm->addRow(QStringLiteral("F0 步长:"), m_lblF0Timestep);

    contentLayout->addWidget(rightWidget);

    contentLayout->addStretch();
    outer->addWidget(m_content);
}

void PropertyPanel::toggleCollapse() {
    m_collapsed = !m_collapsed;
    m_content->setVisible(!m_collapsed);
    m_toggleBtn->setText(m_collapsed ? QStringLiteral("▶") : QStringLiteral("▼"));
}

// ============================================================================
// Public API
// ============================================================================

void PropertyPanel::setDSFile(std::shared_ptr<pitchlabeler::DSFile> ds) {
    m_dsFile = ds;
    m_selectedNoteIndex = -1;
    clearNoteFields();
    updateGlobalFields();
}

void PropertyPanel::setSelectedNote(int noteIndex) {
    m_selectedNoteIndex = noteIndex;

    if (!m_dsFile || noteIndex < 0 || noteIndex >= static_cast<int>(m_dsFile->notes.size())) {
        clearNoteFields();
        return;
    }

    const auto &note = m_dsFile->notes[noteIndex];

    // Rest
    if (note.isRest()) {
        m_lblNoteName->setText(QStringLiteral("休止"));
        clearNoteFields();
        return;
    }

    // Note name
    auto pitch = parseNoteName(note.name);
    if (pitch.valid) {
        m_lblNoteName->setText(note.name);
        m_lblNoteName->setStyleSheet("color: #D8D8E0; font-size: 11px; border: none; font-weight: bold;");
        m_lblCentsOffset->setText(QString("%1%2 ¢")
            .arg(pitch.cents > 0 ? "+" : "")
            .arg(pitch.cents));

        // Target frequency
        double targetFreq = 440.0 * std::pow(2.0, (pitch.midiNumber - 69.0) / 12.0);
        m_lblTargetFreq->setText(QString::number(targetFreq, 'f', 1) + " Hz");
    } else {
        m_lblNoteName->setText(note.name);
        m_lblCentsOffset->setText(QString::fromUtf8("—"));
        m_lblTargetFreq->setText(QString::fromUtf8("—"));
    }

    // Duration in ms
    m_lblDuration->setText(QString::number(usToMs(note.duration), 'f', 1) + " ms");

    // F0 statistics
    auto stats = computeNoteF0Stats(noteIndex);
    if (stats.meanFreq > 0) {
        m_lblF0Mean->setText(QString::number(stats.meanFreq, 'f', 1) + " Hz");
    } else {
        m_lblF0Mean->setText(QString::fromUtf8("—"));
    }

    if (stats.sampleCount > 0) {
        double dev = stats.deviationCents;
        QString color = deviationColor(dev);
        m_lblF0Deviation->setText(QString("%1%2 ¢")
            .arg(dev > 0 ? "+" : "")
            .arg(dev, 0, 'f', 1));
        m_lblF0Deviation->setStyleSheet(
            QString("color: %1; font-size: 11px; border: none; font-weight: bold;").arg(color));
    } else {
        m_lblF0Deviation->setText(QString::fromUtf8("—"));
        m_lblF0Deviation->setStyleSheet("color: #D8D8E0; font-size: 11px; border: none;");
    }

    // Vibrato (placeholder — basic estimation not implemented yet)
    m_lblVibratoFreq->setText(QString::fromUtf8("—"));
    m_lblVibratoAmp->setText(QString::fromUtf8("—"));

    // Slur status
    m_lblSlurStatus->setText(slurLabelInContext(m_dsFile->notes, noteIndex));

    // Also update global
    updateGlobalFields();
}

void PropertyPanel::clear() {
    m_dsFile.reset();
    m_selectedNoteIndex = -1;
    clearNoteFields();

    m_lblFileName->setText(QString::fromUtf8("—"));
    m_lblTotalNotes->setText(QString::fromUtf8("—"));
    m_lblTotalRests->setText(QString::fromUtf8("—"));
    m_lblTotalDuration->setText(QString::fromUtf8("—"));
    m_lblAvgDeviation->setText(QString::fromUtf8("—"));
    m_lblF0Timestep->setText(QString::fromUtf8("—"));
}

void PropertyPanel::invalidateStatsCache() {
    // Re-compute when next note is selected
    if (m_dsFile && m_selectedNoteIndex >= 0) {
        setSelectedNote(m_selectedNoteIndex);
    }
}

// ============================================================================
// Internal
// ============================================================================

void PropertyPanel::clearNoteFields() {
    const QString dash = QString::fromUtf8("—");
    m_lblNoteName->setText(dash);
    m_lblNoteName->setStyleSheet("color: #D8D8E0; font-size: 11px; border: none;");
    m_lblDuration->setText(dash);
    m_lblCentsOffset->setText(dash);
    m_lblTargetFreq->setText(dash);
    m_lblF0Mean->setText(dash);
    m_lblF0Deviation->setText(dash);
    m_lblF0Deviation->setStyleSheet("color: #D8D8E0; font-size: 11px; border: none;");
    m_lblVibratoFreq->setText(dash);
    m_lblVibratoAmp->setText(dash);
    m_lblSlurStatus->setText(dash);
}

void PropertyPanel::updateGlobalFields() {
    if (!m_dsFile) return;

    // File name
    if (!m_dsFile->filePath().isEmpty()) {
        m_lblFileName->setText(QFileInfo(m_dsFile->filePath()).fileName());
    } else {
        m_lblFileName->setText(QString::fromUtf8("—"));
    }

    // Note / rest counts
    int noteCount = 0, restCount = 0;
    for (const auto &n : m_dsFile->notes) {
        if (n.isRest()) restCount++;
        else noteCount++;
    }
    m_lblTotalNotes->setText(QString::number(noteCount));
    m_lblTotalRests->setText(QString::number(restCount));

    // Total duration
    if (!m_dsFile->notes.empty()) {
        double totalDur = usToSec(m_dsFile->notes.back().end() - m_dsFile->offset);
        m_lblTotalDuration->setText(QString::number(totalDur, 'f', 2) + " s");
    } else {
        m_lblTotalDuration->setText(QString::fromUtf8("—"));
    }

    // F0 timestep
    double ts = usToMs(m_dsFile->f0.timestep);
    m_lblF0Timestep->setText(ts > 0 ? QString::number(ts, 'f', 1) + " ms" : QString::fromUtf8("—"));

    // Average F0 deviation across all non-rest non-slur notes
    double totalAbsDev = 0.0;
    int devCount = 0;
    for (int i = 0; i < static_cast<int>(m_dsFile->notes.size()); ++i) {
        const auto &n = m_dsFile->notes[i];
        if (n.isRest() || n.isSlur()) continue;
        auto stats = computeNoteF0Stats(i);
        if (stats.sampleCount > 0) {
            totalAbsDev += std::abs(stats.deviationCents);
            devCount++;
        }
    }
    if (devCount > 0) {
        double avgDev = totalAbsDev / devCount;
        QString color = deviationColor(avgDev);
        m_lblAvgDeviation->setText(QString::number(avgDev, 'f', 1) + QString::fromUtf8(" ¢"));
        m_lblAvgDeviation->setStyleSheet(
            QString("color: %1; font-size: 11px; border: none; font-weight: bold;").arg(color));
    } else {
        m_lblAvgDeviation->setText(QString::fromUtf8("—"));
        m_lblAvgDeviation->setStyleSheet("color: #D8D8E0; font-size: 11px; border: none;");
    }
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
