#pragma once

#include <dsfw/widgets/WidgetsGlobal.h>
#include <dsfw/widgets/ViewportController.h>
#include <QWidget>

class QPainter;

namespace dsfw::widgets {

class DSFW_WIDGETS_API TimeRulerWidget : public QWidget {
    Q_OBJECT

public:
    explicit TimeRulerWidget(ViewportController *viewport, QWidget *parent = nullptr);

    void setViewport(const ViewportState &state);

signals:
    void entryScrollRequested(int delta);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    [[nodiscard]] int timeToX(double time) const;
    [[nodiscard]] static double smoothStep(double edge0, double edge1, double x);
    [[nodiscard]] static double spacingVisibility(double spacing, double minimumSpacing);

    struct TickLevel {
        double interval;
        int height;
        bool showLabel;
    };

    ViewportController *m_viewport = nullptr;
    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_pixelsPerSecond = 200.0;

    static constexpr double kMinimumSpacing = 24.0;
    static constexpr double kFadeInSpacing = 48.0;
};

} // namespace dsfw::widgets
