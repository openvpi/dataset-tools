#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/AppSettings.h>

#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QToolBar>
#include <QToolButton>
#include <QUndoStack>
#include <QWidget>

#include <dstools/ViewportController.h>
#include <dstools/PlayWidget.h>
#include <ui/SliceBoundaryModel.h>

#include <AudioVisualizerContainer.h>
#include <SliceTierLabel.h>

#include <map>

namespace dstools {

class AudioFileListPanel;

namespace phonemelabeler {
class WaveformChartPanel;
class SpectrogramChartPanel;
class PowerChartPanel;
} // namespace phonemelabeler

class SliceListPanel;

class SlicerPage : public QWidget, public labeler::IPageActions, public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit SlicerPage(QWidget *parent = nullptr);
    ~SlicerPage() override;

    // IPageActions
    QMenuBar *createMenuBar(QWidget *parent) override;
    QString windowTitle() const override;

    // IPageLifecycle
    void onActivated() override;
    bool onDeactivating() override;

private:
    QUndoStack *m_undoStack = nullptr;

    dstools::widgets::ViewportController *m_viewport = nullptr;
    AudioVisualizerContainer *m_container = nullptr;
    SliceTierLabel *m_tierLabel = nullptr;

    phonemelabeler::SliceBoundaryModel *m_boundaryModel = nullptr;

    dstools::widgets::PlayWidget *m_playWidget = nullptr;

    AudioFileListPanel *m_audioFileList = nullptr;
    QSplitter *m_hSplitter = nullptr;
    phonemelabeler::WaveformChartPanel *m_waveformChartPanel = nullptr;
    phonemelabeler::SpectrogramChartPanel *m_spectrogramChartPanel = nullptr;
    SliceListPanel *m_sliceListPanel = nullptr;

    std::vector<float> m_samples;
    int m_sampleRate = 44100;

    class QDoubleSpinBox *m_thresholdSpin = nullptr;
    class QSpinBox *m_minLengthSpin = nullptr;
    class QSpinBox *m_minIntervalSpin = nullptr;
    class QSpinBox *m_hopSizeSpin = nullptr;
    class QSpinBox *m_maxSilenceSpin = nullptr;

    class QPushButton *m_btnAutoSlice = nullptr;
    class QPushButton *m_btnReSlice = nullptr;
    class QPushButton *m_btnImportMarkers = nullptr;
    class QPushButton *m_btnSaveMarkers = nullptr;
    class QPushButton *m_btnExportAudio = nullptr;

    QToolBar *m_toolbar = nullptr;
    QToolButton *m_btnPointer = nullptr;
    QToolButton *m_btnKnife = nullptr;

    enum class ToolMode { Pointer, Knife };
    ToolMode m_toolMode = ToolMode::Pointer;

    std::vector<double> m_slicePoints;
    int m_selectedBoundary = -1;

    std::map<QString, std::vector<double>> m_fileSlicePoints;
    QString m_currentAudioPath;

    dstools::AppSettings m_settings;

    void buildLayout();
    void connectSignals();
    void onAutoSlice();
    void onImportMarkers();
    void onSaveMarkers();
    void onExportAudio();
    void onBatchExportAll();
    void onOpenAudioFiles();
    void onOpenAudioDirectory();
    void refreshBoundaries();
    void updateSlicerListPanel();
    void saveCurrentSlicePoints();
    void loadSlicePointsForFile(const QString &filePath);
    void updateFileProgress();
    void autoSliceFiles(const QStringList &filePaths);
    void loadAudioFile(const QString &filePath);

protected:
    void keyPressEvent(QKeyEvent *event) override;
};

} // namespace dstools
