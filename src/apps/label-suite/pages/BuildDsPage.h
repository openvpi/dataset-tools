/// @file BuildDsPage.h
/// @brief DiffSinger .ds file build page for the labeler wizard.

#pragma once

#include "WizardPageBase.h"

#include <QComboBox>
#include <QSpinBox>
#include <dsfw/widgets/PathSelector.h>

namespace dstools::labeler {

    /// @brief IPageActions page that builds .ds files with RMVPE pitch extraction.
    class BuildDsPage : public WizardPageBase {
        Q_OBJECT

    public:
        /// @brief Constructs the .ds build page.
        /// @param parent Optional parent widget.
        explicit BuildDsPage(QWidget *parent = nullptr);

    private:
        void buildUi(); ///< Builds the page UI.

        dsfw::widgets::PathSelector *m_rmvpePath = nullptr;     ///< RMVPE model path selector.
        QComboBox *m_gpuSelector = nullptr;                        ///< GPU device selector.
        QSpinBox *m_hopSize = nullptr;                             ///< Hop size for pitch extraction.
        QSpinBox *m_sampleRate = nullptr;                          ///< Sample rate setting.
    };

} // namespace dstools::labeler
