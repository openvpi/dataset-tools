#include "Audio.h"

#include <cmath>
#include <cstdint>
#include <cstring>

#include "ComDefine.h"

namespace FunAsr {
    AudioFrame::AudioFrame(const int &len) : start(0), len(len) {}

    AudioFrame::~AudioFrame() = default;

    int AudioFrame::get_start() const {
        return start;
    }

    int AudioFrame::get_len() const {
        return len;
    }

    Audio::Audio(const int &data_type)
        : speech_len(0), speech_align_len(0), offset(0), align_size(1360), data_type(data_type) {}

    Audio::Audio(const int &data_type, const int &size)
        : speech_len(0), speech_align_len(0), offset(0), align_size(static_cast<float>(size)), data_type(data_type) {}

    Audio::~Audio() = default;

    bool Audio::loadPcmFloat(const float *buf, const int num_samples) {
        speech_buff.clear();
        speech_data.clear();
        offset = 0;

        speech_len = num_samples;
        speech_align_len = static_cast<int>(ceil(static_cast<float>(speech_len) / align_size) * align_size);

        speech_data.resize(speech_align_len, 0.0f);
        memcpy(speech_data.data(), buf, sizeof(float) * speech_len);

        frame_queue.push(std::make_unique<AudioFrame>(speech_len));
        return true;
    }

    bool Audio::loadwav(const char *buf, const int &nFileLen) {
#define WAV_HEADER_SIZE 44
        speech_data.clear();
        speech_buff.clear();
        offset = 0;

        if (nFileLen <= WAV_HEADER_SIZE) {
            return false;
        }

        speech_len = (nFileLen - WAV_HEADER_SIZE) / 2;
        speech_align_len = static_cast<int>(ceil(static_cast<float>(speech_len) / align_size) * align_size);

        speech_buff.resize(speech_align_len, 0);
        memcpy(speech_buff.data(), buf + WAV_HEADER_SIZE, speech_len * sizeof(int16_t));

        speech_data.resize(speech_align_len, 0.0f);
        float scale = 1;

        if (data_type == 1) {
            scale = 32768;
        }

        for (int i = 0; i < speech_len; i++) {
            speech_data[i] = static_cast<float>(speech_buff[i]) / scale;
        }

        frame_queue.push(std::make_unique<AudioFrame>(speech_len));
        return true;
    }

    int Audio::fetch(float *&dout, int &len, int &flag) {
        if (!frame_queue.empty()) {
            const auto &frame = frame_queue.front();

            dout = speech_data.data() + frame->get_start();
            len = frame->get_len();
            frame_queue.pop();
            flag = S_END;
            return 1;
        }
        return 0;
    }
}
