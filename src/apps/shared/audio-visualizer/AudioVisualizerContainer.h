#pragma once

/// @file AudioVisualizerContainer.h
/// @brief Base container for audio visualization pages (Slicer, Phoneme, etc.)
///
/// Provides a standardized vertical layout:
///   MiniMapScrollBar → TimeRuler → TierLabelArea → QSplitter(charts)
/// with shared ViewportController, IBoundaryModel, and BoundaryOverlayWidget.

#include <QByteArray>
#include <QMap>
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
} // namespace phonemelabeler

using phonemelabeler::IBoundaryModel;
using phonemelabeler::BoundaryOverlayWidget;
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
    TimeRulerWidget *timeRuler() const;
    BoundaryOverlayWidget *boundaryOverlay() const;
    TierLabelArea *tierLabelArea() const;
    QSplitter *chartSplitter() const;
    MiniMapScrollBar *miniMap() const;

    void setBoundaryModel(IBoundaryModel *model);
    void setTierLabelArea(TierLabelArea *area);

    /// Set an editor widget to be placed between TierLabelArea and chartSplitter.
    /// This is used for TierEditWidget (phoneme interval editing).
    void setEditorWidget(QWidget *widget);
    QWidget *editorWidget() const { return m_editorWidget; }

    void setTotalDuration(double seconds);

    void setAudioData(const std::vector<float> &samples, int sampleRate);

    void fitToWindow();

    void addChart(const QString &id, QWidget *widget, int defaultOrder,
                  int stretchFactor = 1, double heightWeight = 1.0);
    void removeChart(const QString &id);
    QStringList chartOrder() const;
    void setChartOrder(const QStringList &order);

    QByteArray saveSplitterState() const;
    void restoreSplitterState(const QByteArray &state);

    void setPlayWidget(dsfw::widgets::PlayWidget *playWidget);

    /// Notify that boundary model data has changed (boundaries added/moved/removed,
    /// tier structure changed). Refreshes overlay, tier label, and all chart widgets.
    void invalidateBoundaryModel();

    /// Update the scale indicator text from the current viewport state.
    void updateScaleIndicator();

signals:
    void chartOrderChanged(const QStringList &order);

    /// Emitted after invalidateBoundaryModel() completes internal refresh.
    void boundaryModelInvalidated();

private:
    void rebuildChartLayout();
    void connectViewportToWidget(QWidget *widget);
    void applyDefaultHeightRatios();
    void updateOverlayTopOffset();
    void resizeEvent(QResizeEvent *event) override;

    // eventFilter: track editor widget resize
    bool eventFilter(QObject *watched, QEvent *event) override;

    widgets::ViewportController *m_viewport = nullptr;
    IBoundaryModel *m_boundaryModel = nullptr;
    BoundaryOverlayWidget *m_boundaryOverlay = nullptr;
    TimeRulerWidget *m_timeRuler = nullptr;
    TierLabelArea *m_tierLabelArea = nullptr;
    QWidget *m_editorWidget = nullptr;
    QSplitter *m_chartSplitter = nullptr;
    MiniMapScrollBar *m_miniMap = nullptr;
    QLabel *m_scaleLabel = nullptr;
    dsfw::widgets::PlayWidget *m_playWidget = nullptr;

    QMap<QString, ChartEntry> m_charts;
    QStringList m_chartOrder;

    AppSettings m_settings;
    bool m_needsFitOnResize = false; ///< fitToWindow was deferred because width was 0
};

} // namespace dstools
