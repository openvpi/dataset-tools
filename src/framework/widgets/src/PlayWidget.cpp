#include <dsfw/widgets/PlayWidget.h>
#include <dstools/IAudioPlayer.h>
#include <dstools/AudioPlayer.h>
#include <dsfw/ServiceLocator.h>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QTime>
#include <QTimerEvent>
#include <QVBoxLayout>

namespace dsfw::widgets {

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
    m_playBtn->setIcon(QIcon(":/icons/play.svg"));
    m_stopBtn = new QPushButton();
    m_stopBtn->setObjectName("stop-button");
    m_stopBtn->setIcon(QIcon(":/icons/stop.svg"));
    m_devBtn = new QPushButton();
    m_devBtn->setObjectName("dev-button");
    m_devBtn->setIcon(QIcon(":/icons/audio.svg"));

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
        connect(m_player, &dstools::audio::IAudioPlayer::stateChanged,
                this, &PlayWidget::onPlaybackStateChanged);
        connect(m_player, &dstools::audio::IAudioPlayer::deviceChanged,
                this, &PlayWidget::onDeviceChanged);
        reloadDevices();
    } else {
        m_playBtn->setEnabled(false);
        m_stopBtn->setEnabled(false);
        m_devBtn->setEnabled(false);
        m_slider->setEnabled(false);
    }
}

