#include "DataAreaWidget.h"

#include <QVBoxLayout>

namespace dstools {

    DataAreaWidget::DataAreaWidget(QWidget *parent) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    }

} // namespace dstools