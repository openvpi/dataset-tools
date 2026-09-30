#include "Model.h"

#include <array>

#include "PathUtils.h"

#ifdef ONNXRUNTIME_ENABLE_DML
#    include <dml_provider_factory.h>
#endif

namespace Tifa {

    static bool initDirectML(Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage = nullptr);
    static bool initCUDA(Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage = nullptr);

    namespace {

        /** ONNX Runtime environment shared by every session of the application. */
        Ort::Env &sharedEnv() {
            static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "Tifa");
            return env;
        }

        /** Locate a graph, accepting the suffix some optimizers add. */
        std::filesystem::path findGraph(const std::filesystem::path &dir, const std::string &name) {
            const std::filesystem::path plain = dir / (name + ".onnx");
            if (std::filesystem::exists(plain)) {
                return plain;
            }
            const std::filesystem::path optimized = dir / (name + "_optimized.onnx");
            if (std::filesystem::exists(optimized)) {
                return optimized;
            }
            return {};
        }

        std::unique_ptr<Ort::Session> createSession(const std::filesystem::path &path,
                                                    const Ort::SessionOptions &options, std::string &error) {
            try {
#ifdef _WIN32
                return std::make_unique<Ort::Session>(sharedEnv(), path.wstring().c_str(), options);
#else
                return std::make_unique<Ort::Session>(sharedEnv(), path.c_str(), options);
#endif
            } catch (const Ort::Exception &e) {
                error = "Failed to load '" + pathToUtf8(path.filename()) + "': " + e.what();
                return nullptr;
            }
        }

        template <typename T>
        Ort::Value createTensor(const Ort::MemoryInfo &info, const void *data, size_t count,
                                const std::vector<int64_t> &shape) {
            return Ort::Value::CreateTensor<T>(info, const_cast<T *>(static_cast<const T *>(data)), count, shape.data(),
                                               shape.size());
        }

        /** Boolean masks are stored as bytes so that they can be packed tightly. */
        Ort::Value createBoolTensor(const Ort::MemoryInfo &info, const std::vector<uint8_t> &data,
                                    const std::vector<int64_t> &shape) {
            return Ort::Value::CreateTensor<bool>(info, reinterpret_cast<bool *>(const_cast<uint8_t *>(data.data())),
                                                  data.size(), shape.data(), shape.size());
        }

        bool runSession(Ort::Session &session, const std::vector<const char *> &inputNames,
                        std::vector<Ort::Value> &inputs, const std::vector<const char *> &outputNames,
                        std::vector<Ort::Value> &outputs, std::string &error) {
            try {
                outputs = session.Run(Ort::RunOptions{nullptr}, inputNames.data(), inputs.data(), inputs.size(),
                                      outputNames.data(), outputNames.size());
                return true;
            } catch (const Ort::Exception &e) {
                error = e.what();
                return false;
            }
        }

        std::vector<int64_t> outputShape(const Ort::Value &value) {
            return value.GetTensorTypeAndShapeInfo().GetShape();
        }

        void copyFloat(const Ort::Value &value, std::vector<float> &data) {
            const size_t count = value.GetTensorTypeAndShapeInfo().GetElementCount();
            const float *source = value.GetTensorData<float>();
            data.assign(source, source + count);
        }

        void copyInt64(const Ort::Value &value, std::vector<int64_t> &data) {
            const size_t count = value.GetTensorTypeAndShapeInfo().GetElementCount();
            const int64_t *source = value.GetTensorData<int64_t>();
            data.assign(source, source + count);
        }

