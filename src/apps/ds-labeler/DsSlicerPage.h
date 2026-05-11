/// @file DsSlicerPage.h
/// @brief DsLabeler slicer page — extends SlicerPage with project integration.

#pragma once

#include <SlicerPage.h>

#include <QWidget>

namespace dstools {

    class ProjectDataSource;

    class DsSlicerPage : public SlicerPage {
        Q_OBJECT

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

    protected:
        void onAutoSlice() override;
        void onImportMarkers() override;
        void onExportAudio() override;
        void onBatchExportAll() override;
        void saveCurrentSlicePoints() override;

    private:
        ProjectDataSource *m_dataSource = nullptr;

        void connectProjectSignals();
        void onExportMenu();
        void promptSliceUpdateIfNeeded();
        void saveSlicerParamsToProject();
        void saveSlicerStateToProject();
    };

} // namespace dstools