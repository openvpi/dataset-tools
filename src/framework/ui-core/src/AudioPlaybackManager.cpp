#include <dsfw/AudioPlaybackManager.h>
#include <dsfw/Log.h>

#include <QMetaObject>
#include <QWidget>

namespace dsfw {

AudioPlaybackManager::AudioPlaybackManager(QObject *parent)
    : QObject(parent) {}

AudioPlaybackManager::~AudioPlaybackManager() {
    stopAll();
}

void AudioPlaybackManager::requestPlay(QWidget *playWidget) {
    if (!playWidget)
        return;

    if (m_currentPlayer && m_currentPlayer != playWidget) {
        DSFW_LOG_DEBUG("audio", "AudioPlaybackManager: stopping previous player");
        QMetaObject::invokeMethod(m_currentPlayer, "setPlaying",
                                  Qt::DirectConnection, Q_ARG(bool, false));
    }

    m_currentPlayer = playWidget;
    DSFW_LOG_DEBUG("audio", "AudioPlaybackManager: play granted");
}

void AudioPlaybackManager::releasePlay(QWidget *playWidget) {
    if (m_currentPlayer == playWidget)
        m_currentPlayer = nullptr;
}

void AudioPlaybackManager::stopAll() {
    if (m_currentPlayer) {
        DSFW_LOG_DEBUG("audio", "AudioPlaybackManager: stopAll");
        QMetaObject::invokeMethod(m_currentPlayer, "setPlaying",
                                  Qt::DirectConnection, Q_ARG(bool, false));
        m_currentPlayer = nullptr;
    }
}

} // namespace dsfw
