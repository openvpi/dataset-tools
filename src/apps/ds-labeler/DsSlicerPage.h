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

namespace dstools {

    class ProjectDataSource;
    class AudioFileListPanel;

    namespace waveform {
        class WaveformPanel;
        class MelSpectrogramWidget;
    } // namespace waveform

    class SliceNumberLayer;
    class SlicerListPanel;

    /// @brief Slicer page for DsLabeler (ADR-61, ADR-64).
    ///
    /// Provides auto-slice, manual slice editing (add/move/delete cut lines),
    /// and slice audio export. Slice parameters are embedded directly in this
    /// page (not in Settings). Layout:
    ///   - Left sidebar: AudioFileListPanel (drag-drop audio files)
    ///   - Slice params panel + action buttons
    ///   - WaveformPanel (TimeRuler + Waveform + scrollbar)
    ///   - MelSpectrogramWidget (collapsible)
    ///   - SliceNumberLayer (numeric labels for each segment)
    ///   - SlicerListPanel (bottom panel with slice list)
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

    private:
        ProjectDataSource *m_dataSource = nullptr;
        QUndoStack *m_undoStack = nullptr;

        // UI components
        AudioFileListPanel *m_audioFileList = nullptr;  // left sidebar
        waveform::WaveformPanel *m_waveformPanel = nullptr;
        waveform::MelSpectrogramWidget *m_melSpectrogram = nullptr;
        SliceNumberLayer *m_sliceNumberLayer = nullptr;
        SlicerListPanel *m_sliceListPanel = nullptr;  // bottom panel

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

        // Slice boundary times (seconds)
        std::vector<double> m_slicePoints;
        int m_selectedBoundary = -1;

        void buildLayout();
        void connectSignals();
        void onAutoSlice();
        void onImportMarkers();
        void onSaveMarkers();
        void onExportAudio();
        void onOpenAudioFiles();
        void onOpenAudioDirectory();
        void refreshBoundaries();
        void updateSlicerListPanel();

    protected:
        void keyPressEvent(QKeyEvent *event) override;
    };

} // namespace dstools
