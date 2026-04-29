#ifndef FEATUREQUEUE_H
#define FEATUREQUEUE_H

#include "Tensor.h"
#include <memory>
#include <queue>

namespace FunAsr {
    class FeatureQueue {
    private:
        std::queue<std::unique_ptr<Tensor<float>>> feature_queue;
        std::unique_ptr<Tensor<float>> buff;
        int buff_idx;
        int window_size;

    public:
        FeatureQueue();
        ~FeatureQueue();
        void reinit(const int &size);
        void reset();
        void push(const float *din, const int &flag);
        std::unique_ptr<Tensor<float>> pop();
        int size() const;
    };
}
#endif
