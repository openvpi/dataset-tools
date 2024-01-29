//
// Created by fluty on 2023/8/6.
//

#ifndef CHORUSKIT_LEVELMETER_H
#define CHORUSKIT_LEVELMETER_H

#include "LevelMeterChunk.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QSizePolicy>

class LevelMeterPrivate;

class LevelMeter : public QWidget {
    Q_OBJECT
public:
    enum ChannelType { Mono, Stereo };

    explicit LevelMeter(Qt::Orientation orientation, ChannelType channelType = Stereo, QWidget *parent = nullptr);


    void readSample(double sample);
    void readSample(double sampleL, double sampleR);
    void setValue(double value);
    void setValue(double valueL, double valueR);
    void setClippedIndicator(bool on);
    void setClippedIndicator(bool onL, bool onR);
    int bufferSize() const;
    void setBufferSize(int size);
    void initBuffer(int bufferSize);
    bool freeze() const;
    void setFreeze(bool on);

protected:
    void paintEvent(QPaintEvent *event) override;
    Qt::Orientation m_orientation;
    ChannelType m_channelType;

    // Channel left or mono
    LevelMeterChunk *m_chunk1;
    QPushButton *m_button1;
    QBoxLayout *m_channelLayout1;

    // Channel right
    LevelMeterChunk *m_chunk2;
    QPushButton *m_button2;
    QBoxLayout *m_channelLayout2;

    QBoxLayout *m_mainLayout;
};



#endif // CHORUSKIT_LEVELMETER_H
