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
#include <ChartPageBase.h>
#include <SliceTierLabel.h>

#include <map>

namespace dstools {

class AudioFileListPanel;

class SliceListPanel;

class SlicerPage : public ChartPageBase, public labeler::IPageActions, public labeler::IPageLifecycle {
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

protected:
    QUndoStack *m_undoStack = nullptr;

    SliceTierLabel *m_tierLabel = nullptr;

    phonemelabeler::SliceBoundaryModel *m_boundaryModel = nullptr;

    AudioFileListPanel *m_audioFileList = nullptr;
    QSplitter *m_hSplitter = nullptr;
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

    virtual void buildLayout();
    virtual void connectSignals();
    virtual void onAutoSlice();
    virtual void onImportMarkers();
    virtual void onSaveMarkers();
    virtual void onExportAudio();
    virtual void onBatchExportAll();
    virtual void onOpenAudioFiles();
    virtual void onOpenAudioDirectory();
    virtual void refreshBoundaries();
    virtual void updateSlicerListPanel();
    virtual void saveCurrentSlicePoints();
    virtual void loadSlicePointsForFile(const QString &filePath);
    virtual void updateFileProgress();
    virtual void autoSliceFiles(const QStringList &filePaths);
    virtual void loadAudioFile(const QString &filePath);

    int performBatchExport(const QString &outputDir, int digits, int sndFormat);

    void keyPressEvent(QKeyEvent *event) override;
};

} // namespace dstools
