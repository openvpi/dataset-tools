/// @file DsSlicerPage.h
/// @brief DsLabeler slicer page placeholder — full implementation in M.5.

#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

#include <QWidget>

namespace dstools {

class ProjectDataSource;

/// @brief Slicer page for DsLabeler.
///
/// Provides auto-slice, manual slice editing, and slice export.
/// Currently a placeholder; full implementation in M.5.
class DsSlicerPage : public QWidget,
                     public labeler::IPageActions,
                     public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit DsSlicerPage(QWidget *parent = nullptr);
    ~DsSlicerPage() override = default;

    void setDataSource(ProjectDataSource *source);

    // IPageActions
    QMenuBar *createMenuBar(QWidget *parent) override;
    QString windowTitle() const override;

    // IPageLifecycle
    void onActivated() override;

private:
    ProjectDataSource *m_dataSource = nullptr;
};

} // namespace dstools
