#pragma once

#include "WizardPageBase.h"

#include <QCheckBox>
#include <dsfw/widgets/PathSelector.h>

namespace dstools::labeler {

    class BuildCsvPage : public WizardPageBase {
        Q_OBJECT

    public:
        explicit BuildCsvPage(QWidget *parent = nullptr);

    private:
        void buildUi();

        dsfw::widgets::PathSelector *m_dictPath = nullptr;
        QCheckBox *m_chkPhNum = nullptr;
    };

} // namespace dstools::labeler
