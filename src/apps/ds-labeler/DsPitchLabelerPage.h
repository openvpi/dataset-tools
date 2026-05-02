/// @file DsPitchLabelerPage.h
/// @brief DsLabeler PitchLabeler page with auto F0/MIDI, batch processing, and preload.

#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

#include <QWidget>

#include <PitchEditor.h>

#include <memory>

namespace dstools {

class ProjectDataSource;
class SliceListPanel;
namespace pitchlabeler { class DSFile; }

/// @brief DsLabeler PitchLabeler page — project-backed F0/MIDI editing.
///
/// Composes PitchEditor with ProjectDataSource. Adds automatic add_ph_num,
/// F0 extraction, and MIDI transcription when opening slices.
class DsPitchLabelerPage : public QWidget,
                           public labeler::IPageActions,
                           public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit DsPitchLabelerPage(QWidget *parent = nullptr);
    ~DsPitchLabelerPage() override = default;

    void setDataSource(ProjectDataSource *source);

    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;
    QString windowTitle() const override;
    bool hasUnsavedChanges() const override;

    void onActivated() override;
    bool onDeactivating() override;

signals:
    void sliceChanged(const QString &sliceId);

private:
    pitchlabeler::PitchEditor *m_editor = nullptr;
    SliceListPanel *m_sliceList = nullptr;
    ProjectDataSource *m_source = nullptr;
    QString m_currentSliceId;
    std::shared_ptr<pitchlabeler::DSFile> m_currentFile;

    void onSliceSelected(const QString &sliceId);
    bool saveCurrentSlice();
    bool maybeSave();

    void onExtractPitch();
    void onExtractMidi();
    void onBatchExtract();
};

} // namespace dstools
