#pragma once

#include "IBoundaryModel.h"


#include <QString>
#include <QWidget>

#include <dstools/ViewportController.h>
#include <dstools/PlayWidget.h>

namespace dstools {

class AudioVisualizerContainer;

namespace phonemelabeler {
class WaveformChartPanel;
class SpectrogramChartPanel;
class PowerChartPanel;
}

class ChartPageBase : public QWidget {
    Q_OBJECT
public:
    explicit ChartPageBase(const QString &settingsGroup, QWidget *parent = nullptr);
    ~ChartPageBase() override;

    AudioVisualizerContainer *container() const { return m_container; }
    widgets::ViewportController *viewport() const { return m_viewport; }
    widgets::PlayWidget *playWidget() const { return m_playWidget; }

    phonemelabeler::WaveformChartPanel *waveformChart() const { return m_waveformChart; }
    phonemelabeler::SpectrogramChartPanel *spectrogramChart() const { return m_spectrogramChart; }
    phonemelabeler::PowerChartPanel *powerChart() const { return m_powerChart; }

    int defaultResolution() const { return m_defaultResolution; }

    void onZoomIn();
    void onZoomOut();
    void onZoomReset();

signals:
    void zoomChanged(int resolution);

protected:
    void setupContainerAndPlayWidget(int defaultResolution);

    void addWaveformChart(int tierIndex, int panelIndex, double stretch);
    void addSpectrogramChart(int tierIndex, int panelIndex, double stretch);
    void addPowerChart(int tierIndex, int panelIndex, double stretch);
    void setAllChartsBoundaryModel(phonemelabeler::IBoundaryModel *model);

    void updateZoomActions(class QAction *in, class QAction *out, class QAction *reset);

    AudioVisualizerContainer *m_container = nullptr;
    widgets::ViewportController *m_viewport = nullptr;
    widgets::PlayWidget *m_playWidget = nullptr;

    phonemelabeler::WaveformChartPanel *m_waveformChart = nullptr;
    phonemelabeler::SpectrogramChartPanel *m_spectrogramChart = nullptr;
    phonemelabeler::PowerChartPanel *m_powerChart = nullptr;

    int m_defaultResolution = 800;

private:
    QString m_settingsGroup;
};

} // namespace dstools