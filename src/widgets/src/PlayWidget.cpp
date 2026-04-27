#include <dstools/PlayWidget.h>
#include <dstools/AudioDecoder.h>
#include <dstools/AudioPlayback.h>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QStyle>
#include <QTime>
#include <QTimerEvent>
#include <QVBoxLayout>

namespace dstools::widgets {

PlayWidget::PlayWidget(QWidget *parent) : QWidget(parent) {
    initAudio();

    m_deviceMenu = new QMenu(this);
    m_deviceActionGroup = new QActionGroup(this);
    m_deviceActionGroup->setExclusive(true);

    m_fileLabel = new QLabel(tr("Select audio file."));
    m_timeLabel = new QLabel("--:--/--:--");

    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setRange(0, 10000);

    m_playBtn = new QPushButton();
    m_playBtn->setObjectName("play-button");
    m_stopBtn = new QPushButton();
    m_stopBtn->setObjectName("stop-button");
    m_devBtn = new QPushButton();
    m_devBtn->setObjectName("dev-button");

    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(m_playBtn);
    buttonsLayout->addWidget(m_stopBtn);
    buttonsLayout->addWidget(m_devBtn);
    buttonsLayout->addWidget(m_timeLabel);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_fileLabel);
    mainLayout->addWidget(m_slider);
    mainLayout->addLayout(buttonsLayout);

    if (m_valid) {
        connect(m_playBtn, &QPushButton::clicked, this, &PlayWidget::onPlayClicked);
        connect(m_stopBtn, &QPushButton::clicked, this, &PlayWidget::onStopClicked);
        connect(m_devBtn, &QPushButton::clicked, this, &PlayWidget::onDevClicked);
        connect(m_slider, &QSlider::sliderReleased, this, &PlayWidget::onSliderReleased);
        connect(m_deviceActionGroup, &QActionGroup::triggered, this, &PlayWidget::onDeviceAction);
        connect(m_playback, &dstools::audio::AudioPlayback::stateChanged,
                this, &PlayWidget::onPlaybackStateChanged);
        connect(m_playback, &dstools::audio::AudioPlayback::deviceChanged,
                this, &PlayWidget::onDeviceChanged);
        reloadDevices();
    } else {
        m_playBtn->setEnabled(false);
        m_stopBtn->setEnabled(false);
        m_devBtn->setEnabled(false);
        m_slider->setEnabled(false);
    }
}

PlayWidget::~PlayWidget() {
    uninitAudio();
}

void PlayWidget::initAudio() {
    m_decoder = new dstools::audio::AudioDecoder();
    m_playback = new dstools::audio::AudioPlayback(this);

    if (!m_playback->setup(44100, 2, 1024)) {
        QMessageBox::warning(this, qApp->applicationName(),
            tr("Failed to initialize audio playback. Audio features will be disabled."));
        delete m_playback; m_playback = nullptr;
        delete m_decoder; m_decoder = nullptr;
        m_valid = false;
        return;
    }
    m_playback->setDecoder(m_decoder);
    m_valid = true;
}

void PlayWidget::uninitAudio() {
    if (m_playback) {
        m_playback->stop();
        m_playback->dispose();
    }
    delete m_playback;
    m_playback = nullptr;
    delete m_decoder;
    m_decoder = nullptr;
}

void PlayWidget::openFile(const QString &path) {
    if (!m_valid) return;
    m_playback->stop();
    m_decoder->close();
    m_filename = path;
    if (m_decoder->open(path)) {
        m_fileLabel->setText(QFileInfo(path).fileName());
        reloadSliderStatus();
    }
}

void PlayWidget::closeFile() {
    if (!m_valid) return;
    m_playback->stop();
    m_decoder->close();
    m_filename.clear();
    m_fileLabel->setText(tr("Select audio file."));
}

bool PlayWidget::isPlaying() const {
    if (!m_valid || !m_playback) return false;
    return m_playback->state() == dstools::audio::AudioPlayback::Playing;
}

void PlayWidget::setPlaying(bool playing) {
    if (!m_valid) return;
    if (playing && !isPlaying()) {
        m_playback->play();
        m_lastObtainedTimeMs = 0;
        m_lastObtainedTimePoint = std::chrono::steady_clock::now();
        if (m_notifyTimerId == 0)
            m_notifyTimerId = startTimer(16); // ~60fps
    } else if (!playing && isPlaying()) {
        m_playback->stop();
    }
    reloadButtonStatus();
}

void PlayWidget::setPlayRange(double startSec, double endSec) {
    m_rangeStart = startSec;
    m_rangeEnd = endSec;
    m_hasRange = true;
    if (m_valid && m_decoder && m_decoder->isOpen()) {
        auto fmt = m_decoder->format();
        qint64 pos = static_cast<qint64>(startSec * fmt.averageBytesPerSecond());
        m_decoder->setPosition(pos);
    }
}

void PlayWidget::clearPlayRange() {
    m_hasRange = false;
    m_rangeStart = 0.0;
    m_rangeEnd = 0.0;
}

uint64_t PlayWidget::estimatedTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastObtainedTimePoint).count();
    return m_lastObtainedTimeMs + elapsed;
}

