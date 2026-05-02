/// @file ExportPage.h
/// @brief DsLabeler export page for generating CSV/DS/WAV output.

#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

#include <QCheckBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

namespace dstools {

class ProjectDataSource;

/// @brief Export page for DsLabeler — generates transcriptions.csv, ds/, wavs/.
///
/// Validates data completeness, auto-completes missing steps, and exports
/// the dataset in MakeDiffSinger-compatible format.
class ExportPage : public QWidget,
                   public labeler::IPageActions,
                   public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit ExportPage(QWidget *parent = nullptr);
    ~ExportPage() override = default;

    void setDataSource(ProjectDataSource *source);

    QMenuBar *createMenuBar(QWidget *parent) override;
    QString windowTitle() const override;

    void onActivated() override;

private:
    ProjectDataSource *m_source = nullptr;

    // UI
    QLineEdit *m_outputDir = nullptr;
    QPushButton *m_btnBrowse = nullptr;
    QCheckBox *m_chkCsv = nullptr;
    QCheckBox *m_chkDs = nullptr;
    QCheckBox *m_chkWavs = nullptr;
    QSpinBox *m_sampleRate = nullptr;
    QCheckBox *m_chkResample = nullptr;
    QSpinBox *m_hopSize = nullptr;
    QCheckBox *m_chkIncludeDiscarded = nullptr;
    QPushButton *m_btnExport = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_statusLabel = nullptr;

    void buildUi();
    void onBrowseOutput();
    void onExport();
    void updateExportButton();
};

} // namespace dstools
