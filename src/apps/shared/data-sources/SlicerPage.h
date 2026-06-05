#pragma once

#include "AudioVisualizerContainer.h"
#include "EditorContainerBase.h"

#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QToolBar>
#include <QToolButton>
#include <QUndoStack>
#include <QWidget>
#include <SliceExportDialog.h>
#include <SliceTierLabel.h>
#include <dsfw/AppSettings.h>
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dstools/Constants.h>
#include <dsfw/widgets/PlayWidget.h>
#include <dsfw/widgets/ViewportController.h>
#include <dsfw/audio/AudioBuffer.h>
#include <map>
#include <ui/SliceBoundaryModel.h>

namespace dstools {

    class AudioFileListPanel;
    class SliceListPanel;

    class SlicerPage : public EditorContainerBase, public labeler::IPageActions, public labeler::IPageLifecycle {
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
        SliceTierLabel *m_tierLabel = nullptr;

        std::unique_ptr<phonemelabeler::SliceBoundaryModel> m_boundaryModel;

        AudioFileListPanel *m_audioFileList = nullptr;
        QSplitter *m_hSplitter = nullptr;
        QSplitter *m_contentSplitter = nullptr;
        SliceListPanel *m_sliceListPanel = nullptr;

        std::vector<float> m_samples;
        int m_sampleRate = constants::kDefaultSampleRate;

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
        QToolButton *m_btnToggleSidebar = nullptr;

        enum class ToolMode { Pointer, Knife };
        ToolMode m_toolMode = ToolMode::Pointer;

        std::vector<double> m_slicePoints;
        int m_selectedBoundary = -1;

        std::map<QString, std::vector<double>> m_fileSlicePoints;
        QString m_currentAudioPath;

        dstools::AppSettings m_settings;

        QList<int> m_sidebarExpandedSize;

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

        int performBatchExport(const QString &outputDir, int digits, dsfw::audio::SampleFormat sampleFmt);

        static dsfw::audio::SampleFormat bitDepthToSampleFormat(SliceExportDialog::BitDepth bitDepth);

        void keyPressEvent(QKeyEvent *event) override;
    };

} // namespace dstools
