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

#include <memory>

namespace HFA {
class HFA;
}

namespace Rmvpe {
class Rmvpe;
}

namespace Game {
class Game;
}

namespace dstools {

class ProjectDataSource;
class PhNumCalculator;

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
    ~ExportPage() override;

    void setDataSource(ProjectDataSource *source);

    QMenuBar *createMenuBar(QWidget *parent) override;
    QString windowTitle() const override;

    void onActivated() override;
    void onDeactivated() override;

private:
    ProjectDataSource *m_source = nullptr;

    // Inference engines (shared with other pages, lazy-loaded)
    std::unique_ptr<HFA::HFA> m_hfa;
    std::unique_ptr<Rmvpe::Rmvpe> m_rmvpe;
    std::unique_ptr<Game::Game> m_game;
    std::unique_ptr<PhNumCalculator> m_phNumCalc;

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
    QCheckBox *m_chkAutoComplete = nullptr;
    QPushButton *m_btnExport = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_statusLabel = nullptr;

    // Validation
    QLabel *m_validationLabel = nullptr;
    int m_readyForCsv = 0;   ///< Slices with grapheme layer
    int m_readyForDs = 0;    ///< Slices with pitch_review in editedSteps
    int m_dirtyCount = 0;    ///< Slices with dirty layers
    int m_totalSlices = 0;
    int m_missingFa = 0;     ///< Slices missing phoneme layer
    int m_missingPitch = 0;  ///< Slices missing pitch curve
    int m_missingMidi = 0;   ///< Slices missing midi layer
    int m_missingPhNum = 0;  ///< Slices missing ph_num layer

    void buildUi();
    void onBrowseOutput();
    void onExport();
    void updateExportButton();
    void runValidation();

    void ensureEngines();
    void autoCompleteSlice(const QString &sliceId);
};

} // namespace dstools
