#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include <dstools/ExecutionProvider.h>
#include <dstools/Result.h>

namespace dstools::infer {

    class IInferenceEngine {
    public:
        virtual ~IInferenceEngine() = default;

        virtual bool isOpen() const = 0;

        virtual void terminate() {}

        virtual const char *engineName() const = 0;

        virtual Result<void> load(const std::filesystem::path &modelPath,
                                  ExecutionProvider provider, int deviceId) = 0;

        virtual void unload() {}

        virtual int64_t estimatedMemoryBytes() const { return 0; }
    };

} // namespace dstools::infer
