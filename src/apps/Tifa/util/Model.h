#ifndef TIFA_MODEL_H
#define TIFA_MODEL_H

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>

#include "Encoding.h"
#include "Provider.h"

namespace Tifa {

    /** Outputs of the exported spectrogram graph. */
    struct SpectrogramOutput {
        int64_t frames = 0;
        int64_t mels = 0;
        std::vector<float> features; // [frames, mels]
        std::vector<uint8_t> mask;   // [frames]
    };

    /** Outputs of the exported model graph. */
    struct FeatureOutput {
        int64_t frames = 0;
        int64_t tokens = 0;
        int64_t vocab = 0;
        std::vector<float> similarities; // [frames, tokens]
        std::vector<float> logits;       // [tokens, vocab]
    };

    /**
     * Outputs of the exported score graph, where ``p1`` is the padded capacity
     * ``P + 1`` of the score fragments.
     */
    struct ScoreOutput {
        int64_t p1 = 0;
        int64_t width = 0;
        std::vector<int64_t> descriptors; // [p1, 2]
        std::vector<int64_t> lengths;     // [p1, width]
        std::vector<float> costs;         // [p1, width, p1]
        std::vector<float> tails;         // [p1, p1]
        std::vector<int64_t> capacity;    // [p1]

        int64_t lengthAt(int64_t fragment, int64_t candidate) const {
            return lengths[fragment * width + candidate];
        }

        float costAt(int64_t fragment, int64_t candidate, int64_t target) const {
            return costs[(fragment * width + candidate) * p1 + target];
        }

        float tailAt(int64_t source, int64_t target) const {
            return tails[source * p1 + target];
        }
    };

    /** Outputs of the exported select graph, which materializes chosen paths. */
    struct SelectOutput {
        int64_t capacity = 0;
        std::vector<int64_t> tokens; // [capacity]
        std::vector<int64_t> words;  // [capacity]
        std::vector<int64_t> groups; // [capacity]
        std::vector<uint8_t> mask;   // [capacity]
    };

    /**
     * The five graphs of an exported TIFA model.
     *
     * The spectrogram and model graphs run on the selected execution provider;
     * prepare, score and select are pinned to the CPU because they are index
     * arithmetic over the candidate grids.
     */
    class Model {
    public:
        Model() = default;
        ~Model();

        Model(const Model &) = delete;
        Model &operator=(const Model &) = delete;

        /** Load the five graphs from a model directory. */
        bool load(const std::filesystem::path &modelDir, ExecutionProvider provider, int deviceId, std::string &error);

        bool valid() const {
            return m_spectrogram && m_model && m_prepare && m_score && m_select;
        }

        /** Mel spectrogram of one mono waveform and the frames covered by *duration*. */
        bool spectrogram(const std::vector<float> &waveform, float duration, SpectrogramOutput &output,
                         std::string &error) const;

        /**
         * Frame and token features of one utterance.
         *
         * @param spec         Spectrogram of the utterance.
         * @param tokens       Token IDs [tokenCount]; zero marks padding.
         * @param tokenCount   Number of tokens, which is the capacity of the
         *                     candidate grid so that the graphs stay aligned.
         */
        bool features(const SpectrogramOutput &spec, const int64_t *tokens, int64_t tokenCount, FeatureOutput &output,
                      std::string &error) const;

        /** Scoring units of the candidates: ``prepare.onnx``. */
        bool prepare(const PathGrid &grid, std::vector<int64_t> &tokens, std::vector<int64_t> &segments,
                     std::vector<int64_t> &mapping, std::string &error) const;

        /** Candidate scores of one word: ``score.onnx``. */
        bool score(const std::vector<float> &logits, int64_t capacity, int64_t vocab, const PathGrid &grid,
                   const std::vector<int64_t> &segments, const std::vector<int64_t> &mapping, ScoreOutput &output,
                   std::string &error) const;

        /** Materialize the chosen candidate of every word: ``select.onnx``. */
        bool select(const PathGrid &grid, const std::vector<int64_t> &choices, SelectOutput &output,
                    std::string &error) const;

    private:
        std::unique_ptr<Ort::Session> m_spectrogram;
        std::unique_ptr<Ort::Session> m_model;
        std::unique_ptr<Ort::Session> m_prepare;
        std::unique_ptr<Ort::Session> m_score;
        std::unique_ptr<Ort::Session> m_select;
        Ort::MemoryInfo m_memoryInfo{nullptr};
    };

} // namespace Tifa

#endif // TIFA_MODEL_H
