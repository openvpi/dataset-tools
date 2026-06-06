#include "MoeModel.h"

#include <array>
#include <iostream>
#include <dsfw/infer/OnnxEnv.h>

namespace Moe
{
    MoeModel::MoeModel(const std::filesystem::path &modelPath, dstools::infer::ExecutionProvider provider, int deviceId)
        : CancellableOnnxModel(provider, deviceId) {
#if defined(_M_IX86) || defined(__i386__)
        m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
#endif
        auto loadResult = loadSession(modelPath);
        if (!loadResult) {
            std::cerr << "[MOE] Failed to create session: " << loadResult.error() << std::endl;
            m_initFailed = true;
        }
    }

    MoeModel::~MoeModel() = default;

    void MoeModel::terminate() {
        CancellableOnnxModel::terminate();
    }

    dstools::Result<std::vector<float>> MoeModel::predict(const float *waveform, int64_t numSamples) {
        if (!m_session) {
            return dstools::Result<std::vector<float>>::Error("Session is not initialized.");
        }

        try {
            const std::array<int64_t, 2> inputShape = {1, numSamples};
            std::vector<float> waveformCopy(waveform, waveform + numSamples);
            Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
                m_memoryInfo, waveformCopy.data(), waveformCopy.size(),
                inputShape.data(), inputShape.size());

            const char *inputNames[] = {"waveform"};
            const char *outputNames[] = {"curve"};

            Ort::RunOptions runOptions;
            {
                std::lock_guard lock(m_runMutex);
                m_activeRunOptions = &runOptions;
            }

            auto outputTensors = m_session->Run(runOptions, inputNames, &inputTensor, 1,
                                                outputNames, 1);

            {
                std::lock_guard lock(m_runMutex);
                m_activeRunOptions = nullptr;
            }

            const float *curveData = outputTensors.front().GetTensorMutableData<float>();
            auto elemCount = outputTensors.front().GetTensorTypeAndShapeInfo().GetElementCount();

            std::vector<float> curve(curveData, curveData + elemCount);
            return dstools::Result<std::vector<float>>::Ok(std::move(curve));

        } catch (const Ort::Exception &e) {
            return dstools::Result<std::vector<float>>::Error(
                std::string("Error during MOE model inference: ") + e.what());
        }
    }

} // namespace Moe