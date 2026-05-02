#pragma once

#include <QWidget>
#include <QLabel>
#include <QToolButton>

#include "DSFile.h"

namespace dstools {
namespace pitchlabeler {
namespace ui {

/// Collapsible bottom property panel showing note details and global statistics.
/// Layout follows the Python reference: 3 columns (note info, deviation/vibrato, global stats).
class PropertyPanel : public QWidget {
    Q_OBJECT

public:
    explicit PropertyPanel(QWidget *parent = nullptr);
    ~PropertyPanel() override;

    void setDSFile(std::shared_ptr<DSFile> ds);
    void setSelectedNote(int noteIndex);
    void clear();

    /// Mark stats cache dirty (call after F0 edits)
    void invalidateStatsCache();

signals:
    void f0EditRequested(int noteIndex);

private:
    std::shared_ptr<DSFile> m_dsFile;
    int m_selectedNoteIndex = -1;
    bool m_collapsed = false;

    // Header
    QWidget *m_header = nullptr;
    QToolButton *m_toggleBtn = nullptr;
    QLabel *m_headerLabel = nullptr;
    QWidget *m_content = nullptr;

    // Left column: note info
    QLabel *m_lblNoteName = nullptr;
    QLabel *m_lblDuration = nullptr;
    QLabel *m_lblCentsOffset = nullptr;
    QLabel *m_lblTargetFreq = nullptr;
    QLabel *m_lblF0Mean = nullptr;

    // Middle column: deviation + vibrato
    QLabel *m_lblF0Deviation = nullptr;
    QLabel *m_lblVibratoFreq = nullptr;
    QLabel *m_lblVibratoAmp = nullptr;
    QLabel *m_lblSlurStatus = nullptr;

    // Right column: global stats
    QLabel *m_lblFileName = nullptr;
    QLabel *m_lblTotalNotes = nullptr;
    QLabel *m_lblTotalRests = nullptr;
    QLabel *m_lblTotalDuration = nullptr;
    QLabel *m_lblAvgDeviation = nullptr;
    QLabel *m_lblF0Timestep = nullptr;

    void setupUI();
    void toggleCollapse();
    void clearNoteFields();
    void updateGlobalFields();

    // Helpers
    static QLabel *makeValueLabel(const QString &text = QString::fromUtf8("—"));
    static QString deviationColor(double cents);
    static QString slurLabelInContext(const std::vector<Note> &notes, int index);

    // F0 statistics for a note
    struct F0Stats {
        double meanFreq = 0.0;
        double deviationCents = 0.0;
        int sampleCount = 0;
    };
    F0Stats computeNoteF0Stats(int noteIndex) const;
};

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
