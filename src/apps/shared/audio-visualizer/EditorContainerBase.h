#pragma once

#include "AudioEditorWidgetBase.h"
#include "MouthCurveChartPanel.h"

#include <IBoundaryModel.h>
#include <QIcon>
#include <QString>
#include <dsfw/widgets/PlayWidget.h>
#include <dsfw/widgets/ViewportController.h>

class QAction;
class QSplitter;
class QToolBar;

namespace dstools {

    class AudioVisualizerContainer;

    namespace chart {
        class WaveformChartPanel;
        class SpectrogramChartPanel;
        class PowerChartPanel;
    }
    namespace phonemelabeler {
        class MouthCurveChartPanel;
    }

    class EditorContainerBase : public AudioEditorWidgetBase {
        Q_OBJECT
    public:
        explicit EditorContainerBase(const QString &settingsGroup, QWidget *parent = nullptr);
        ~EditorContainerBase() override;

        AudioVisualizerContainer *container() const {
            return m_container;
        }
        dsfw::widgets::ViewportController *viewport() const {
            return m_viewport;
        }
        dsfw::widgets::PlayWidget *playWidget() const {
            return m_playWidget;
        }

        chart::WaveformChartPanel *waveformChart() const {
            return m_waveformChart;
        }
        chart::SpectrogramChartPanel *spectrogramChart() const {
            return m_spectrogramChart;
        }
        chart::PowerChartPanel *powerChart() const {
            return m_powerChart;
        }
        MouthCurveChartPanel *mouthCurveChart() const {
            return m_mouthCurveChart;
        }

        int defaultResolution() const {
            return m_defaultResolution;
        }

        void onZoomIn();
        void onZoomOut();
        void onZoomReset();

        void setupSidebarToggle(QSplitter *mainSplitter, QToolBar *toolbar);

        QByteArray saveSplitterState() const;
        void restoreSplitterState(const QByteArray &state);
        void saveViewportResolution() const;
        void restoreViewportResolution() const;
        void setViewportResolutionKey(const QString &key);

        [[nodiscard]] QAction *playPauseAction() const { return m_actPlayPause; }
        [[nodiscard]] QAction *stopAction() const { return m_actStop; }

    signals:
        void zoomChanged(int resolution);

    protected:
        void setupContainerAndPlayWidget(int defaultResolution);

        void addWaveformChart(int tierIndex, int panelIndex, double stretch);
        void addSpectrogramChart(int tierIndex, int panelIndex, double stretch);
        void addPowerChart(int tierIndex, int panelIndex, double stretch);
        void addMouthCurveChart(int tierIndex, int panelIndex, double stretch);
        void setAllChartsBoundaryModel(chart::IBoundaryModel *model);

        void setMouthCurveData(const MouthCurve &curve);

        void updateZoomActions(class QAction *in, class QAction *out, class QAction *reset);

        void createPlaybackActions();

        virtual void onPlayPause();
        virtual void onStop();

        void updatePlayPauseIcon();

        AudioVisualizerContainer *m_container = nullptr;
        dsfw::widgets::ViewportController *m_viewport = nullptr;
        dsfw::widgets::PlayWidget *m_playWidget = nullptr;

        chart::WaveformChartPanel *m_waveformChart = nullptr;
        chart::SpectrogramChartPanel *m_spectrogramChart = nullptr;
        chart::PowerChartPanel *m_powerChart = nullptr;
        MouthCurveChartPanel *m_mouthCurveChart = nullptr;

        int m_defaultResolution = 800;

        QSplitter *m_mainSplitter = nullptr;
        QAction *m_actToggleSidebar = nullptr;
        QList<int> m_sidebarExpandedSize;

        QAction *m_actPlayPause = nullptr;
        QAction *m_actStop = nullptr;

    private:
        QString m_settingsGroup;
    };

} // namespace dstools