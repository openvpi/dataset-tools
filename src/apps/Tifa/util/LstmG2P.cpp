#include "LstmG2P.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <numeric>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include <onnxruntime_cxx_api.h>

#include "G2P.h"
#include "PathUtils.h"
#include "TextUtils.h"

using json = nlohmann::json;

namespace Tifa {

    namespace {
        struct FloatTensor {
            std::vector<float> data;
            std::vector<int64_t> shape;
        };

        FloatTensor copyFloatTensor(const Ort::Value &value) {
            FloatTensor tensor;
            const auto info = value.GetTensorTypeAndShapeInfo();
            tensor.shape = info.GetShape();
            const size_t count = info.GetElementCount();
            const float *data = value.GetTensorData<float>();
            tensor.data.assign(data, data + count);
            return tensor;
        }

        /** One decoder hypothesis; parent states stay untouched by extensions. */
        struct Beam {
            std::vector<int64_t> tokens;
            double score = 0.0;
            FloatTensor hidden;
            FloatTensor cell;
            bool finished = false;

            double normalizedScore() const {
                const size_t length = std::max<size_t>(1, tokens.size() - 1);
                return score / static_cast<double>(length);
            }
        };

        std::vector<std::string> sessionOutputNames(const Ort::Session &session) {
            Ort::AllocatorWithDefaultOptions allocator;
            std::vector<std::string> names;
            const size_t count = session.GetOutputCount();
            names.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                names.emplace_back(session.GetOutputNameAllocated(i, allocator).get());
            }
            return names;
        }
    } // namespace

    struct LstmG2P::Impl {
        Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "Tifa_LstmG2P"};
        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::unique_ptr<Ort::Session> encoder;
        std::unique_ptr<Ort::Session> decoder;
        std::vector<std::string> encoderOutputNames;
        std::vector<std::string> decoderOutputNames;
        std::unordered_map<char32_t, int64_t> charVocab;
        std::unordered_map<std::string, int64_t> phonemeVocab;
        std::vector<std::string> indexToPhoneme;
        int64_t unkIdx = 0;
        int64_t padIdx = 0;
        int64_t bosIdx = 0;
        int64_t eosIdx = 0;
        int64_t charUnkIdx = 0;
        bool ready = false;
    };

    LstmG2P::LstmG2P() : m_impl(std::make_unique<Impl>()) {
    }

    LstmG2P::~LstmG2P() = default;

    bool LstmG2P::available() const {
        return m_impl && m_impl->ready;
    }

    bool LstmG2P::init(const std::filesystem::path &modelDir, std::string &error) {
        const std::filesystem::path encoderPath = modelDir / "encoder.onnx";
        const std::filesystem::path decoderPath = modelDir / "decoder.onnx";
        const std::filesystem::path charPath = modelDir / "char.json";
        const std::filesystem::path phonemePath = modelDir / "phonemes.json";
        for (const auto &path : {encoderPath, decoderPath, charPath, phonemePath}) {
            if (!std::filesystem::exists(path)) {
                error = "Missing OOV model file: " + pathToUtf8(path);
                return false;
            }
        }

        try {
            std::ifstream charStream(charPath);
            json charData = json::parse(charStream);
            m_impl->charVocab.clear();
            for (const auto &[key, value] : charData.items()) {
                const std::u32string symbol = Text::decode(key);
                if (symbol.size() == 1) {
                    m_impl->charVocab[symbol.front()] = value.get<int64_t>();
                }
            }
            const auto charUnk = charData.find("<unk>");
            m_impl->charUnkIdx = charUnk != charData.end() ? charUnk->get<int64_t>() : 0;

            std::ifstream phonemeStream(phonemePath);
            json phonemeData = json::parse(phonemeStream);
            m_impl->phonemeVocab.clear();
            int64_t maxIndex = -1;
            for (const auto &[key, value] : phonemeData.items()) {
                const int64_t index = value.get<int64_t>();
                m_impl->phonemeVocab[key] = index;
                maxIndex = std::max(maxIndex, index);
            }
            if (!m_impl->phonemeVocab.count("<unk>") || !m_impl->phonemeVocab.count("<pad>") ||
                !m_impl->phonemeVocab.count("<bos>") || !m_impl->phonemeVocab.count("<eos>")) {
                error = "Phoneme vocabulary misses a special token: " + pathToUtf8(phonemePath);
                return false;
            }
            m_impl->unkIdx = m_impl->phonemeVocab.at("<unk>");
            m_impl->padIdx = m_impl->phonemeVocab.at("<pad>");
            m_impl->bosIdx = m_impl->phonemeVocab.at("<bos>");
            m_impl->eosIdx = m_impl->phonemeVocab.at("<eos>");
            m_impl->indexToPhoneme.assign(static_cast<size_t>(maxIndex) + 1, std::string());
            for (const auto &[key, index] : m_impl->phonemeVocab) {
                if (index >= 0) {
                    m_impl->indexToPhoneme[static_cast<size_t>(index)] = key;
                }
            }

            Ort::SessionOptions options;
            options.SetIntraOpNumThreads(1);
            options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
#ifdef _WIN32
            m_impl->encoder = std::make_unique<Ort::Session>(m_impl->env, encoderPath.wstring().c_str(), options);
            m_impl->decoder = std::make_unique<Ort::Session>(m_impl->env, decoderPath.wstring().c_str(), options);
#else
            m_impl->encoder = std::make_unique<Ort::Session>(m_impl->env, encoderPath.c_str(), options);
            m_impl->decoder = std::make_unique<Ort::Session>(m_impl->env, decoderPath.c_str(), options);
#endif
            m_impl->encoderOutputNames = sessionOutputNames(*m_impl->encoder);
            m_impl->decoderOutputNames = sessionOutputNames(*m_impl->decoder);
            if (m_impl->encoderOutputNames.size() < 3 || m_impl->decoderOutputNames.size() < 3) {
                error = "Unexpected OOV model outputs in: " + pathToUtf8(modelDir);
                return false;
            }
        } catch (const std::exception &e) {
            error = std::string("Failed to load the OOV model: ") + e.what();
            return false;
        }
        m_impl->ready = true;
        return true;
    }

    bool LstmG2P::usesCharacters(const std::u32string &token) const {
        if (!m_impl || !m_impl->ready) {
            return false;
        }
        for (const char32_t c : token) {
            if (!m_impl->charVocab.count(c)) {
                return false;
            }
        }
        return true;
    }

    std::vector<std::vector<std::string>> LstmG2P::predict(const std::u32string &word, std::string &error) const {
        if (!m_impl || !m_impl->ready) {
            error = "The OOV model is not loaded.";
            return {};
        }

        // Strip the token and lowercase it, as the model was trained on lowercase letters.
        std::u32string text = Text::lower(word);
        while (!text.empty() && Text::isSpace(text.front())) {
            text.erase(text.begin());
        }
        while (!text.empty() && Text::isSpace(text.back())) {
            text.pop_back();
        }

        try {
            std::vector<int64_t> source;
            source.push_back(m_impl->bosIdx);
            for (const char32_t c : text) {
                const auto it = m_impl->charVocab.find(c);
                source.push_back(it != m_impl->charVocab.end() ? it->second : m_impl->charUnkIdx);
            }
            source.push_back(m_impl->eosIdx);

            const std::array<int64_t, 2> sourceShape = {1, static_cast<int64_t>(source.size())};
            Ort::Value sourceTensor =
                Ort::Value::CreateTensor<int64_t>(m_impl->memoryInfo, source.data(), source.size(),
                                                  sourceShape.data(), sourceShape.size());
            const char *encoderInputNames[] = {"input_ids"};
            std::vector<const char *> encoderOutputNamePtrs;
            for (const auto &name : m_impl->encoderOutputNames) {
                encoderOutputNamePtrs.push_back(name.c_str());
            }
            auto encoderOutputs =
                m_impl->encoder->Run(Ort::RunOptions{nullptr}, encoderInputNames, &sourceTensor, 1,
                                     encoderOutputNamePtrs.data(), encoderOutputNamePtrs.size());

            const FloatTensor encoderFeatures = copyFloatTensor(encoderOutputs[0]);

            std::vector<Beam> beams;
            Beam initial;
            initial.tokens.push_back(m_impl->bosIdx);
            initial.hidden = copyFloatTensor(encoderOutputs[1]);
            initial.cell = copyFloatTensor(encoderOutputs[2]);
            beams.push_back(std::move(initial));

            for (int step = 0; step < maxLength; ++step) {
                const bool allFinished = std::all_of(beams.begin(), beams.end(), [](const Beam &beam) {
                    return beam.finished;
                });
                if (allFinished) {
                    break;
                }

                std::vector<Beam> candidates;
                for (const Beam &beam : beams) {
                    if (beam.finished) {
                        candidates.push_back(beam);
                        continue;
                    }

                    const std::array<int64_t, 2> inputShape = {1, 1};
                    Ort::Value decoderInput =
                        Ort::Value::CreateTensor<int64_t>(m_impl->memoryInfo, const_cast<int64_t *>(&beam.tokens.back()),
                                                          1, inputShape.data(), inputShape.size());
                    Ort::Value hiddenTensor = Ort::Value::CreateTensor<float>(
                        m_impl->memoryInfo, const_cast<float *>(beam.hidden.data.data()), beam.hidden.data.size(),
                        beam.hidden.shape.data(), beam.hidden.shape.size());
                    Ort::Value cellTensor = Ort::Value::CreateTensor<float>(
                        m_impl->memoryInfo, const_cast<float *>(beam.cell.data.data()), beam.cell.data.size(),
                        beam.cell.shape.data(), beam.cell.shape.size());
                    Ort::Value featuresTensor =
                        Ort::Value::CreateTensor<float>(m_impl->memoryInfo, const_cast<float *>(encoderFeatures.data.data()),
                                                        encoderFeatures.data.size(), encoderFeatures.shape.data(),
                                                        encoderFeatures.shape.size());
                    const char *decoderInputNames[] = {"decoder_input", "hidden", "cell", "encoder_outputs"};
                    Ort::Value decoderInputs[] = {std::move(decoderInput), std::move(hiddenTensor), std::move(cellTensor),
                                                  std::move(featuresTensor)};
                    std::vector<const char *> decoderOutputNamePtrs;
                    for (const auto &name : m_impl->decoderOutputNames) {
                        decoderOutputNamePtrs.push_back(name.c_str());
                    }
                    auto decoderOutputs =
                        m_impl->decoder->Run(Ort::RunOptions{nullptr}, decoderInputNames, decoderInputs, 4,
                                             decoderOutputNamePtrs.data(), decoderOutputNamePtrs.size());

                    const auto logitsInfo = decoderOutputs[0].GetTensorTypeAndShapeInfo();
                    const size_t logitsCount = logitsInfo.GetElementCount();
                    const float *logitsData = decoderOutputs[0].GetTensorData<float>();
                    std::vector<double> logProbs(logitsData, logitsData + logitsCount);
                    const double maxLogit = *std::max_element(logProbs.begin(), logProbs.end());
                    double sum = 0.0;
                    for (auto &value : logProbs) {
                        value -= maxLogit;
                        sum += std::exp(value);
                    }
                    const double logSum = std::log(sum);
                    for (auto &value : logProbs) {
                        value -= logSum;
                    }

                    const FloatTensor hidden = copyFloatTensor(decoderOutputs[1]);
                    const FloatTensor cell = copyFloatTensor(decoderOutputs[2]);

                    std::vector<size_t> order(logProbs.size());
                    std::iota(order.begin(), order.end(), 0);
                    std::stable_sort(order.begin(), order.end(), [&logProbs](size_t a, size_t b) {
                        return logProbs[a] > logProbs[b];
                    });
                    const size_t kept = std::min<size_t>(static_cast<size_t>(std::max(1, beamSize)), order.size());
                    for (size_t k = 0; k < kept; ++k) {
                        const int64_t predicted = static_cast<int64_t>(order[k]);
                        Beam child;
                        child.tokens = beam.tokens;
                        child.tokens.push_back(predicted);
                        child.score = beam.score + logProbs[order[k]];
                        child.hidden = hidden;
                        child.cell = cell;
                        child.finished = predicted == m_impl->eosIdx;
                        candidates.push_back(std::move(child));
                    }
                }

                std::stable_sort(candidates.begin(), candidates.end(), [](const Beam &a, const Beam &b) {
                    return a.normalizedScore() > b.normalizedScore();
                });
                if (candidates.size() > static_cast<size_t>(std::max(1, beamSize))) {
                    candidates.resize(static_cast<size_t>(std::max(1, beamSize)));
                }
                beams = std::move(candidates);
            }

            // Removing special tokens can collapse distinct beams to one reading.
            std::vector<std::vector<std::string>> pronunciations;
            for (const Beam &beam : beams) {
                std::vector<std::string> phonemes;
                for (size_t i = 1; i < beam.tokens.size(); ++i) {
                    const int64_t index = beam.tokens[i];
                    if (index == m_impl->unkIdx || index == m_impl->padIdx || index == m_impl->bosIdx ||
                        index == m_impl->eosIdx) {
                        continue;
                    }
                    if (index >= 0 && static_cast<size_t>(index) < m_impl->indexToPhoneme.size() &&
                        !m_impl->indexToPhoneme[static_cast<size_t>(index)].empty()) {
                        phonemes.push_back(m_impl->indexToPhoneme[static_cast<size_t>(index)]);
                    }
                }
                if (std::find(pronunciations.begin(), pronunciations.end(), phonemes) == pronunciations.end()) {
                    pronunciations.push_back(std::move(phonemes));
                }
            }
            return pronunciations;
        } catch (const std::exception &e) {
            error = std::string("OOV model inference failed: ") + e.what();
            return {};
        }
    }

} // namespace Tifa
