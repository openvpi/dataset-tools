//
// Created by fluty on 2023/8/14.
//

#ifndef DATASET_TOOLS_PROGRESSINDICATOR_H
#define DATASET_TOOLS_PROGRESSINDICATOR_H

#include "WidgetsGlobal/QMWidgetsGlobal.h"
#include <QPropertyAnimation>
#include <QWidget>

class QMWIDGETS_EXPORT ProgressIndicator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double secondaryValue READ secondaryValue WRITE setSecondaryValue NOTIFY secondaryValueChanged)
    Q_PROPERTY(double currentTaskValue READ currentTaskValue WRITE setValue NOTIFY currentTaskValueChanged)
    //    Q_PROPERTY(bool invertedAppearance READ invertedAppearance WRITE setInvertedAppearance)
    Q_PROPERTY(int thumbProgress READ thumbProgress WRITE setThumbProgress)
    Q_PROPERTY(double apparentValue READ apparentValue WRITE setApparentValue)
    Q_PROPERTY(double apparentSecondaryValue READ apparentSecondaryValue WRITE setApparentSecondaryValue)
    Q_PROPERTY(double apparentCurrentTaskValue READ apparentCurrentTaskValue WRITE setApparentCurrentTaskValue)
    Q_PROPERTY(bool indeterminate READ indeterminate WRITE setIndeterminate)

public:
    enum IndicatorStyle {
        HorizontalBar,
        //        VerticalBar,
        Ring
    };

    enum TaskStatus { Normal, Warning, Error };

    explicit ProgressIndicator(QWidget *parent = nullptr);
    explicit ProgressIndicator(IndicatorStyle indicatorStyle, QWidget *parent = nullptr);
    ~ProgressIndicator();

    double minimum() const;
    double maximum() const;
    double value() const;
    double secondaryValue() const;
    double currentTaskValue() const;
    bool indeterminate() const;
    TaskStatus taskStatus() const;
    //
    //    QSize sizeHint() const override;
    //    QSize minimumSizeHint() const override;
    //
    //    Qt::Orientation orientation() const;
    //
    //    void setInvertedAppearance(bool invert);
    //    bool invertedAppearance() const;
    //
    //    void setFormat(const QString &format);
    //    void resetFormat();
    //    QString format() const;

public slots:
    void reset();
    void setRange(double minimum, double maximum);
    void setMinimum(double minimum);
    void setMaximum(double maximum);
    void setValue(double value);
    void setSecondaryValue(double value);
    void setCurrentTaskValue(double value);
    void setIndeterminate(bool on);
    void setTaskStatus(TaskStatus status);

signals:
    void valueChanged(int value);
    void secondaryValueChanged(int value);
    void currentTaskValueChanged(int value);

protected:
    struct ColorPalette {
        QColor inactive;
        QColor total;
        QColor secondary;
        QColor currentTask;
    };

    ColorPalette colorPaletteNormal = {
        QColor(217, 217, 217),
        QColor(112, 156, 255),
        QColor(159, 189, 255),
        QColor(113, 218, 255)
    };

    ColorPalette colorPaletteWarning = {
        QColor(217, 217, 217),
        QColor(255, 175, 95),
        QColor(255, 204, 153),
        QColor(255, 218, 113)
    };

    ColorPalette colorPaletteError = {
        QColor(217, 217, 217),
        QColor(255, 124, 128),
        QColor(255, 171, 173),
        QColor(255, 171, 221)
    };

    ColorPalette m_colorPalette;

    IndicatorStyle m_indicatorStyle = HorizontalBar;
    TaskStatus m_taskStatus = Normal;
    double m_max = 100;
    double m_min = 0;
    double m_value = 0;
    double m_secondaryValue = 0;
    double m_currentTaskValue = 0;
    bool m_indeterminate = false;
    int m_thumbProgress = 0;
    int m_penWidth;
    int m_padding;
    int m_halfRectHeight;
    QPoint m_trackStart;
    QPoint m_trackEnd;
    int m_actualLength;
    QRect m_ringRect;
    //    bool m_invertedAppearance = false;
    QTimer *m_timer;
    QPropertyAnimation *m_valueAnimation;
    QPropertyAnimation *m_SecondaryValueAnimation;
    QPropertyAnimation *m_currentTaskValueAnimation;

    void initUi(QWidget *parent);
    void calculateBarParams();
    void calculateRingParams();
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    double m_apparentValue = 0;
    double m_apparentSecondaryValue = 0;
    double m_apparentCurrentTaskValue = 0;

    int thumbProgress() const;
    void setThumbProgress(int x);
    double apparentValue() const;
    void setApparentValue(double x);
    double apparentSecondaryValue() const;
    void setApparentSecondaryValue(double x);
    double apparentCurrentTaskValue() const;
    void setApparentCurrentTaskValue(double x);
};



#endif // DATASET_TOOLS_PROGRESSINDICATOR_H