void PlayWidget::timerEvent(QTimerEvent *event) {
    if (event->timerId() == m_notifyTimerId) {
        if (!m_valid || !m_playback) return;
        if (isPlaying()) {
            reloadSliderStatus();
            if (m_hasRange) {
                double posSec = static_cast<double>(estimatedTimeMs()) / 1000.0;
                emit playheadChanged(posSec - m_rangeStart);
                if (posSec >= m_rangeEnd) {
                    m_playback->stop();
                    reloadButtonStatus();
                }
            }
        } else {
            if (m_notifyTimerId) {
                killTimer(m_notifyTimerId);
                m_notifyTimerId = 0;
            }
            // EOF: reset position to range start or beginning
            if (m_decoder && m_decoder->isOpen()) {
                if (m_decoder->position() >= m_decoder->length()) {
                    if (m_hasRange) {
                        auto fmt = m_decoder->format();
                        m_decoder->setPosition(
                            static_cast<qint64>(m_rangeStart * fmt.averageBytesPerSecond()));
                    } else {
                        m_decoder->setPosition(0);
                    }
                }
            }
            reloadSliderStatus();
            reloadButtonStatus();
        }
    }
    QWidget::timerEvent(event);
}

void PlayWidget::reloadDevices() {
    if (!m_playback) return;
    m_deviceMenu->clear();
    for (auto a : m_deviceActionGroup->actions()) {
        m_deviceActionGroup->removeAction(a);
        delete a;
    }
    auto devs = m_playback->devices();
    for (const auto &dev : devs) {
        auto *action = m_deviceMenu->addAction(dev);
        action->setCheckable(true);
        m_deviceActionGroup->addAction(action);
    }
    reloadDeviceActionStatus();
}

void PlayWidget::reloadButtonStatus() {
    bool playing = isPlaying();
    m_playBtn->setIcon(playing ? QIcon(":/res/pause.svg") : QIcon(":/res/play.svg"));
}

void PlayWidget::reloadSliderStatus() {
    if (!m_decoder || !m_decoder->isOpen()) return;
    qint64 len = m_decoder->length();
    qint64 pos = m_decoder->position();
    if (len > 0) {
        m_slider->setValue(static_cast<int>(pos * 10000 / len));
    }
    // Update time label
    auto fmt = m_decoder->format();
    if (fmt.averageBytesPerSecond() > 0) {
        int curMs = static_cast<int>(pos * 1000 / fmt.averageBytesPerSecond());
        int totMs = static_cast<int>(len * 1000 / fmt.averageBytesPerSecond());
        m_timeLabel->setText(QString("%1:%2/%3:%4")
            .arg(curMs / 60000, 2, 10, QChar('0'))
            .arg((curMs / 1000) % 60, 2, 10, QChar('0'))
            .arg(totMs / 60000, 2, 10, QChar('0'))
            .arg((totMs / 1000) % 60, 2, 10, QChar('0')));
    }
}

void PlayWidget::reloadDeviceActionStatus() {
    if (!m_playback) return;
    QString cur = m_playback->currentDevice();
    for (auto *a : m_deviceActionGroup->actions()) {
        a->setChecked(a->text() == cur);
    }
}

void PlayWidget::reloadFinePlayheadStatus(uint64_t timeMs) {
    // Update steady_clock tracking
    if (timeMs == UINT64_MAX) {
        if (!m_decoder || !m_decoder->isOpen()) return;
        auto fmt = m_decoder->format();
        if (fmt.averageBytesPerSecond() > 0) {
            timeMs = m_decoder->position() * 1000 / fmt.averageBytesPerSecond();
        } else {
            timeMs = 0;
        }
    }
    m_lastObtainedTimeMs = timeMs;
    m_lastObtainedTimePoint = std::chrono::steady_clock::now();
}

void PlayWidget::onPlayClicked() {
    if (!m_valid) return;
    setPlaying(!isPlaying());
}

void PlayWidget::onStopClicked() {
    if (!m_valid) return;
    m_playback->stop();
    if (m_decoder && m_decoder->isOpen()) {
        if (m_hasRange) {
            auto fmt = m_decoder->format();
            m_decoder->setPosition(static_cast<qint64>(m_rangeStart * fmt.averageBytesPerSecond()));
        } else {
            m_decoder->setPosition(0);
        }
    }
    reloadSliderStatus();
    reloadButtonStatus();
}

void PlayWidget::onDevClicked() {
    reloadDevices();
    m_deviceMenu->popup(m_devBtn->mapToGlobal(QPoint(0, m_devBtn->height())));
}

void PlayWidget::onSliderReleased() {
    if (!m_valid || !m_decoder || !m_decoder->isOpen()) return;
    qint64 len = m_decoder->length();
    qint64 pos = static_cast<qint64>(m_slider->value()) * len / 10000;
    m_decoder->setPosition(pos);
    reloadFinePlayheadStatus();
}

void PlayWidget::onDeviceAction(QAction *action) {
    if (!m_playback) return;
    m_playback->setDevice(action->text());
}

void PlayWidget::onPlaybackStateChanged() {
    reloadButtonStatus();
}

void PlayWidget::onDeviceChanged() {
    reloadDeviceActionStatus();
}

} // namespace dstools::widgets