PlayWidget::PlayWidget(dstools::audio::IAudioPlayer *player, QWidget *parent) : QWidget(parent) {
    m_player = player;
    m_valid = (m_player != nullptr);

    m_deviceMenu = new QMenu(this);
    m_deviceActionGroup = new QActionGroup(this);
    m_deviceActionGroup->setExclusive(true);

    m_fileLabel = new QLabel(tr("Select audio file."));
    m_timeLabel = new QLabel("--:--/--:--");

    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setRange(0, 10000);

    m_playBtn = new QPushButton();
    m_playBtn->setObjectName("play-button");
    m_playBtn->setIcon(QIcon(":/icons/play.svg"));
    m_stopBtn = new QPushButton();
    m_stopBtn->setObjectName("stop-button");
    m_stopBtn->setIcon(QIcon(":/icons/stop.svg"));
    m_devBtn = new QPushButton();
    m_devBtn->setObjectName("dev-button");
    m_devBtn->setIcon(QIcon(":/icons/audio.svg"));

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
        connect(m_player, &dstools::audio::IAudioPlayer::stateChanged,
                this, &PlayWidget::onPlaybackStateChanged);
        connect(m_player, &dstools::audio::IAudioPlayer::deviceChanged,
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
    if (auto *shared = dstools::ServiceLocator::get<dstools::audio::IAudioPlayer>()) {
        m_player = shared;
        m_valid = true;
        return;
    }

    m_ownedPlayer = std::make_unique<dstools::audio::AudioPlayer>();
    m_player = m_ownedPlayer.get();

    if (!m_player->isOpen() && !m_player->setup(44100, 2, 1024)) {
        QMessageBox::warning(this, qApp->applicationName(),
            tr("Failed to initialize audio playback. Audio features will be disabled."));
        m_ownedPlayer.reset();
        m_player = nullptr;
        m_valid = false;
        return;
    }
    m_valid = true;

    dstools::ServiceLocator::set<dstools::audio::IAudioPlayer>(m_player);
}

void PlayWidget::uninitAudio() {
    if (m_player) {
        m_player->stop();
    }
    if (m_ownedPlayer) {
        if (dstools::ServiceLocator::get<dstools::audio::IAudioPlayer>() == m_player) {
            dstools::ServiceLocator::reset<dstools::audio::IAudioPlayer>();
        }
        m_ownedPlayer.reset();
    }
    m_player = nullptr;
}

void PlayWidget::openFile(const QString &path) {
    if (!m_valid || !m_player) return;
    if (path.isEmpty()) return;
    m_player->stop();
    m_player->close();
    m_filename = path;
    if (m_player->open(path)) {
        m_fileLabel->setText(QFileInfo(path).fileName());
        reloadSliderStatus();
    } else {
        qWarning() << "PlayWidget: Failed to open audio file:" << path;
    }
}

void PlayWidget::closeFile() {
    if (!m_valid) return;
    m_player->stop();
    m_player->close();
    m_filename.clear();
    m_fileLabel->setText(tr("Select audio file."));
}

bool PlayWidget::isPlaying() const {
    if (!m_valid || !m_player) return false;
    return m_player->isPlaying();
}

void PlayWidget::setPlaying(bool playing) {
    if (!m_valid) return;
    if (playing && !isPlaying()) {
        m_player->play();
        m_lastObtainedTimeMs = 0;
        m_lastObtainedTimePoint = std::chrono::steady_clock::now();
        if (m_notifyTimerId == 0)
            m_notifyTimerId = startTimer(16);
    } else if (!playing && isPlaying()) {
        m_player->stop();
    }
    reloadButtonStatus();
}

void PlayWidget::seek(double sec) {
    if (!m_valid || !m_player || !m_player->isOpen()) return;
    m_player->setPosition(sec);
    reloadFinePlayheadStatus(static_cast<uint64_t>(sec * 1000));
    reloadSliderStatus();
}

double PlayWidget::duration() const {
    if (!m_valid || !m_player || !m_player->isOpen()) return 0.0;
    return m_player->duration();
}

void PlayWidget::setPlayRange(double startSec, double endSec) {
    m_rangeStart = startSec;
    m_rangeEnd = endSec;
    m_hasRange = true;
    if (m_valid && m_player && m_player->isOpen()) {
        m_player->setPosition(startSec);
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
        if (!m_valid || !m_player) return;
        if (isPlaying()) {
            reloadSliderStatus();
            double posSec = static_cast<double>(estimatedTimeMs()) / 1000.0;
            if (m_hasRange) {
                emit playheadChanged(posSec - m_rangeStart);
                if (posSec >= m_rangeEnd) {
                    m_player->stop();
                    reloadButtonStatus();
                }
            } else {
                if (m_player->isOpen()) {
                    double decoderSec = m_player->position();
                    emit playheadChanged(decoderSec);
                }
            }
        } else {
            if (m_notifyTimerId) {
                killTimer(m_notifyTimerId);
                m_notifyTimerId = 0;
            }
            if (m_player && m_player->isOpen()) {
                double pos = m_player->position();
                double dur = m_player->duration();
                if (dur > 0 && pos >= dur) {
                    if (m_hasRange) {
                        m_player->setPosition(m_rangeStart);
                    } else {
                        m_player->setPosition(0);
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
    if (!m_player) return;
    m_deviceMenu->clear();
    for (auto a : m_deviceActionGroup->actions()) {
        m_deviceActionGroup->removeAction(a);
        delete a;
    }
    auto devs = m_player->devices();
    for (const auto &dev : devs) {
        auto *action = m_deviceMenu->addAction(dev);
        action->setCheckable(true);
        m_deviceActionGroup->addAction(action);
    }
    reloadDeviceActionStatus();
}

void PlayWidget::reloadButtonStatus() {
    bool playing = isPlaying();
    m_playBtn->setIcon(QIcon(playing ? ":/icons/pause.svg" : ":/icons/play.svg"));
}

void PlayWidget::reloadSliderStatus() {
    if (!m_player || !m_player->isOpen()) return;
    double dur = m_player->duration();
    double pos = m_player->position();
    if (dur > 0) {
        m_slider->setValue(static_cast<int>(pos / dur * 10000));
    }
    int curMs = static_cast<int>(pos * 1000);
    int totMs = static_cast<int>(dur * 1000);
    m_timeLabel->setText(QString("%1:%2/%3:%4")
        .arg(curMs / 60000, 2, 10, QChar('0'))
        .arg((curMs / 1000) % 60, 2, 10, QChar('0'))
        .arg(totMs / 60000, 2, 10, QChar('0'))
        .arg((totMs / 1000) % 60, 2, 10, QChar('0')));
}

void PlayWidget::reloadDeviceActionStatus() {
    if (!m_player) return;
    QString cur = m_player->currentDevice();
    for (auto *a : m_deviceActionGroup->actions()) {
        a->setChecked(a->text() == cur);
    }
}

void PlayWidget::reloadFinePlayheadStatus(uint64_t timeMs) {
    if (timeMs == UINT64_MAX) {
        if (!m_player || !m_player->isOpen()) return;
        timeMs = static_cast<uint64_t>(m_player->position() * 1000);
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
    m_player->stop();
    if (m_player->isOpen()) {
        if (m_hasRange) {
            m_player->setPosition(m_rangeStart);
        } else {
            m_player->setPosition(0);
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
    if (!m_valid || !m_player || !m_player->isOpen()) return;
    double dur = m_player->duration();
    if (dur <= 0) return;
    double sec = static_cast<double>(m_slider->value()) / 10000.0 * dur;
    m_player->setPosition(sec);
    reloadFinePlayheadStatus();
}

void PlayWidget::onDeviceAction(QAction *action) {
    if (!m_player) return;
    m_player->setDevice(action->text());
}

void PlayWidget::onPlaybackStateChanged() {
    reloadButtonStatus();
}

void PlayWidget::onDeviceChanged() {
    reloadDeviceActionStatus();
}

} // namespace dsfw::widgets
