#include "FeatureQueue.h"

#include <ComDefine.h>

namespace FunAsr {
    FeatureQueue::FeatureQueue()
        : buff(std::make_unique<Tensor<float>>(67, 80)), window_size(67), buff_idx(0) {}

    FeatureQueue::~FeatureQueue() = default;

    void FeatureQueue::reinit(const int &size) {
        buff = std::make_unique<Tensor<float>>(size, 80);
        buff_idx = 0;
        window_size = size;
    }

    void FeatureQueue::reset() {
        buff_idx = 0;
    }

    void FeatureQueue::push(const float *din, const int &flag) {
        if (buff_idx >= window_size) return;
        const int offset = buff_idx * 80;
        memcpy(buff->buff + offset, din, 80 * sizeof(float));
        buff_idx++;

        if (flag == S_END) {
            auto tmp = std::make_unique<Tensor<float>>(buff_idx, 80);
            memcpy(tmp->buff, buff->buff, buff_idx * 80 * sizeof(float));
            feature_queue.push(std::move(tmp));
            buff_idx = 0;
        } else if (buff_idx == window_size) {
            feature_queue.push(std::move(buff));
            auto tmp = std::make_unique<Tensor<float>>(window_size, 80);
            memcpy(tmp->buff, feature_queue.back()->buff + (window_size - 3) * 80, 3 * 80 * sizeof(float));
            buff_idx = 3;
            buff = std::move(tmp);
        }
    }

    std::unique_ptr<Tensor<float>> FeatureQueue::pop() {
        auto tmp = std::move(feature_queue.front());
        feature_queue.pop();
        return tmp;
    }

    int FeatureQueue::size() const {
        return static_cast<int>(feature_queue.size());
    }
}
