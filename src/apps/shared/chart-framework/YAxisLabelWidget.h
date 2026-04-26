#pragma once

#include <QWidget>
#include <functional>

class QPainter;

namespace dstools {

    class YAxisLabelWidget : public QWidget {
        Q_OBJECT

    public:
        using PaintFunc = std::function<void(QPainter &, const QRect &)>;

        explicit YAxisLabelWidget(int yOffset, int height, PaintFunc paintFn, QWidget *parent = nullptr);

        void setChartGeometry(int yOffset, int height);
        void setLabelWidth(int width);

    protected:
        void paintEvent(QPaintEvent *event) override;

    private:
        PaintFunc m_paintFn;
    };

} // namespace dstools