        void copyBool(const Ort::Value &value, std::vector<uint8_t> &data) {
            const size_t count = value.GetTensorTypeAndShapeInfo().GetElementCount();
            const bool *source = value.GetTensorData<bool>();
            data.resize(count);
            for (size_t i = 0; i < count; ++i) {
                data[i] = source[i] ? 1 : 0;
            }
        }

    } // namespace

    Model::~Model() = default;

    bool Model::load(const std::filesystem::path &modelDir, const ExecutionProvider provider, const int deviceId,
                         std::string &error) {
        m_spectrogram.reset();
        m_model.reset();
        m_prepare.reset();
        m_score.reset();
        m_select.reset();

        const std::array<std::pair<const char *, std::filesystem::path>, 5> graphs = {{
            {"spectrogram", findGraph(modelDir, "spectrogram")},
            {"model", findGraph(modelDir, "model")},
            {"prepare", findGraph(modelDir, "prepare")},
            {"score", findGraph(modelDir, "score")},
            {"select", findGraph(modelDir, "select")},
        }};
        for (const auto &[name, path] : graphs) {
            if (path.empty()) {
                error = "Missing graph '" + std::string(name) + ".onnx' in '" + pathToUtf8(modelDir) + "'";
                return false;
            }
        }

        // The feature extractor and the acoustic model benefit from the GPU; the
        // remaining graphs are index arithmetic and stay on the CPU.
        Ort::SessionOptions options;
        options.SetInterOpNumThreads(4);
        switch (provider) {
#ifdef ONNXRUNTIME_ENABLE_DML
            case ExecutionProvider::DML: {
                std::string message;
                if (!initDirectML(options, deviceId, &message)) {
                    error = "Failed to enable DirectML: " + message;
                    return false;
                }
                break;
            }
#endif
#if ONNXRUNTIME_ENABLE_CUDA
            case ExecutionProvider::CUDA: {
                std::string message;
                if (!initCUDA(options, deviceId, &message)) {
                    error = "Failed to enable CUDA: " + message;
                    return false;
                }
                break;
            }
#endif
            default:
                break;
        }

        Ort::SessionOptions cpuOptions;
        cpuOptions.SetInterOpNumThreads(4);

        if ((m_spectrogram = createSession(graphs[0].second, options, error)) == nullptr ||
            (m_model = createSession(graphs[1].second, options, error)) == nullptr ||
            (m_prepare = createSession(graphs[2].second, cpuOptions, error)) == nullptr ||
            (m_score = createSession(graphs[3].second, cpuOptions, error)) == nullptr ||
            (m_select = createSession(graphs[4].second, cpuOptions, error)) == nullptr) {
            return false;
        }

        m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        return true;
    }

    bool Model::spectrogram(const std::vector<float> &waveform, const float duration, SpectrogramOutput &output,
                                std::string &error) const {
        if (!m_spectrogram) {
            error = "The spectrogram graph is not loaded";
            return false;
        }
        if (waveform.empty()) {
            error = "The waveform is empty";
            return false;
        }

        constexpr std::array<const char *, 2> inputNames = {"waveform", "duration"};
        constexpr std::array<const char *, 2> outputNames = {"spectrogram", "maskT"};
        const std::array<float, 1> durations = {duration};

        std::vector<Ort::Value> inputs;
        inputs.push_back(createTensor<float>(m_memoryInfo, waveform.data(), waveform.size(),
                                             {1, static_cast<int64_t>(waveform.size())}));
        inputs.push_back(createTensor<float>(m_memoryInfo, durations.data(), durations.size(), {1}));

        std::vector<Ort::Value> outputs;
        if (!runSession(*m_spectrogram, {inputNames.begin(), inputNames.end()}, inputs,
                        {outputNames.begin(), outputNames.end()}, outputs, error)) {
            error = "Spectrogram extraction failed: " + error;
            return false;
        }

        const std::vector<int64_t> shape = outputShape(outputs[0]);
        if (shape.size() != 3) {
            error = "Unexpected spectrogram shape";
            return false;
        }
        output.frames = shape[1];
        output.mels = shape[2];
        copyFloat(outputs[0], output.features);
        copyBool(outputs[1], output.mask);
        return true;
    }

    bool Model::features(const SpectrogramOutput &spec, const int64_t *tokens, const int64_t tokenCount,
                             FeatureOutput &output, std::string &error) const {
        if (!m_model) {
            error = "The model graph is not loaded";
            return false;
        }
        if (tokenCount <= 0) {
            error = "The token sequence is empty";
            return false;
        }

        std::vector<uint8_t> maskN(static_cast<size_t>(tokenCount), 0);
        for (int64_t i = 0; i < tokenCount; ++i) {
            maskN[static_cast<size_t>(i)] = tokens[i] != 0 ? 1 : 0;
        }

        constexpr std::array<const char *, 4> inputNames = {"spectrogram", "tokens", "maskT", "maskN"};
        constexpr std::array<const char *, 2> outputNames = {"similarities", "logits"};

        std::vector<Ort::Value> inputs;
        inputs.push_back(createTensor<float>(m_memoryInfo, spec.features.data(), spec.features.size(),
                                             {1, spec.frames, spec.mels}));
        inputs.push_back(createTensor<int64_t>(m_memoryInfo, tokens, static_cast<size_t>(tokenCount),
                                               {1, tokenCount}));
        inputs.push_back(createBoolTensor(m_memoryInfo, spec.mask, {1, spec.frames}));
        inputs.push_back(createBoolTensor(m_memoryInfo, maskN, {1, tokenCount}));

        std::vector<Ort::Value> outputs;
        if (!runSession(*m_model, {inputNames.begin(), inputNames.end()}, inputs, {outputNames.begin(), outputNames.end()},
                        outputs, error)) {
            error = "Acoustic model failed: " + error;
            return false;
        }

        const std::vector<int64_t> similarityShape = outputShape(outputs[0]);
        const std::vector<int64_t> logitsShape = outputShape(outputs[1]);
        if (similarityShape.size() != 3 || logitsShape.size() != 3) {
            error = "Unexpected model output shape";
            return false;
        }
        output.frames = similarityShape[1];
        output.tokens = similarityShape[2];
        output.vocab = logitsShape[2];
        copyFloat(outputs[0], output.similarities);
        copyFloat(outputs[1], output.logits);
        return true;
    }

    bool Model::prepare(const PathGrid &grid, std::vector<int64_t> &tokens, std::vector<int64_t> &segments,
                            std::vector<int64_t> &mapping, std::string &error) const {
        if (!m_prepare) {
            error = "The prepare graph is not loaded";
            return false;
        }

        constexpr std::array<const char *, 4> inputNames = {"paths", "words", "candidates", "grouped"};
        constexpr std::array<const char *, 3> outputNames = {"tokens", "segments", "mapping"};
        static const std::array<int64_t, 1> scalarShape = {0};
        const std::array<uint8_t, 1> grouped = {0};

        std::vector<Ort::Value> inputs;
        inputs.push_back(createTensor<int64_t>(m_memoryInfo, grid.tokens.data(), grid.tokens.size(),
                                               {1, grid.capacity, grid.width}));
        inputs.push_back(createTensor<int64_t>(m_memoryInfo, grid.words.data(), grid.words.size(), {1, grid.capacity}));
        inputs.push_back(createBoolTensor(m_memoryInfo, grid.candidates, {1, grid.wordCount, grid.width}));
        // The host always scores the local (levenshtein) units of a candidate.
        inputs.push_back(Ort::Value::CreateTensor<bool>(m_memoryInfo, const_cast<bool *>(reinterpret_cast<const bool *>(
                                                                        grouped.data())),
                                                        grouped.size(), scalarShape.data(), 0));

        std::vector<Ort::Value> outputs;
        if (!runSession(*m_prepare, {inputNames.begin(), inputNames.end()}, inputs,
                        {outputNames.begin(), outputNames.end()}, outputs, error)) {
            error = "Path preparation failed: " + error;
            return false;
        }

        copyInt64(outputs[0], tokens);
        copyInt64(outputs[1], segments);
        copyInt64(outputs[2], mapping);
        return true;
    }

    bool Model::score(const std::vector<float> &logits, const int64_t capacity, const int64_t vocab,
                          const PathGrid &grid, const std::vector<int64_t> &segments,
                          const std::vector<int64_t> &mapping, ScoreOutput &output, std::string &error) const {
        if (!m_score) {
            error = "The score graph is not loaded";
            return false;
        }
        if (static_cast<int64_t>(logits.size()) != capacity * vocab) {
            error = "Logits do not match the candidate capacity";
            return false;
        }

        constexpr std::array<const char *, 5> inputNames = {"logits", "paths", "words", "segments", "mapping"};
        constexpr std::array<const char *, 5> outputNames = {"descriptors", "lengths", "costs", "tails", "capacity"};

        std::vector<Ort::Value> inputs;
        inputs.push_back(createTensor<float>(m_memoryInfo, logits.data(), logits.size(), {1, capacity, vocab}));
        inputs.push_back(createTensor<int64_t>(m_memoryInfo, grid.tokens.data(), grid.tokens.size(),
                                               {1, grid.capacity, grid.width}));
        inputs.push_back(createTensor<int64_t>(m_memoryInfo, grid.words.data(), grid.words.size(), {1, grid.capacity}));
        inputs.push_back(createTensor<int64_t>(m_memoryInfo, segments.data(), segments.size(), {1, grid.capacity}));
        inputs.push_back(createTensor<int64_t>(m_memoryInfo, mapping.data(), mapping.size(), {1, grid.capacity}));

        std::vector<Ort::Value> outputs;
        if (!runSession(*m_score, {inputNames.begin(), inputNames.end()}, inputs, {outputNames.begin(), outputNames.end()},
                        outputs, error)) {
            error = "Candidate scoring failed: " + error;
            return false;
        }

        const std::vector<int64_t> descriptorShape = outputShape(outputs[0]);
        const std::vector<int64_t> lengthShape = outputShape(outputs[1]);
        if (descriptorShape.size() != 3 || lengthShape.size() != 3) {
            error = "Unexpected score output shape";
            return false;
        }
        output.p1 = descriptorShape[1];
        output.width = lengthShape[2];
        copyInt64(outputs[0], output.descriptors);
        copyInt64(outputs[1], output.lengths);
        copyFloat(outputs[2], output.costs);
        copyFloat(outputs[3], output.tails);
        copyInt64(outputs[4], output.capacity);
        return true;
    }

    bool Model::select(const PathGrid &grid, const std::vector<int64_t> &choices, SelectOutput &output,
                           std::string &error) const {
        if (!m_select) {
            error = "The select graph is not loaded";
            return false;
        }
        if (static_cast<int64_t>(choices.size()) != grid.wordCount) {
            error = "The choices do not match the word count";
            return false;
        }

        constexpr std::array<const char *, 4> inputNames = {"paths", "words", "groups", "choices"};
        constexpr std::array<const char *, 4> outputNames = {"best_tokens", "best_words", "best_groups", "maskN"};

        std::vector<Ort::Value> inputs;
        inputs.push_back(createTensor<int64_t>(m_memoryInfo, grid.tokens.data(), grid.tokens.size(),
                                               {1, grid.capacity, grid.width}));
        inputs.push_back(createTensor<int64_t>(m_memoryInfo, grid.words.data(), grid.words.size(), {1, grid.capacity}));
        inputs.push_back(createTensor<int64_t>(m_memoryInfo, grid.groups.data(), grid.groups.size(),
                                               {1, grid.capacity, grid.width}));
        inputs.push_back(createTensor<int64_t>(m_memoryInfo, choices.data(), choices.size(), {1, grid.wordCount}));

        std::vector<Ort::Value> outputs;
        if (!runSession(*m_select, {inputNames.begin(), inputNames.end()}, inputs,
                        {outputNames.begin(), outputNames.end()}, outputs, error)) {
            error = "Path selection failed: " + error;
            return false;
        }

        const std::vector<int64_t> shape = outputShape(outputs[0]);
        if (shape.size() != 2) {
            error = "Unexpected select output shape";
            return false;
        }
        output.capacity = shape[1];
        copyInt64(outputs[0], output.tokens);
        copyInt64(outputs[1], output.words);
        copyInt64(outputs[2], output.groups);
        copyBool(outputs[3], output.mask);
        return true;
    }

    static bool initDirectML(Ort::SessionOptions &options, const int deviceIndex, std::string *errorMessage) {
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
            if (errorMessage) {
                *errorMessage = getApiStatus.GetErrorMessage();
            }
            return false;
        }

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

    static bool initCUDA(Ort::SessionOptions &options, const int deviceIndex, std::string *errorMessage) {
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

        std::unique_ptr<OrtCUDAProviderOptionsV2, decltype(ortApi.ReleaseCUDAProviderOptions)> cudaOptions(
            cudaOptionsPtr, ortApi.ReleaseCUDAProviderOptions);

        if (!createStatus.IsOK()) {
            if (errorMessage) {
                *errorMessage = createStatus.GetErrorMessage();
            }
            return false;
        }

        {
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

} // namespace Tifa
