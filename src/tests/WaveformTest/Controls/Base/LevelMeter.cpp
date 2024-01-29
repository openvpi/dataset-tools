//
// Created by fluty on 2023/8/6.
//

#include "LevelMeter.h"
#include <QDebug>

LevelMeter::LevelMeter(Qt::Orientation orientation, ChannelType channelType, QWidget *parent) : QWidget(parent) {
    m_orientation = orientation;
    m_channelType = channelType;

    auto buildChannelMeter = [](LevelMeterChunk *chunk, QPushButton *button,
                                Qt::Orientation orientation) -> QBoxLayout * {
        if (orientation == Qt::Horizontal) {
            chunk->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            chunk->setOrientation(Qt::Horizontal);
            button->setMaximumWidth(10);
            button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

            auto layout = new QHBoxLayout;
            layout->setSpacing(1);
            layout->setMargin(0);
            layout->addWidget(chunk);
            layout->addWidget(button);

            return layout;
        } else {
            chunk->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            chunk->setOrientation(Qt::Vertical);
            button->setMaximumHeight(10);
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

            auto layout = new QVBoxLayout;
            layout->setSpacing(1);
            layout->setMargin(0);
            layout->addWidget(button);
            layout->addWidget(chunk);

            return layout;
        }
    };

    m_chunk1 = new LevelMeterChunk;
    m_button1 = new QPushButton;
    m_button1->setObjectName("button1");
    m_button1->setCheckable(true);
    m_button1->setChecked(false);

    if (m_orientation == Qt::Horizontal) {
        //        m_chunk1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        //        m_chunk1->setOrientation(Qt::Horizontal);
        //        m_button1->setMaximumWidth(10);
        //        m_button1->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        //
        //        m_channelLayout1 = new QHBoxLayout;
        //        m_channelLayout1->setSpacing(1);
        //        m_channelLayout1->setMargin(0);
        //        m_channelLayout1->setObjectName("hLayout");
        //        m_channelLayout1->addWidget(m_chunk1);
        //        m_channelLayout1->addWidget(m_button1);

        m_mainLayout = new QVBoxLayout;

        m_channelLayout1 = buildChannelMeter(m_chunk1, m_button1, Qt::Horizontal);
        m_mainLayout->addLayout(m_channelLayout1);
        if (m_channelType == Stereo) {
            m_chunk2 = new LevelMeterChunk;
            m_button2 = new QPushButton;
            m_button2->setObjectName("button2");
            m_button2->setCheckable(true);
            m_button2->setChecked(false);

            m_channelLayout2 = buildChannelMeter(m_chunk2, m_button2, Qt::Horizontal);
            m_mainLayout->addLayout(m_channelLayout2);
        }
        m_mainLayout->setSpacing(1);
        m_mainLayout->setMargin(0);

        this->setMinimumHeight(24);
        this->setMaximumHeight(24);
        this->setLayout(m_mainLayout);
    } else {
        //        m_chunk1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        //        m_chunk1->setOrientation(Qt::Vertical);
        //        m_button1->setMaximumHeight(10);
        //        m_button1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        //
        //        m_channelLayout1 = new QVBoxLayout;
        //        m_channelLayout1->setSpacing(1);
        //        m_channelLayout1->setMargin(0);
        //        m_channelLayout1->setObjectName("vLayout");
        //        m_channelLayout1->addWidget(m_button1);
        //        m_channelLayout1->addWidget(m_chunk1);

        m_mainLayout = new QHBoxLayout;

        m_channelLayout1 = buildChannelMeter(m_chunk1, m_button1, Qt::Vertical);
        m_mainLayout->addLayout(m_channelLayout1);
        if (m_channelType == Stereo) {
            m_chunk2 = new LevelMeterChunk;
            m_button2 = new QPushButton;
            m_button2->setObjectName("button2");
            m_button2->setCheckable(true);
            m_button2->setChecked(false);

            m_channelLayout2 = buildChannelMeter(m_chunk2, m_button2, Qt::Vertical);
            m_mainLayout->addLayout(m_channelLayout2);
        }
        m_mainLayout->setSpacing(1);
        m_mainLayout->setMargin(0);

        this->setMinimumWidth(6);
        this->setMaximumWidth(24);
        this->setLayout(m_mainLayout);
    }

    QObject::connect(m_button1, &QPushButton::clicked, this, [=]() { this->m_button1->setChecked(false); });

    this->setStyleSheet(R"(
LevelMeter {
    border-style: none;
    border-radius: 4px;
}

LevelMeter > QProgressBar#bar {
    background-color:#d9d9d9;
    color:#808080;
    border-style: none;
    border-radius: 0px;
    text-align:center;
}

LevelMeter > QProgressBar#bar::chunk {
    background-color: #0060C0;
    border-radius: 0px;
}

LevelMeter > QPushButton {
    background-color: #d9d9d9;
    border-radius: 0px;
}

LevelMeter > QPushButton:checked {
    background-color: #ff7c80;
}
)");
}

void LevelMeter::paintEvent(QPaintEvent *event) {
    return QWidget::paintEvent(event);
}

void LevelMeter::readSample(double sample) {
    m_chunk1->readSample(sample);
    if (sample > 1)
        m_button1->setChecked(true);
}

void LevelMeter::initBuffer(int bufferSize) {
    m_chunk1->initBuffer(bufferSize);
    if (m_chunk2 != nullptr)
        m_chunk2->initBuffer(bufferSize);
}

void LevelMeter::setClippedIndicator(bool on) {
    m_button1->setChecked(on);
}

void LevelMeter::setValue(double value) {
    m_chunk1->setValue(value);
}

void LevelMeter::readSample(double sampleL, double sampleR) {
    m_chunk1->readSample(sampleL);
    if (sampleL > 1)
        m_button1->setChecked(true);
    m_chunk2->readSample(sampleR);
    if (sampleR > 1)
        m_button2->setChecked(true);
}

void LevelMeter::setValue(double valueL, double valueR) {
    m_chunk1->setValue(valueL);
    m_chunk2->setValue(valueR);
}

void LevelMeter::setClippedIndicator(bool onL, bool onR) {
    m_button1->setChecked(onL);
    m_button2->setChecked(onR);
}

int LevelMeter::bufferSize() const {
    return m_chunk1->bufferSize();
}

void LevelMeter::setBufferSize(int size) {
    m_chunk1->setBufferSize(size);
    if (m_chunk2 != nullptr)
        m_chunk2->setBufferSize(size);
}

bool LevelMeter::freeze() const {
    return m_chunk1->freeze();
}

void LevelMeter::setFreeze(bool on) {
    m_chunk1->setFreeze(on);
    if (m_chunk1 != nullptr)
        m_chunk2->setFreeze(on);
}
