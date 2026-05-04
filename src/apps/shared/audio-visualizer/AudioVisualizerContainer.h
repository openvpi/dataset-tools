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
    TierLabelArea *tierLabelArea() const;
    QSplitter *chartSplitter() const;
    MiniMapScrollBar *miniMap() const;

    void setBoundaryModel(IBoundaryModel *model);
    void setTotalDuration(double seconds);

    void setAudioData(const std::vector<float> &samples, int sampleRate);

    void addChart(const QString &id, QWidget *widget, int defaultOrder, int stretchFactor = 1);
    void removeChart(const QString &id);
    QStringList chartOrder() const;
    void setChartOrder(const QStringList &order);

    QByteArray saveSplitterState() const;
    void restoreSplitterState(const QByteArray &state);

    void setPlayWidget(dsfw::widgets::PlayWidget *playWidget);

signals:
    void chartOrderChanged(const QStringList &order);

private:
    void rebuildChartLayout();
    void connectViewportToWidget(QWidget *widget);

    widgets::ViewportController *m_viewport = nullptr;
    IBoundaryModel *m_boundaryModel = nullptr;
    BoundaryOverlayWidget *m_boundaryOverlay = nullptr;
    TimeRulerWidget *m_timeRuler = nullptr;
    TierLabelArea *m_tierLabelArea = nullptr;
    QSplitter *m_chartSplitter = nullptr;
    MiniMapScrollBar *m_miniMap = nullptr;
    dsfw::widgets::PlayWidget *m_playWidget = nullptr;

    QMap<QString, ChartEntry> m_charts;
    QStringList m_chartOrder;

    AppSettings m_settings;
};

} // namespace dstools
