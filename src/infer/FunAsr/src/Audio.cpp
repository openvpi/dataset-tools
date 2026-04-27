#include "Audio.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "ComDefine.h"

namespace FunAsr {
    AudioFrame::AudioFrame(const int &len) : len(len) {
        start = 0;
    };
    AudioFrame::~AudioFrame() = default;

    int AudioFrame::get_start() const {
        return start;
    };

    int AudioFrame::get_len() const {
        return len;
    };

    Audio::Audio(const int &data_type) : data_type(data_type) {
        speech_buff = nullptr;
        speech_data = nullptr;
        align_size = 1360;
    }

    Audio::Audio(const int &data_type, const int &size) : data_type(data_type) {
        speech_buff = nullptr;
        speech_data = nullptr;
        align_size = static_cast<float>(size);
    }

    Audio::~Audio() {
        if (speech_buff != nullptr) {
            free(speech_buff);
        }

        if (speech_data != nullptr) {
            free(speech_data);
        }
    }

    bool Audio::loadPcmFloat(const float *buf, const int num_samples) {
        if (speech_buff != nullptr) {
            free(speech_buff);
            speech_buff = nullptr;
        }
        if (speech_data != nullptr) {
            free(speech_data);
            speech_data = nullptr;
        }
        offset = 0;

        speech_len = num_samples;
        speech_align_len = static_cast<int>(ceil(static_cast<float>(speech_len) / align_size) * align_size);

        speech_data = static_cast<float *>(malloc(sizeof(float) * speech_align_len));
        if (speech_data) {
            memset(speech_data, 0, sizeof(float) * speech_align_len);
            memcpy(speech_data, buf, sizeof(float) * speech_len);

            auto *frame = new AudioFrame(speech_len);
            frame_queue.push(frame);
            return true;
        }
        return false;
    }

    bool Audio::loadwav(const char *buf, const int &nFileLen) {
#define WAV_HEADER_SIZE 44
        if (speech_data != nullptr) {
            free(speech_data);
        }
        if (speech_buff != nullptr) {
            free(speech_buff);
        }
        offset = 0;

        speech_len = (nFileLen - WAV_HEADER_SIZE) / 2;
        speech_align_len = static_cast<int>(ceil(static_cast<float>(speech_len) / align_size) * align_size);
        speech_buff = static_cast<int16_t *>(malloc(sizeof(int16_t) * speech_align_len));
        if (speech_buff) {
            memset(speech_buff, 0, sizeof(int16_t) * speech_align_len);
            memcpy(speech_buff, buf + WAV_HEADER_SIZE, speech_len * sizeof(int16_t));

            speech_data = static_cast<float *>(malloc(sizeof(float) * speech_align_len));
            memset(speech_data, 0, sizeof(float) * speech_align_len);
            float scale = 1;

            if (data_type == 1) {
                scale = 32768;
            }

            for (int i = 0; i < speech_len; i++) {
                speech_data[i] = static_cast<float>(speech_buff[i]) / scale;
            }

            auto *frame = new AudioFrame(speech_len);
            frame_queue.push(frame);
            return true;
        }
        return false;
    }

    int Audio::fetch(float *&dout, int &len, int &flag) {
        if (!frame_queue.empty()) {
            const AudioFrame *frame = frame_queue.front();
            frame_queue.pop();

            dout = speech_data + frame->get_start();
            len = frame->get_len();
            delete frame;
            flag = S_END;
            return 1;
        }
        return 0;
    }
}