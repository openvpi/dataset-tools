#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <filesystem>

#include <dstools/ExecutionProvider.h>

namespace FunAsr {
    using dstools::infer::ExecutionProvider;

    class Model {
    public:
        virtual ~Model() = default;
        virtual void reset() = 0;
        virtual std::string forward(float *din, int len, int flag) = 0;
    };

    Model *create_model(const std::filesystem::path &path, const int &nThread = 0,
                         ExecutionProvider provider = ExecutionProvider::CPU, int deviceId = 0);
}
#endif
