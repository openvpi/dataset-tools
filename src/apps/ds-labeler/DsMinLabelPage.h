/// @file DsMinLabelPage.h
/// @brief DsLabeler MinLabel page with project data source, ASR, and batch processing.

#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

#include <QWidget>

#include <MinLabelEditor.h>

namespace dstools {

class ProjectDataSource;
class SliceListPanel;

/// @brief DsLabeler MinLabel page — project-backed lyric/G2P labeling.
///
/// Composes MinLabelEditor with ProjectDataSource. Adds ASR and batch
/// processing capabilities not present in the standalone MinLabel.
class DsMinLabelPage : public QWidget,
                       public labeler::IPageActions,
                       public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit DsMinLabelPage(QWidget *parent = nullptr);
    ~DsMinLabelPage() override = default;

    /// Set the project data source (called when project is loaded).
    void setDataSource(ProjectDataSource *source);

    // IPageActions
    QMenuBar *createMenuBar(QWidget *parent) override;
    QString windowTitle() const override;
    bool hasUnsavedChanges() const override;

    // IPageLifecycle
    void onActivated() override;
    bool onDeactivating() override;

signals:
    void sliceChanged(const QString &sliceId);

private:
    Minlabel::MinLabelEditor *m_editor = nullptr;
    SliceListPanel *m_sliceList = nullptr;
    ProjectDataSource *m_source = nullptr;
    QString m_currentSliceId;
    bool m_dirty = false;

    void onSliceSelected(const QString &sliceId);
    bool saveCurrentSlice();
    bool maybeSave();

    // ASR
    void onRunAsr();
    void onBatchAsr();
};

} // namespace dstools
