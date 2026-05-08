#pragma once

#include <QPointer>
#include <QObject>

class QWidget;

namespace dsfw {

class AudioPlaybackManager : public QObject {
    Q_OBJECT
public:
    explicit AudioPlaybackManager(QObject *parent = nullptr);
    ~AudioPlaybackManager() override;

    void requestPlay(QWidget *playWidget);
    void releasePlay(QWidget *playWidget);
    void stopAll();

    QWidget *currentPlayer() const { return m_currentPlayer; }

private:
    QPointer<QWidget> m_currentPlayer;
};

} // namespace dsfw
