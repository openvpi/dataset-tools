/// @file DsSlicerPage.h
/// @brief DsLabeler slicer page — auto-slice + manual slice editing + export.

#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSpinBox>
#include <QToolBar>
#include <QToolButton>
#include <QUndoStack>
#include <QWidget>

#include <ui/SliceBoundaryModel.h>

#include <dstools/PlayWidget.h>

#include <AudioVisualizerContainer.h>
#include <SliceTierLabel.h>

#include <map>

namespace dstools {

    class ProjectDataSource;
    class AudioFileListPanel;

    namespace phonemelabeler {
        class WaveformChartPanel;
        class SpectrogramChartPanel;
    } // namespace phonemelabeler

    class SliceListPanel;

    class DsSlicerPage : public QWidget, public labeler::IPageActions, public labeler::IPageLifecycle {
        Q_OBJECT
        Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

    public:
        explicit DsSlicerPage(QWidget *parent = nullptr);
        ~DsSlicerPage() override;

        void setDataSource(ProjectDataSource *source);

        // IPageActions
        QMenuBar *createMenuBar(QWidget *parent) override;
        QString windowTitle() const override;

        // IPageLifecycle
        void onActivated() override;
        bool onDeactivating() override;

    private:
        ProjectDataSource *m_dataSource = nullptr;
        QUndoStack *m_undoStack = nullptr;

        // Boundary model
        phonemelabeler::SliceBoundaryModel *m_boundaryModel = nullptr;

        // Playback
        dstools::widgets::PlayWidget *m_playWidget = nullptr;

        // AudioVisualizerContainer (owns viewport, miniMap, timeRuler, tierLabel, charts)
        AudioVisualizerContainer *m_container = nullptr;

        // UI components
        AudioFileListPanel *m_audioFileList = nullptr;
        phonemelabeler::WaveformChartPanel *m_waveformWidget = nullptr;
        phonemelabeler::SpectrogramChartPanel *m_spectrogramWidget = nullptr;
        SliceListPanel *m_sliceListPanel = nullptr;

        // Audio data
        std::vector<float> m_samples;
        int m_sampleRate = 44100;

        // Slice params
        class QDoubleSpinBox *m_thresholdSpin = nullptr;
        class QSpinBox *m_minLengthSpin = nullptr;
        class QSpinBox *m_minIntervalSpin = nullptr;
        class QSpinBox *m_hopSizeSpin = nullptr;
        class QSpinBox *m_maxSilenceSpin = nullptr;

        // Action buttons
        class QPushButton *m_btnAutoSlice = nullptr;
        class QPushButton *m_btnReSlice = nullptr;
        class QPushButton *m_btnImportMarkers = nullptr;
        class QPushButton *m_btnSaveMarkers = nullptr;
        class QPushButton *m_btnExportAudio = nullptr;

        // Tool buttons
        QToolBar *m_toolbar = nullptr;
        QToolButton *m_btnPointer = nullptr;
        QToolButton *m_btnKnife = nullptr;

        // Tool mode
        enum class ToolMode { Pointer, Knife };
        ToolMode m_toolMode = ToolMode::Pointer;

        // Slice boundary times (seconds)
        std::vector<double> m_slicePoints;
        int m_selectedBoundary = -1;

        // Per-file slice points storage (filePath → slice points)
        std::map<QString, std::vector<double>> m_fileSlicePoints;
        QString m_currentAudioPath;

        void buildLayout();
        void connectSignals();
        void onAutoSlice();
        void onImportMarkers();
        void onSaveMarkers();
        void onExportAudio();
        void onBatchExportAll();
        void onExportMenu();
        void onOpenAudioFiles();
        void onOpenAudioDirectory();
        void refreshBoundaries();
        void updateSlicerListPanel();
        void saveCurrentSlicePoints();
        void loadSlicePointsForFile(const QString &filePath);
        void updateFileProgress();
        void autoSliceFiles(const QStringList &filePaths);
        void promptSliceUpdateIfNeeded();
        void saveSlicerParamsToProject();
        void saveSlicerStateToProject();
        void loadAudioFile(const QString &filePath);

    protected:
        void keyPressEvent(QKeyEvent *event) override;
    };

} // namespace dstools
