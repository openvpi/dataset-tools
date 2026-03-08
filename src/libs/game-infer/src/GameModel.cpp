#include <game-infer/GameModel.h>

#include <cmath>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#ifdef ONNXRUNTIME_ENABLE_DML
#include <dml_provider_factory.h>
#endif

namespace Game
{
    // 初始化DirectML执行提供者
    static inline bool initDirectML(Ort::SessionOptions &options, const int deviceIndex,
                                    std::string *errorMessage = nullptr) {
#ifdef ONNXRUNTIME_ENABLE_DML
        if (!options) {
            if (errorMessage) {
                *errorMessage = "SessionOptions must not be nullptr!";
            }
            return false;
        }

        if (deviceIndex < 0) {
            if (errorMessage) {
                *errorMessage = "GPU device index must be a non-negative integer!";
            }
            return false;
        }

        const OrtApi &ortApi = Ort::GetApi();
        const OrtDmlApi *ortDmlApi;
        const Ort::Status getApiStatus(
            (ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void **>(&ortDmlApi))));
        if (!getApiStatus.IsOK()) {
            // Failed to get DirectML API.
            if (errorMessage) {
                *errorMessage = getApiStatus.GetErrorMessage();
            }
            return false;
        }

        // Successfully get DirectML API
        options.DisableMemPattern();
        options.SetExecutionMode(ORT_SEQUENTIAL);

