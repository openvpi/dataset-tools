/// @file DsMinLabelPage.h
/// @brief DsLabeler MinLabel page with project data source, ASR, and batch processing.

#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

#include <QWidget>

#include <MinLabelEditor.h>

#include <memory>

namespace dstools {

class ProjectDataSource;
class SliceListPanel;

class DsMinLabelPage : public QWidget,
                       public labeler::IPageActions,
                       public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit DsMinLabelPage(QWidget *parent = nullptr);
    ~DsMinLabelPage() override;

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
    struct AsrEngine;
    std::unique_ptr<AsrEngine> m_asrEngine;

    Minlabel::MinLabelEditor *m_editor = nullptr;
    SliceListPanel *m_sliceList = nullptr;
    ProjectDataSource *m_source = nullptr;
    QString m_currentSliceId;
    bool m_dirty = false;
    bool m_asrRunning = false;

    void onSliceSelected(const QString &sliceId);
    bool saveCurrentSlice();
    bool maybeSave();

    void onRunAsr();
    void onBatchAsr();
    void runAsrForSlice(const QString &sliceId);
    void ensureAsrEngine();
    void setAsrResult(const QString &sliceId, const QString &text);
};

} // namespace dstools
