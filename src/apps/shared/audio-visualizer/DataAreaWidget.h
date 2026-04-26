#pragma once

#include <QWidget>

namespace dstools {

    class DataAreaWidget : public QWidget {
        Q_OBJECT

    public:
        explicit DataAreaWidget(QWidget *parent = nullptr);
    };

} // namespace dstools