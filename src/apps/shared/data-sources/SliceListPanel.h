#pragma once

#include <QListWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <vector>

namespace dsfw::widgets {
class FileProgressTracker;
}

namespace dstools {

class IEditorDataSource;

class SliceListPanel : public QWidget {
    Q_OBJECT

public:
    explicit SliceListPanel(QWidget *parent = nullptr);
    ~SliceListPanel() override = default;

    // ── Editor mode (used by MinLabel, Phoneme, Pitch pages) ──────────────
    void setDataSource(IEditorDataSource *source);
    void refresh();
    void setCurrentSlice(const QString &sliceId);
    [[nodiscard]] QString currentSliceId() const;
    void selectNext();
    void selectPrev();
    [[nodiscard]] int sliceCount() const;
    void setProgress(int completed, int total);
    [[nodiscard]] dsfw::widgets::FileProgressTracker *progressTracker() const;

    // ── Slicer mode (used by SlicerPage, DsSlicerPage) ────────────────────
    void setSlicerMode(bool enabled);
    void setSliceData(const std::vector<double> &slicePoints, double totalDuration,
                      const QString &prefix = QStringLiteral("slice"));
    void setDiscarded(int index, bool discarded);
    [[nodiscard]] std::vector<int> discardedIndices() const { return m_discarded; }

signals:
    void sliceSelected(const QString &sliceId);

    // Slicer mode signals
    void sliceDoubleClicked(int index, double startSec, double endSec);
    void discardToggled(int index, bool discarded);
    void addSlicePointRequested(double timeSec);
    void removeSlicePointRequested(int pointIndex);

private:
    QListWidget *m_listWidget = nullptr;
    IEditorDataSource *m_source = nullptr;
    dsfw::widgets::FileProgressTracker *m_progressTracker = nullptr;

    // Slicer mode state
    bool m_slicerMode = false;
    std::vector<double> m_slicePoints;
    double m_totalDuration = 0.0;
    std::vector<int> m_discarded;

    static constexpr double kMinDuration = 0.5;
    static constexpr double kMaxDuration = 30.0;

    void onCurrentRowChanged(int row);
    void rebuildSlicerList(const QString &prefix);
    void onContextMenu(const QPoint &pos);
};

} // namespace dstools
