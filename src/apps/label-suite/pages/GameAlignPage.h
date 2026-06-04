#pragma once

#include "WizardPageBase.h"

#include <dsfw/widgets/PathSelector.h>

class QComboBox;

namespace dstools::labeler {

    class GameAlignPage : public WizardPageBase {
        Q_OBJECT

    public:
        explicit GameAlignPage(QWidget *parent = nullptr);

    private:
        void buildUi();

        dsfw::widgets::PathSelector *m_modelPath = nullptr;
        QComboBox *m_gpuSelector = nullptr;
    };

} // namespace dstools::labeler
