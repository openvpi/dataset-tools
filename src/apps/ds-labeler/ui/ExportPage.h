/// @file ExportPage.h
/// @brief DsLabeler export page for generating CSV/DS/WAV output.

#pragma once

#include <QCheckBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableView>
#include <QWidget>
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <memory>

namespace dstools {

    class ProjectDataSource;
    class PhNumCalculator;
    class SlicePreviewModel;
    class EnginePool;
    struct SlicePreviewRow;

    /// @brief Export page for DsLabeler — generates transcriptions.csv, ds/, wavs/.
    ///
    /// Validates data completeness, auto-completes missing steps, and exports
    /// the dataset in MakeDiffSinger-compatible format.
    class ExportPage : public QWidget, public labeler::IPageActions, public labeler::IPageLifecycle {
        Q_OBJECT
        Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

    public:
        explicit ExportPage(QWidget *parent = nullptr);
        ~ExportPage() override;

        void setDataSource(ProjectDataSource *source);
        void setEnginePool(std::unique_ptr<EnginePool> pool);

        QMenuBar *createMenuBar(QWidget *parent) override;
        QString windowTitle() const override;

        void onActivated() override;
        void onDeactivated() override;

        bool eventFilter(QObject *obj, QEvent *event) override;

    private:
        ProjectDataSource *m_source = nullptr;

        std::unique_ptr<EnginePool> m_enginePool;
        std::unique_ptr<PhNumCalculator> m_phNumCalc;

        // UI
        QTabWidget *m_tabWidget = nullptr;
        QLineEdit *m_outputDir = nullptr;
        QPushButton *m_btnBrowse = nullptr;
        QCheckBox *m_chkCsv = nullptr;
        QCheckBox *m_chkDs = nullptr;
        QCheckBox *m_chkWavs = nullptr;
        QSpinBox *m_sampleRate = nullptr;
        QCheckBox *m_chkResample = nullptr;
        QSpinBox *m_hopSize = nullptr;
        QCheckBox *m_chkIncludeDiscarded = nullptr;
        QCheckBox *m_chkAutoComplete = nullptr;
        QPushButton *m_btnExport = nullptr;
        QProgressBar *m_progressBar = nullptr;
        QLabel *m_statusLabel = nullptr;

        // Column coverage
        QWidget *m_coverageWidget = nullptr;
        struct ColumnCoverage {
            QString name;
            QProgressBar *bar = nullptr;
            QLabel *pctLabel = nullptr;
            int count = 0;
        };
        std::vector<ColumnCoverage> m_columnCoverages;
        QLabel *m_summaryLabel = nullptr;
        int m_totalSlices = 0;
        int m_readySlices = 0;
        int m_missingLayerSlices = 0;
        int m_dirtySlices = 0;

        // Preview
        QTableView *m_previewTable = nullptr;
        QStandardItemModel *m_previewModel = nullptr;
        QSortFilterProxyModel *m_sortProxy = nullptr;
        QLineEdit *m_filterEdit = nullptr;
        QPushButton *m_clearFilterBtn = nullptr;
        QLabel *m_loadingLabel = nullptr;
        bool m_previewLoading = false;
        std::unique_ptr<SlicePreviewModel> m_previewData;

        // Validation
        QLabel *m_validationLabel = nullptr;
        int m_readyForCsv = 0;  ///< Slices with grapheme layer
        int m_readyForDs = 0;   ///< Slices with pitch_review in editedSteps
        int m_dirtyCount = 0;   ///< Slices with dirty layers
        int m_missingFa = 0;    ///< Slices missing phoneme layer
        int m_missingPitch = 0; ///< Slices missing pitch curve
        int m_missingMidi = 0;  ///< Slices missing midi layer
        int m_missingPhNum = 0; ///< Slices missing ph_num layer

        void buildUi();
        void buildPreviewTab();
        void buildScanProgressTab();
        void refreshPreview();
        void onBrowseOutput();
        void onExport();
        void continueExport(const QStringList &sliceIds, const QString &outputDir);
        void performExport(const QStringList &sliceIds, const QString &outputDir, const QString &wavsDir,
                           const QString &dsDir);
        void updateExportButton();
        void runValidation();

        void populatePreviewModel(const std::vector<SlicePreviewRow> &rows);

        void toggleColumnFilter(int column);
        void applyCoverageFilter(const QString &columnName);
        void clearFilters();

        void ensureEngines();
        void autoCompleteSlice(const QString &sliceId);
    };

} // namespace dstools
