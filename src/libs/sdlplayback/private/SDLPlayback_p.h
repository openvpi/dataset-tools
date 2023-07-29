#ifndef SDLPLAYBACKPRIVATE_H
#define SDLPLAYBACKPRIVATE_H

#include "Api/private/IAudioPlayback_p.h"

#include "../SDLPlayback.h"

#include <mutex>
#include <thread>

extern "C" {
#include <SDL2/SDL.h>
}

class SDLPlaybackPrivate : public IAudioPlaybackPrivate {
    Q_DECLARE_PUBLIC(SDLPlayback)
public:
    SDLPlaybackPrivate();
    ~SDLPlaybackPrivate();

    void init();

    bool setup(const QVariantMap &args) override;
    void dispose() override;

    void play() override;
    void stop() override;

    void switchState(IAudioPlayback::PlaybackState newState);

    bool switchDevId(const QString &dev);
    bool switchDriver(const QString &drv);

    void notifyGetAudioFrame();
    void notifyStop();
    void notifyPlay();
    void notifyQuitPoll();

    void workCallback(quint8 *stream, int len);

    void poll();

    enum SDL_UserEvent {
        SDL_EVENT_BUFFER_END = SDL_USEREVENT + 1,
        SDL_EVENT_MANUAL_STOP,
        SDL_EVENT_MANUAL_PLAY,
        SDL_EVENT_QUIT_POLL,
    };

    // 控制参数
    quint32 curDevId; // 当前播放设备

    SDL_AudioSpec spec;

    std::thread *producer;

    std::mutex mutex;

    struct CallbackBlock {
        char *audio_chunk;
        int audio_len;
        int audio_pos;
    };

    CallbackBlock scb;

    QScopedArrayPointer<float> pcm_buffer;
    int pcm_buffer_size;

    QString driver;
    QString device;
};

#endif // SDLPLAYBACKPRIVATE_H
