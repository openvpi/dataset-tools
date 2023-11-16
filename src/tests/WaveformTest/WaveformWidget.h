//
// Created by fluty on 2023/11/3.
//

#ifndef DATASET_TOOLS_WAVEFORMWIDGET_H
#define DATASET_TOOLS_WAVEFORMWIDGET_H

#include <QFrame>
#include <QThread>
#include <QWheelEvent>
#include <sndfile.hh>

class WaveformWidget : public QFrame {
    Q_OBJECT

public:
    enum RenderMode {
        Peak,
        Line,
        Sample
    };

    explicit WaveformWidget(QWidget *parent = nullptr);
    ~WaveformWidget() override;

    bool openFile(const QString &path);
    void zoomIn(double factor = 1.2);
    void zoomOut(double factor = 1.2);

    RenderMode renderMode();
    void setRenderMode(RenderMode mode);

protected:
    SndfileHandle sf;
    QVector<std::tuple<double, double>> m_peakCache;
//    int m_sceneWidth = 0;
    double m_renderStart = 0;
    double m_renderEnd = 0;
    double m_scale = 1.0;
//    int m_zoomInLevel = 0;
    QString m_clipName;
    QPoint m_mouseLastPos;
    int m_rectLastWidth = -1;
    RenderMode m_renderMode = Peak;

    bool eventFilter(QObject *object, QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:

};

#endif // DATASET_TOOLS_WAVEFORMWIDGET_H
