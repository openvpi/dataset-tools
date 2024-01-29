//
// Created by fluty on 2023/8/6.
//

#ifndef CHORUSKIT_LEVELMETERCHUNK_H
#define CHORUSKIT_LEVELMETERCHUNK_H

#include <QWidget>

class LevelMeterChunkPrivate;

class LevelMeterChunk : public QWidget {
    Q_OBJECT

public:
    explicit LevelMeterChunk(QWidget *parent = nullptr);
    ~LevelMeterChunk();

    void readSample(double sample);
    void setValue(double value);
    void setOrientation(Qt::Orientation orientation);
    //    void setSampleRate(int sampleRate);
    int bufferSize() const;
    void setBufferSize(int size);
    void initBuffer(int bufferSize);
    bool freeze() const;
    void setFreeze(bool on);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void calculateParams();
    QRect m_rect;
    int m_rectLeft;
    int m_rectTop;
    int m_rectWidth;
    int m_rectHeight;
    int m_levelLength;
    int m_safeLength;
    int m_warnLength;
    int m_criticalLength;
    int m_safeStart;
    int m_warnStart;
    int m_criticalStart;
    QRect m_safeChunk;
    QRect m_warnChunk;
    QRect m_criticalChunk;

    Qt::Orientation m_orientation;
    QColor m_colorBackground = QColor(217, 217, 217);
    QColor m_colorSafe = QColor(112, 156, 255);
    QColor m_colorWarn = QColor(255, 175, 95);
    QColor m_colorCritical = QColor(255, 124, 128);
    const double m_safeThreshold = 0.707946; //-3 dB
    const double m_warnThreshold = 0.891251; //-1 dB
    double averageLevel = 0;
    //    int sampleRate = 44100;
    double *m_bufferPtr;
    int m_bufferPos = 0;
    int m_bufferSize = 8;
    QTimer *m_timer;
    bool m_freeze = false;

private:
    LevelMeterChunkPrivate *d;
    void resetBuffer();
};



#endif // CHORUSKIT_LEVELMETERCHUNK_H