        const Ort::Status appendStatus(ortDmlApi->SessionOptionsAppendExecutionProvider_DML(options, deviceIndex));
        if (!appendStatus.IsOK()) {
            if (errorMessage) {
                *errorMessage = appendStatus.GetErrorMessage();
            }
            return false;
        }
        return true;
#else
        if (errorMessage) {
            *errorMessage = "The library is not built with DirectML support.";
        }
        return false;
#endif
    }

    // 初始化CUDA执行提供者
    static inline bool initCUDA(Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage = nullptr) {
#ifdef ONNXRUNTIME_ENABLE_CUDA
        if (!options) {
            if (errorMessage) {
                *errorMessage = "SessionOptions must not be nullptr!";
            }
            return false;
        }

        if (deviceIndex < 0) {
            if (errorMessage) {
                *errorMessage = "GPU device index must be a non-negative integer!";
            }
            return false;
        }

        const OrtApi &ortApi = Ort::GetApi();

        OrtCUDAProviderOptionsV2 *cudaOptionsPtr = nullptr;
        Ort::Status createStatus(ortApi.CreateCUDAProviderOptions(&cudaOptionsPtr));

        // Currently, ORT C++ API does not have a wrapper for CUDAProviderOptionsV2.
        // Let the smart pointer take ownership of cudaOptionsPtr so it will be released when it
        // goes out of scope.
        std::unique_ptr<OrtCUDAProviderOptionsV2, decltype(ortApi.ReleaseCUDAProviderOptions)> cudaOptions(
            cudaOptionsPtr, ortApi.ReleaseCUDAProviderOptions);

        if (!createStatus.IsOK()) {
            if (errorMessage) {
                *errorMessage = createStatus.GetErrorMessage();
            }
            return false;
        }

        // The following block of code sets device_id
        {
            // Device ID from int to string
            auto cudaDeviceIdStr = std::to_string(deviceIndex);
            auto cudaDeviceIdCStr = cudaDeviceIdStr.c_str();

            constexpr int CUDA_OPTIONS_SIZE = 2;
            const char *cudaOptionsKeys[CUDA_OPTIONS_SIZE] = {"device_id", "cudnn_conv_algo_search"};
            const char *cudaOptionsValues[CUDA_OPTIONS_SIZE] = {cudaDeviceIdCStr, "DEFAULT"};
            Ort::Status updateStatus(ortApi.UpdateCUDAProviderOptions(cudaOptions.get(), cudaOptionsKeys,
                                                                      cudaOptionsValues, CUDA_OPTIONS_SIZE));
            if (!updateStatus.IsOK()) {
                if (errorMessage) {
                    *errorMessage = updateStatus.GetErrorMessage();
                }
                return false;
            }
        }
        Ort::Status appendStatus(ortApi.SessionOptionsAppendExecutionProvider_CUDA_V2(options, cudaOptions.get()));
        if (!appendStatus.IsOK()) {
            if (errorMessage) {
                *errorMessage = appendStatus.GetErrorMessage();
            }
            return false;
        }
        return true;
#else
        if (errorMessage) {
            *errorMessage = "The library is not built with CUDA support.";
        }
        return false;
#endif
    }

    GameModel::GameModel(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id) :
        env(Ort::Env(ORT_LOGGING_LEVEL_WARNING, "GameModel")), sessionOptions(Ort::SessionOptions()) {
        modelDir = modelPath;

        std::ifstream configFile(modelDir / "config.json");
        if (!configFile.is_open()) {
            throw std::runtime_error("Could not open config.json: " + (modelDir / "config.json").string());
        }
        nlohmann::json config;
        configFile >> config;
        configFile.close();

        // Load parameters from config.json
        timestep = config.value("timestep", 0.01f);
        sampleRate = config.value("samplerate", 44100);
        embeddingDim = config.value("embedding_dim", 256);
        m_target_sample_rate = config.value("samplerate", 44100);

        // Set initial parameter values from config or defaults
        m_timestep = timestep;
        m_d3pm_ts = generate_d3pm_ts(0.0f, 8); // Default D3PM settings

        // Load language if available
        if (config.contains("languages")) {
            // Default to 0 (unknown/universal) if no language specified
            m_language = 0;
        }

        sessionOptions.SetInterOpNumThreads(4);
        sessionOptions.SetGraphOptimizationLevel(ORT_ENABLE_EXTENDED);

        // Choose execution provider based on the provided option
        switch (provider) {
#ifdef ONNXRUNTIME_ENABLE_DML
        case ExecutionProvider::DML:
            {
                std::string errorMessage;
                if (!initDirectML(sessionOptions, device_id, &errorMessage)) {
                    std::cout << "Failed to enable Dml: " << errorMessage << ". Falling back to CPU." << std::endl;
                } else {
                    std::cout << "Use Dml execution provider" << std::endl;
                }
                break;
            }
#endif

#ifdef ONNXRUNTIME_ENABLE_CUDA
        case ExecutionProvider::CUDA:
            {
                std::string errorMessage;
                if (!initCUDA(sessionOptions, device_id, &errorMessage)) {
                    std::cout << "Failed to enable CUDA: " << errorMessage << std::endl;
                } else {
                    std::cout << "Using CUDA execution provider" << std::endl;
                }
                break;
            }
#endif

        default:
            break;
        }

        auto loadSession = [&](const std::string &name)
        {
            const std::filesystem::path model_path = modelDir / name;
#ifdef _WIN32
            return std::make_unique<Ort::Session>(env, model_path.wstring().c_str(), sessionOptions);
#else
            return std::make_unique<Ort::Session>(env, model_path.c_str(), sessionOptions);
#endif
        };
        sessEncoder = loadSession("encoder.onnx");
        sessSegmenter = loadSession("segmenter.onnx");
        sessEstimator = loadSession("estimator.onnx");
        sessDur2bd = loadSession("bd2dur.onnx");

        // Set default values from config or keep defaults
        m_seg_threshold = config.value("seg_threshold", 0.2f);
        m_seg_radius_seconds = config.value("seg_radius_seconds", 0.02f);
        m_est_threshold = config.value("est_threshold", 0.2f);
    }

    GameModel::~GameModel() = default;

    int GameModel::get_target_sample_rate() const { return m_target_sample_rate; }

    bool GameModel::is_open() { return true; }

    void GameModel::terminate() {}

    bool GameModel::forward(const std::vector<float> &waveform_data, std::vector<bool> &boundaries,
                            std::vector<float> &durations, std::vector<float> &presence, std::vector<float> &scores,
                            std::string &msg) const {
        try {
            InferenceInput input;
            input.waveform = waveform_data;
            input.duration = static_cast<float>(waveform_data.size()) / static_cast<float>(sampleRate);
            input.known_durations = {};
            input.language = m_language;

            int T = static_cast<int>(std::ceil(input.duration / m_timestep));
            if (T <= 0)
                T = 1;
            input.maskT = std::vector<bool>(T, true);
            input.timestep = m_timestep;

            int seg_radius_frames = static_cast<int>(std::round(m_seg_radius_seconds / m_timestep));
            if (seg_radius_frames < 1)
                seg_radius_frames = 1;

            const InferenceOutput output =
                inferSlice(input, m_seg_threshold, seg_radius_frames, m_est_threshold, m_d3pm_ts);

            boundaries.clear();
            boundaries.reserve(output.boundaries.size());
            for (const uint8_t val : output.boundaries) {
                boundaries.push_back(val != 0);
            }

            durations = output.durations;
            presence = output.presence;
            scores = output.scores;

            return true;
        }
        catch (const std::exception &e) {
            msg = "Error during inference: " + std::string(e.what());
            return false;
        }
    }

    std::vector<float> GameModel::generate_d3pm_ts(const float t0, const int n_steps) {
        std::vector<float> ts;
        if (n_steps <= 0)
            return ts;
        const float step = (1.0f - t0) / static_cast<float>(n_steps);
        for (int i = 0; i < n_steps; ++i) {
            ts.push_back(t0 + i * step);
        }
        return ts;
    }

    // Parameter setter methods implementation
    void GameModel::set_seg_threshold(const float threshold) { m_seg_threshold = threshold; }

    void GameModel::set_seg_radius_seconds(const float radius) { m_seg_radius_seconds = radius; }
    void GameModel::set_seg_radius_frames(const float radiusFrames) { m_est_threshold = radiusFrames * m_timestep; }

    void GameModel::set_est_threshold(const float threshold) { m_est_threshold = threshold; }

    void GameModel::set_d3pm_ts(const std::vector<float> &ts) { m_d3pm_ts = ts; }

    void GameModel::set_language(const int language) { m_language = language; }

    std::tuple<Ort::Value, Ort::Value, Ort::Value>
    GameModel::runEncoder(const std::vector<float> &waveform, const float duration, const int language) const {
        if (waveform.empty()) {
            return std::make_tuple(Ort::Value{nullptr}, Ort::Value{nullptr}, Ort::Value{nullptr});
        }

        const std::vector<int64_t> waveformShape = {1, static_cast<int64_t>(waveform.size())};
        const std::vector<int64_t> durationShape = {1};
        const std::vector<int64_t> languageShape = {1};

        const Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        Ort::Value waveformTensor =
            Ort::Value::CreateTensor<float>(memoryInfo, const_cast<float *>(waveform.data()), waveform.size(),
                                            waveformShape.data(), waveformShape.size());

        std::vector<float> durationVec = {duration};
        Ort::Value durationTensor = Ort::Value::CreateTensor<float>(memoryInfo, durationVec.data(), 1,
                                                                    durationShape.data(), durationShape.size());

        std::vector<int64_t> languageVec = {static_cast<int64_t>(language)};
        Ort::Value languageTensor = Ort::Value::CreateTensor<int64_t>(memoryInfo, languageVec.data(), 1,
                                                                      languageShape.data(), languageShape.size());

        std::vector<const char *> inputNames;
        std::vector<Ort::Value> inputTensors;

        inputNames.push_back("waveform");
        inputTensors.push_back(std::move(waveformTensor));

        inputNames.push_back("duration");
        inputTensors.push_back(std::move(durationTensor));

        bool hasLanguage = false;
        const size_t numInputs = sessEncoder->GetInputCount();
        for (size_t i = 0; i < numInputs; ++i) {
            Ort::AllocatorWithDefaultOptions alloc;
            auto name = sessEncoder->GetInputNameAllocated(i, alloc);
            if (name && std::strcmp(name.get(), "language") == 0) {
                hasLanguage = true;
                break;
            }
        }
        if (hasLanguage) {
            inputNames.push_back("language");
            inputTensors.push_back(std::move(languageTensor));
        }

        const char *outputNames[] = {"x_seg", "x_est", "maskT"};

        auto outputTensors = sessEncoder->Run(Ort::RunOptions{nullptr}, inputNames.data(), inputTensors.data(),
                                              inputTensors.size(), outputNames, 3);

        return std::make_tuple(std::move(outputTensors[0]), std::move(outputTensors[1]), std::move(outputTensors[2]));
    }

    std::vector<uint8_t> GameModel::runDur2bd(const std::vector<float> &knownDurations,
                                              const std::vector<uint8_t> &maskT) const {
        if (knownDurations.empty() || maskT.empty()) {
            return {};
        }

        const std::array<int64_t, 2> knownDurationsShape = {1, static_cast<int64_t>(knownDurations.size())};
        const std::array<int64_t, 2> maskTShape = {1, static_cast<int64_t>(maskT.size())};

        const Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        std::vector<float> tempKnownDurations(knownDurations.begin(), knownDurations.end());
        Ort::Value knownDurationsTensor =
            Ort::Value::CreateTensor<float>(memoryInfo, tempKnownDurations.data(), knownDurations.size(),
                                            knownDurationsShape.data(), knownDurationsShape.size());

        std::vector<uint8_t> tempMaskT(maskT.begin(), maskT.end());
        Ort::Value maskTTensor = Ort::Value::CreateTensor<bool>(memoryInfo, reinterpret_cast<bool *>(tempMaskT.data()),
                                                                tempMaskT.size(), maskTShape.data(), maskTShape.size());

        const char *inputNames[] = {"known_durations", "maskT"};
        std::vector<Ort::Value> inputTensors;
        inputTensors.push_back(std::move(knownDurationsTensor));
        inputTensors.push_back(std::move(maskTTensor));

        const char *outputNames[] = {"boundaries"};

        const auto outputTensors = sessDur2bd->Run(Ort::RunOptions{nullptr}, inputNames, inputTensors.data(),
                                                   inputTensors.size(), outputNames, 1);

        const bool *boundaryData = outputTensors[0].GetTensorData<bool>();
        const size_t boundaryCount = outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount();
        std::vector<uint8_t> boundaries(boundaryCount);
        for (size_t i = 0; i < boundaryCount; ++i)
            boundaries[i] = boundaryData[i] ? 1 : 0;

        return boundaries;
    }

    std::vector<uint8_t> GameModel::runSegmenter(const Ort::Value &xSeg, const std::vector<uint8_t> &knownBoundaries,
                                                 const std::vector<uint8_t> &prevBoundaries, int language,
                                                 const Ort::Value &maskT, float threshold, int radius,
                                                 const std::vector<float> &d3pmTs) const {
        if (d3pmTs.empty())
            return prevBoundaries;

        auto maskTInfo = maskT.GetTensorTypeAndShapeInfo();
        auto maskTShape = maskTInfo.GetShape();
        if (maskTShape.size() != 2 || maskTShape[0] != 1)
            throw std::runtime_error("maskT shape unexpected");
        int64_t T = maskTShape[1];
        if (T <= 0)
            return {};

        const bool *maskTData = maskT.GetTensorData<bool>();
        std::vector<uint8_t> maskTBool(T);
        for (int64_t i = 0; i < T; ++i)
            maskTBool[i] = maskTData[i] ? 1 : 0;

        auto xSegInfo = xSeg.GetTensorTypeAndShapeInfo();
        auto xSegShape = xSegInfo.GetShape();
        std::vector<float> xSegData(xSegInfo.GetElementCount());
        std::memcpy(xSegData.data(), xSeg.GetTensorData<float>(), xSegData.size() * sizeof(float));

        Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        std::array<int64_t, 2> boundariesShape = {1, T};
        std::array<int64_t, 3> xSegShapeArr = {xSegShape[0], xSegShape[1], xSegShape[2]};

        std::vector<uint8_t> currentBoundaries = prevBoundaries;
        if (currentBoundaries.size() != static_cast<size_t>(T))
            currentBoundaries.resize(T, 0);

        std::vector<uint8_t> knownBd = knownBoundaries;
        if (knownBd.size() != static_cast<size_t>(T))
            knownBd.resize(T, 0);

        for (float t : d3pmTs) {
            Ort::Value xSegTensor = Ort::Value::CreateTensor<float>(memInfo, xSegData.data(), xSegData.size(),
                                                                    xSegShapeArr.data(), xSegShapeArr.size());

            std::vector<uint8_t> maskTBoolVec(maskTBool.begin(), maskTBool.end());
            Ort::Value maskTTensor =
                Ort::Value::CreateTensor<bool>(memInfo, reinterpret_cast<bool *>(maskTBoolVec.data()),
                                               maskTBoolVec.size(), boundariesShape.data(), boundariesShape.size());

            std::vector<uint8_t> knownBdVec(knownBd.begin(), knownBd.end());
            Ort::Value knownBdTensor =
                Ort::Value::CreateTensor<bool>(memInfo, reinterpret_cast<bool *>(knownBdVec.data()), knownBdVec.size(),
                                               boundariesShape.data(), boundariesShape.size());

            std::vector<uint8_t> currentBdVec(currentBoundaries.begin(), currentBoundaries.end());
            Ort::Value prevBdTensor =
                Ort::Value::CreateTensor<bool>(memInfo, reinterpret_cast<bool *>(currentBdVec.data()),
                                               currentBdVec.size(), boundariesShape.data(), boundariesShape.size());

            std::array<int64_t, 1> langShape = {1};
            int64_t langVal = language;
            Ort::Value langTensor =
                Ort::Value::CreateTensor<int64_t>(memInfo, &langVal, 1, langShape.data(), langShape.size());

            Ort::Value threshTensor = Ort::Value::CreateTensor<float>(memInfo, &threshold, 1, nullptr, 0);

            int64_t radius_64 = radius;
            Ort::Value radiusTensor = Ort::Value::CreateTensor<int64_t>(memInfo, &radius_64, 1, nullptr, 0);

            std::array<int64_t, 1> tShape = {1};
            float tVal = t;
            Ort::Value tTensor = Ort::Value::CreateTensor<float>(memInfo, &tVal, 1, tShape.data(), tShape.size());

            std::vector<const char *> inputNames;
            std::vector<Ort::Value> inputTensors;

            static std::vector<std::string> segInputNames;
            if (segInputNames.empty()) {
                size_t numInputs = sessSegmenter->GetInputCount();
                for (size_t i = 0; i < numInputs; ++i) {
                    Ort::AllocatorWithDefaultOptions alloc;
                    auto namePtr = sessSegmenter->GetInputNameAllocated(i, alloc);
                    segInputNames.emplace_back(namePtr.get());
                }
            }

            for (const auto &name : segInputNames) {
                if (name == "x_seg")
                    inputTensors.push_back(std::move(xSegTensor));
                else if (name == "known_boundaries")
                    inputTensors.push_back(std::move(knownBdTensor));
                else if (name == "prev_boundaries")
                    inputTensors.push_back(std::move(prevBdTensor));
                else if (name == "language")
                    inputTensors.push_back(std::move(langTensor));
                else if (name == "maskT")
                    inputTensors.push_back(std::move(maskTTensor));
                else if (name == "threshold")
                    inputTensors.push_back(std::move(threshTensor));
                else if (name == "radius")
                    inputTensors.push_back(std::move(radiusTensor));
                else if (name == "t")
                    inputTensors.push_back(std::move(tTensor));
                else
                    throw std::runtime_error("Unknown segmenter input: " + name);
                inputNames.push_back(name.c_str());
            }

            const char *outputNames[] = {"boundaries"};
            auto outputs = sessSegmenter->Run(Ort::RunOptions{nullptr}, inputNames.data(), inputTensors.data(),
                                              inputTensors.size(), outputNames, 1);

            const bool *outData = outputs[0].GetTensorData<bool>();
            size_t outCount = outputs[0].GetTensorTypeAndShapeInfo().GetElementCount();
            currentBoundaries.clear();
            currentBoundaries.reserve(outCount);
            for (size_t i = 0; i < outCount; ++i) {
                currentBoundaries.push_back(outData[i] ? 1 : 0);
            }
            if (currentBoundaries.size() != static_cast<size_t>(T))
                currentBoundaries.resize(T, 0);
        }
        return currentBoundaries;
    }

    std::vector<uint8_t> GameModel::runSegmenterWithConfig(const Ort::Value &xSeg,
                                                           const std::vector<uint8_t> &knownBoundaries,
                                                           const std::vector<uint8_t> &prevBoundaries,
                                                           const int language, const Ort::Value &maskT,
                                                           const float threshold, const int radius,
                                                           const std::vector<float> &d3pmTs) const {
        return runSegmenter(xSeg, knownBoundaries, prevBoundaries, language, maskT, threshold, radius, d3pmTs);
    }

    std::tuple<std::vector<float>, std::vector<uint8_t>> GameModel::runBd2dur(const std::vector<uint8_t> &boundaries,
                                                                              const std::vector<uint8_t> &maskT) const {
        if (boundaries.empty() || maskT.empty()) {
            return {{}, {}};
        }

        const std::array<int64_t, 2> boundariesShape = {1, static_cast<int64_t>(boundaries.size())};
        const std::array<int64_t, 2> maskTShape = {1, static_cast<int64_t>(maskT.size())};

        const Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        std::vector<uint8_t> tempBoundaries(boundaries.begin(), boundaries.end());
        Ort::Value boundariesTensor =
            Ort::Value::CreateTensor<bool>(memoryInfo, reinterpret_cast<bool *>(tempBoundaries.data()),
                                           boundaries.size(), boundariesShape.data(), boundariesShape.size());

        std::vector<uint8_t> tempMaskT(maskT.begin(), maskT.end());
        Ort::Value maskTTensor = Ort::Value::CreateTensor<bool>(memoryInfo, reinterpret_cast<bool *>(tempMaskT.data()),
                                                                tempMaskT.size(), maskTShape.data(), maskTShape.size());

        const char *inputNames[] = {"boundaries", "maskT"};
        std::vector<Ort::Value> inputTensors;
        inputTensors.push_back(std::move(boundariesTensor));
        inputTensors.push_back(std::move(maskTTensor));

        const char *outputNames[] = {"durations", "maskN"};

        const auto outputTensors = sessDur2bd->Run(Ort::RunOptions{nullptr}, inputNames, inputTensors.data(),
                                                   inputTensors.size(), outputNames, 2);

        const float *durData = outputTensors[0].GetTensorData<float>();
        const size_t durCount = outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount();
        std::vector<float> durations(durData, durData + durCount);

        const bool *maskNData = outputTensors[1].GetTensorData<bool>();
        const size_t maskNCount = outputTensors[1].GetTensorTypeAndShapeInfo().GetElementCount();
        std::vector<uint8_t> maskN(maskNCount);
        for (size_t i = 0; i < maskNCount; ++i)
            maskN[i] = maskNData[i] ? 1 : 0;

        return std::make_tuple(durations, maskN);
    }

    std::tuple<std::vector<float>, std::vector<float>>
    GameModel::runEstimator(const Ort::Value &xEst, const std::vector<uint8_t> &boundaries, const Ort::Value &maskT,
                            const std::vector<uint8_t> &maskN, float threshold) const {
        if (boundaries.empty() || maskN.empty()) {
            return {{}, {}};
        }

        if (!maskT) {
            throw std::runtime_error("runEstimator: maskT is null");
        }
        auto maskTTypeInfo = maskT.GetTensorTypeAndShapeInfo();
        auto maskTShape = maskTTypeInfo.GetShape();
        if (maskTShape.size() != 2 || maskTShape[0] != 1) {
            throw std::runtime_error("runEstimator: maskT shape unexpected");
        }
        int64_t T = maskTShape[1];
        if (T <= 0) {
            return {{}, {}};
        }

        const bool *originalMaskTData = nullptr;
        try {
            originalMaskTData = maskT.GetTensorData<bool>();
        }
        catch (const Ort::Exception &e) {
            throw std::runtime_error("Failed to get tensor data from maskT in estimator: " + std::string(e.what()));
        }

        std::vector<uint8_t> boundariesAdjusted = boundaries;
        if (boundariesAdjusted.size() != static_cast<size_t>(T)) {
            boundariesAdjusted.resize(T, 0);
        }

        if (!xEst) {
            throw std::runtime_error("runEstimator: xEst is null");
        }
        auto xEstTypeInfo = xEst.GetTensorTypeAndShapeInfo();
        auto xEstShape = xEstTypeInfo.GetShape();
        if (xEstShape.size() < 2 || xEstShape[1] != T) {
            throw std::runtime_error("runEstimator: xEst shape mismatch with T");
        }
        auto xEstData = xEst.GetTensorData<float>();
        std::vector<float> xEstCopy(xEstData, xEstData + xEstTypeInfo.GetElementCount());

        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        Ort::Value xEstTensor = Ort::Value::CreateTensor<float>(memoryInfo, xEstCopy.data(), xEstCopy.size(),
                                                                xEstShape.data(), xEstShape.size());

        std::vector<uint8_t> maskTVec;
        maskTVec.reserve(T);
        for (int64_t i = 0; i < T; ++i) {
            maskTVec.push_back(originalMaskTData[i]);
        }

        std::array<int64_t, 2> maskTShapeForTensor = {1, T};
        Ort::Value maskTTensor =
            Ort::Value::CreateTensor<bool>(memoryInfo, reinterpret_cast<bool *>(maskTVec.data()), maskTVec.size(),
                                           maskTShapeForTensor.data(), maskTShapeForTensor.size());

        std::vector<uint8_t> boundariesVec;
        boundariesVec.reserve(boundariesAdjusted.size());
        for (uint8_t val : boundariesAdjusted) {
            boundariesVec.push_back(val != 0);
        }
        std::array<int64_t, 2> bdShape = {1, static_cast<int64_t>(boundariesAdjusted.size())};
        Ort::Value boundariesTensor =
            Ort::Value::CreateTensor<bool>(memoryInfo, reinterpret_cast<bool *>(boundariesVec.data()),
                                           boundariesVec.size(), bdShape.data(), bdShape.size());

        std::vector<uint8_t> tempMaskN;
        tempMaskN.reserve(maskN.size());
        for (uint8_t val : maskN) {
            tempMaskN.push_back(val != 0);
        }

        std::array<int64_t, 2> maskNShape = {1, static_cast<int64_t>(tempMaskN.size())};
        Ort::Value maskNTensor = Ort::Value::CreateTensor<bool>(memoryInfo, reinterpret_cast<bool *>(tempMaskN.data()),
                                                                tempMaskN.size(), maskNShape.data(), maskNShape.size());

        std::vector<float> threshVec = {threshold};
        std::array<int64_t, 1> scalarShape = {1};
        Ort::Value thresholdTensor =
            Ort::Value::CreateTensor<float>(memoryInfo, threshVec.data(), 1, scalarShape.data(), scalarShape.size());

        const char *inputNames[] = {"x_est", "boundaries", "maskT", "maskN", "threshold"};
        std::vector<Ort::Value> inputTensors;
        inputTensors.push_back(std::move(xEstTensor));
        inputTensors.push_back(std::move(boundariesTensor));
        inputTensors.push_back(std::move(maskTTensor));
        inputTensors.push_back(std::move(maskNTensor));
        inputTensors.push_back(std::move(thresholdTensor));

        const char *outputNames[] = {"presence", "scores"};

        auto outputTensors = sessEstimator->Run(Ort::RunOptions{nullptr}, inputNames, inputTensors.data(),
                                                inputTensors.size(), outputNames, 2);

        const bool *presenceDataBool = outputTensors[0].GetTensorData<bool>();
        size_t presenceCount = outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount();
        std::vector<float> presence;
        presence.reserve(presenceCount);
        for (size_t i = 0; i < presenceCount; ++i) {
            presence.push_back(presenceDataBool[i] ? 1.0f : 0.0f);
        }

        const float *scoresData = outputTensors[1].GetTensorData<float>();
        size_t scoresCount = outputTensors[1].GetTensorTypeAndShapeInfo().GetElementCount();
        std::vector<float> scores(scoresData, scoresData + scoresCount);

        return std::make_tuple(presence, scores);
    }

    InferenceOutput GameModel::inferSlice(const InferenceInput &input, float segThreshold, int segRadius,
                                          float estThreshold, const std::vector<float> &d3pmTs) const {
        InferenceOutput output;

        auto [xSegVal, xEstVal, maskTVal] = runEncoder(input.waveform, input.duration, input.language);

        if (!xSegVal || !xEstVal || !maskTVal) {
            return output;
        }

        auto xSegShape = xSegVal.GetTensorTypeAndShapeInfo().GetShape();
        auto xEstShape = xEstVal.GetTensorTypeAndShapeInfo().GetShape();
        auto maskTShape = maskTVal.GetTensorTypeAndShapeInfo().GetShape();

        auto xSegData = xSegVal.GetTensorData<float>();
        size_t xSegCount = xSegVal.GetTensorTypeAndShapeInfo().GetElementCount();
        std::vector<float> xSegClean(xSegData, xSegData + xSegCount);
        Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value xSegCleanVal = Ort::Value::CreateTensor<float>(memInfo, xSegClean.data(), xSegClean.size(),
                                                                  xSegShape.data(), xSegShape.size());

        auto xEstData = xEstVal.GetTensorData<float>();
        size_t xEstCount = xEstVal.GetTensorTypeAndShapeInfo().GetElementCount();
        std::vector<float> xEstClean(xEstData, xEstData + xEstCount);
        Ort::Value xEstCleanVal = Ort::Value::CreateTensor<float>(memInfo, xEstClean.data(), xEstClean.size(),
                                                                  xEstShape.data(), xEstShape.size());

        int64_t T = maskTShape[1];
        if (T <= 0) {
            return output;
        }

        const bool *maskTData = nullptr;
        try {
            maskTData = maskTVal.GetTensorData<bool>();
        }
        catch (const Ort::Exception &e) {
            throw std::runtime_error("Failed to get tensor data from maskT in inferSlice: " + std::string(e.what()));
        }

        std::vector<uint8_t> maskTBool(T);
        for (int64_t i = 0; i < T; ++i) {
            if (maskTData) {
                maskTBool[i] = maskTData[i] ? 1 : 0;
            } else {
                maskTBool[i] = false;
            }
        }

        std::vector<float> knownDurations = input.known_durations;
        if (knownDurations.empty()) {
            knownDurations.push_back(input.duration);
        }
        std::vector<uint8_t> knownBoundaries;
        knownBoundaries.resize(T, 0);

        std::vector<uint8_t> boundaries = runSegmenterWithConfig(
            xSegCleanVal, knownBoundaries, knownBoundaries, input.language, maskTVal, segThreshold, segRadius, d3pmTs);
        if (boundaries.empty()) {
            boundaries.resize(T, 0);
        }

        std::vector<float> durations;
        std::vector<uint8_t> maskN;
        std::tie(durations, maskN) = runBd2dur(boundaries, maskTBool);
        if (durations.empty() || maskN.empty()) {
            output.boundaries = boundaries;
            return output;
        }

        std::vector<float> presence, scores;
        std::tie(presence, scores) = runEstimator(xEstCleanVal, boundaries, maskTVal, maskN, estThreshold);

        output.boundaries = boundaries;
        output.durations = durations;
        output.presence = presence;
        output.scores = scores;
        output.maskN = maskN;

        return output;
    }
} // namespace Game
