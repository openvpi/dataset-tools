#include <dsfw/AudioPlaybackManager.h>
#include <dsfw/Log.h>

#include <QMetaMethod>
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
        if (auto *player = m_currentPlayer.data()) {
            const auto mo = player->metaObject();
            const int idx = mo->indexOfMethod("setPlaying(bool)");
            if (idx >= 0) {
                mo->method(idx).invoke(player, Qt::AutoConnection, Q_ARG(bool, false));
            }
        }
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
        if (auto *player = m_currentPlayer.data()) {
            const auto mo = player->metaObject();
            const int idx = mo->indexOfMethod("setPlaying(bool)");
            if (idx >= 0) {
                mo->method(idx).invoke(player, Qt::AutoConnection, Q_ARG(bool, false));
            }
        }
        m_currentPlayer = nullptr;
    }
}

} // namespace dsfw
