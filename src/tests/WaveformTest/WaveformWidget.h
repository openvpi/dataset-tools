//
// Created by fluty on 2023/11/3.
//

#ifndef DATASET_TOOLS_WAVEFORMWIDGET_H
#define DATASET_TOOLS_WAVEFORMWIDGET_H

#include <QFrame>
#include <QThread>

class WaveformWidget : public QFrame {
    Q_OBJECT

public:
    explicit WaveformWidget(QWidget *parent = nullptr);
    ~WaveformWidget() override;

    bool openFile(const QString &path);

protected:
    QVector<std::tuple<double, double>> m_peakCache;

    //    bool eventFilter(QObject *object, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};

#endif // DATASET_TOOLS_WAVEFORMWIDGET_H
