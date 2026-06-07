#ifndef MODEL_H
#define MODEL_H

#include "dsfw/infer/ExecutionProvider.h"
#include "dsfw/infer/InferenceModelProvider.h"

#include <string>
#include <filesystem>
#include <memory>

namespace FunAsr {

class Model {
public:
    virtual ~Model() = default;
    virtual void reset() = 0;
    virtual std::string forward(float* din, int len, int flag) = 0;
};

std::unique_ptr<Model> create_model(const std::filesystem::path& path,
                                    const int& nThread = 0,
                                    dsfw::infer::ExecutionProvider provider = dsfw::infer::ExecutionProvider::CPU,
                                    int deviceId = 0);
}  // namespace FunAsr
#endif
