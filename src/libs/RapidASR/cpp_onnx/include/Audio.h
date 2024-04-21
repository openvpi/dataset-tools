#ifndef AUDIO_H
#define AUDIO_H

#include <cstdint>
#include <queue>

#include "ComDefine.h"

using namespace std;

class AudioFrame {
public:
    explicit AudioFrame(const int &len);
    ~AudioFrame();
    int get_start() const;
    int get_len() const;

private:
    int start;
    int len;
};

class Audio {
public:
    explicit Audio(const int &data_type);
    Audio(const int &data_type, const int &size);
    ~Audio();
    bool loadwav(const char *buf, const int &nFileLen);
    int fetch(float *&dout, int &len, int &flag);
    float get_time_len() const;
    int get_queue_size() const;

private:
    float *speech_data;
    int16_t *speech_buff;
    int speech_len;
    int speech_align_len;
    int16_t sample_rate;
    int offset;
    float align_size;
    int data_type;
    queue<AudioFrame *> frame_queue;
};

#endif
