#include "FeatureQueue.h"

#include <ComDefine.h>

namespace FunAsr {
    FeatureQueue::FeatureQueue() {
        buff = new Tensor<float>(67, 80);
        window_size = 67;
        buff_idx = 0;
    }

    FeatureQueue::~FeatureQueue() {
        delete buff;
    }

    void FeatureQueue::reinit(const int &size) {
        delete buff;
        buff = new Tensor<float>(size, 80);
        buff_idx = 0;
        window_size = size;
    }

    void FeatureQueue::reset() {
        buff_idx = 0;
    }

    void FeatureQueue::push(const float *din, const int &flag) {
        const int offset = buff_idx * 80;
        memcpy(buff->buff + offset, din, 80 * sizeof(float));
        buff_idx++;

        if (flag == S_END) {
            auto *tmp = new Tensor<float>(buff_idx, 80);
            memcpy(tmp->buff, buff->buff, buff_idx * 80 * sizeof(float));
            feature_queue.push(tmp);
            buff_idx = 0;
        } else if (buff_idx == window_size) {
            feature_queue.push(buff);
            auto *tmp = new Tensor<float>(window_size, 80);
            memcpy(tmp->buff, buff->buff + (window_size - 3) * 80, 3 * 80 * sizeof(float));
            buff_idx = 3;
            buff = tmp;
        }
    }

    Tensor<float> *FeatureQueue::pop() {
        Tensor<float> *tmp = feature_queue.front();
        feature_queue.pop();
        return tmp;
    }

    int FeatureQueue::size() const {
        return static_cast<int>(feature_queue.size());
    }
}