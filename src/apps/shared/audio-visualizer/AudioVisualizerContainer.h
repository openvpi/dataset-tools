#pragma once

/// @file AudioVisualizerContainer.h
/// @brief Base container for audio visualization pages (Slicer, Phoneme, etc.)
///
/// Provides a standardized vertical layout:
///   MiniMapScrollBar → TimeRuler → TierLabelArea → QSplitter(charts)
/// with shared ViewportController, IBoundaryModel, and BoundaryOverlayWidget.

#include <QByteArray>
#include <QMap>
#include <QSet>
#include <QSplitter>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

#include <dsfw/AppSettings.h>
#include <dsfw/widgets/PlayWidget.h>
#include <dsfw/widgets/TimeRulerWidget.h>
#include <dstools/ViewportController.h>

class QLabel;

namespace dstools {

namespace phonemelabeler {
class IBoundaryModel;
class BoundaryOverlayWidget;
class BoundaryDragController;
} // namespace phonemelabeler

using phonemelabeler::IBoundaryModel;
using phonemelabeler::BoundaryOverlayWidget;
using phonemelabeler::BoundaryDragController;
using TimeRulerWidget = dsfw::widgets::TimeRulerWidget;

class TierLabelArea;
class MiniMapScrollBar;

struct ChartEntry {
    QString id;
    QWidget *widget = nullptr;
    int defaultOrder = 0;
    int stretchFactor = 1;
    double heightWeight = 1.0;
};

class AudioVisualizerContainer : public QWidget {
    Q_OBJECT

public:
    explicit AudioVisualizerContainer(const QString &settingsAppName,
                                      QWidget *parent = nullptr);
    ~AudioVisualizerContainer() override;

    widgets::ViewportController *viewport() const;
    IBoundaryModel *boundaryModel() const;
    BoundaryDragController *dragController() const;
    TimeRulerWidget *timeRuler() const;
    BoundaryOverlayWidget *boundaryOverlay() const;
    TierLabelArea *tierLabelArea() const;
    QSplitter *chartSplitter() const;
    MiniMapScrollBar *miniMap() const;

    void setBoundaryModel(IBoundaryModel *model);
    void setTierLabelArea(TierLabelArea *area);

    /// Remove and delete the tier label area from the layout.
    /// Used by pages (e.g. PhonemeEditor) that don't need the tier label area.
    void removeTierLabelArea();

    /// Set an editor widget to be placed between TierLabelArea and chartSplitter.
    /// This is used for TierEditWidget (phoneme interval editing).
    void setEditorWidget(QWidget *widget);
    QWidget *editorWidget() const { return m_editorWidget; }

    void setTotalDuration(double seconds);

    void setAudioData(const std::vector<float> &samples, int sampleRate);

    /// Set default resolution for this container (samples per pixel).
    /// Called once during page setup.
    void setDefaultResolution(int resolution);

    /// Save current resolution to per-page persistent settings.
    void saveResolution();

    /// Restore resolution from per-page persistent settings.
    /// Returns true if a saved value was found and applied.
    bool restoreResolution();

    void fitToWindow();

    void addChart(const QString &id, QWidget *widget, int defaultOrder,
                  int stretchFactor = 1, double heightWeight = 1.0);
    void removeChart(const QString &id);
    QStringList chartOrder() const;
    void setChartOrder(const QStringList &order);

    /// Return the shared global AppSettings used for chart layout persistence
    /// (chartOrder + chartVisible).  Both AudioVisualizerContainer and
    /// SettingsPage must use this instance so they read/write the same store.
    static AppSettings &chartLayoutSettings();

    /// Set visibility of a chart widget by id.
    /// Hidden charts are excluded from the splitter layout.
    void setChartVisible(const QString &id, bool visible);

    /// Return whether a chart is currently visible.
    bool chartVisible(const QString &id) const;

    /// Save current chart visibility state to global AppSettings.
    void saveChartVisibility();

    /// Restore chart visibility from global AppSettings.
    /// Charts not listed in the saved state keep their current visibility.
    void restoreChartVisibility();

    /// Save current chart order to global AppSettings.
    void saveChartOrder();

    /// Restore chart order from global AppSettings.
    /// Returns true if a saved order was found and applied.
    bool restoreChartOrder();

    QByteArray saveSplitterState() const;
    void restoreSplitterState(const QByteArray &state);

    void setPlayWidget(dsfw::widgets::PlayWidget *playWidget);

    /// Convenience: connect playhead to a chart widget that has setPlayhead(double).
    void connectPlayheadToWidget(QWidget *chart);

    /// Notify that boundary model data has changed (boundaries added/moved/removed,
    /// tier structure changed). Refreshes overlay, tier label, and all chart widgets.
    void invalidateBoundaryModel();

    /// Update the scale indicator text from the current viewport state.
    void updateScaleIndicator();

    /// Recalculate the view range to match current resolution + widget width.
    /// Call after setResolution() or zoom changes to keep the viewport consistent.
    void updateViewRangeFromResolution();

    /// Adjust view range to match current resolution, preserving the view center.
    /// Unlike updateViewRangeFromResolution (preserves startSec), this keeps the
    /// time-center stable so zoom operations don't "snap back" to the full view.
    void adjustViewRangeToResolution();

signals:
    void chartOrderChanged(const QStringList &order);

    /// Emitted when a chart's visibility changes.
    void chartVisibilityChanged(const QString &id, bool visible);

    /// Emitted after invalidateBoundaryModel() completes internal refresh.
    void boundaryModelInvalidated();

private:
    void rebuildChartLayout();
    void connectViewportToWidget(QWidget *widget);
    void applyDefaultHeightRatios();
    void updateOverlayTopOffset();
    double xToTimeGlobal(qreal globalX) const;
    void installDragEventFilters();
    void removeDragEventFilters();
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    widgets::ViewportController *m_viewport = nullptr;
    IBoundaryModel *m_boundaryModel = nullptr;
    BoundaryDragController *m_dragController = nullptr;
    BoundaryOverlayWidget *m_boundaryOverlay = nullptr;
    TimeRulerWidget *m_timeRuler = nullptr;
    TierLabelArea *m_tierLabelArea = nullptr;
    QWidget *m_editorWidget = nullptr;
    QSplitter *m_chartSplitter = nullptr;
    MiniMapScrollBar *m_miniMap = nullptr;
    QLabel *m_scaleLabel = nullptr;
    dsfw::widgets::PlayWidget *m_playWidget = nullptr;
    QTimer *m_playheadTimer = nullptr;

    QMap<QString, ChartEntry> m_charts;
    QStringList m_chartOrder;
    QSet<QString> m_hiddenCharts; ///< Chart IDs that are currently hidden

    AppSettings m_settings;
    bool m_needsFitOnResize = false; ///< fitToWindow was deferred because width was 0
    QByteArray m_pendingSplitterState; ///< splitter state deferred until layout is valid
    int m_defaultResolution = 40;
};

} // namespace dstools
