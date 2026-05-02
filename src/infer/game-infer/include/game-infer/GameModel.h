/// @file GameModel.h
/// @brief ONNX Runtime model implementation for the GAME inference pipeline.

#pragma once

#include <atomic>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <dstools/OnnxModelBase.h>

#include "Game.h"
#include "GameGlobal.h"

namespace Game
{

    /// @brief Input data for a single inference slice.
    struct InferenceInput {
        std::vector<float> waveform;         ///< Audio waveform samples.
        float duration;                      ///< Duration in seconds.
        std::vector<float> known_durations;  ///< Pre-computed durations (empty if unknown).
        int language;                        ///< Language ID.
        std::vector<bool> maskT;             ///< Time-step mask.
        float timestep;                      ///< Time step in seconds.
    };

    /// @brief Output from a single inference slice.
    struct InferenceOutput {
        std::vector<uint8_t> boundaries;  ///< Detected boundary flags.
        std::vector<float> durations;     ///< Estimated note durations.
        std::vector<float> presence;      ///< Note presence probabilities.
        std::vector<float> scores;        ///< Pitch scores per note.
        std::vector<uint8_t> maskN;       ///< Note-level mask.
    };

    /// @brief CancellableOnnxModel with 4-stage ONNX pipeline (encoder, segmenter, bd2dur/dur2bd, estimator).
    class GAME_INFER_EXPORT GameModel : public dstools::infer::CancellableOnnxModel {
    public:
        GameModel();
        ~GameModel();

        /// @brief Get the model's target sample rate.
        /// @return Sample rate in Hz.
        int targetSampleRate() const;

        /// @brief Get the model's time step.
        /// @return Time step in seconds.
        float timestep() const;

        /// @brief Check whether the model includes a dur2bd sub-model.
        /// @return True if dur2bd is available.
        bool hasDur2bd() const;

        /// @brief Get the supported language map.
        /// @return Map of language name to language ID.
        const std::map<std::string, int> &languageMap() const;

        /// @brief Load the ONNX model files from a directory.
        /// @param modelPath Path to the model directory.
        /// @param provider Execution provider.
        /// @param device_id Device index for GPU providers.
        /// @param msg Output error message on failure.
        /// @return True on success.
        bool loadModel(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id,
                        std::string &msg);

        /// @brief Check whether a model is loaded.
        /// @return True if a model is currently loaded.
        bool isOpen() const;

        /// @brief Run full inference on waveform data.
        /// @param waveform_data Input audio samples.
        /// @param boundaries Output boundary flags.
        /// @param durations Output note durations.
        /// @param presence Output note presence probabilities.
        /// @param scores Output pitch scores.
        /// @param msg Output error message on failure.
        /// @return True on success.
        bool forward(const std::vector<float> &waveform_data, std::vector<bool> &boundaries,
                     std::vector<float> &durations, std::vector<float> &presence, std::vector<float> &scores,
                     std::string &msg) const;

        /// @brief Run inference with pre-computed durations.
        /// @param waveform_data Input audio samples.
        /// @param known_durations Pre-computed note durations.
        /// @param durations Output refined durations.
        /// @param presence Output note presence probabilities.
        /// @param scores Output pitch scores.
        /// @param msg Output error message on failure.
        /// @return True on success.
        bool forwardWithKnownDurations(const std::vector<float> &waveform_data,
                                       const std::vector<float> &known_durations, std::vector<float> &durations,
                                       std::vector<float> &presence, std::vector<float> &scores,
                                       std::string &msg) const;

        /// @brief Generate D3PM timestep schedule.
        /// @param t0 Initial timestep value.
        /// @param n_steps Number of steps.
        /// @return Vector of timestep values.
        static std::vector<float> generateD3pmTs(float t0, int n_steps);

        /// @brief Set segmenter threshold.
        void setSegThreshold(float threshold);
        /// @brief Set segmenter radius in seconds.
        void setSegRadiusSeconds(float radius);
        /// @brief Set segmenter radius in frames.
        void setSegRadiusFrames(float radiusFrames);
        /// @brief Set estimator threshold.
        void setEstThreshold(float threshold);
        /// @brief Set D3PM timestep schedule.
        void setD3pmTs(const std::vector<float> &ts);
        /// @brief Set the inference language.
        void setLanguage(int language);

    private:
        std::unique_ptr<Ort::Session> sessEncoder;
        std::unique_ptr<Ort::Session> sessSegmenter;
        std::unique_ptr<Ort::Session> sessEstimator;
        std::unique_ptr<Ort::Session> sessBd2dur;
        std::unique_ptr<Ort::Session> sessDur2bd;

        std::filesystem::path modelDir;
        float timestep;
        int sampleRate;
        int embeddingDim;

        std::vector<std::string> m_segInputNames;

        float m_seg_threshold = 0.2f;
        float m_seg_radius_seconds = 0.02f;
        float m_est_threshold = 0.2f;
        std::vector<float> m_d3pm_ts;

        float m_timestep = 0.01f;
        int m_language = 0;
        int m_target_sample_rate;
        bool m_has_languages = false;
        std::map<std::string, int> m_language_map;

        std::tuple<Ort::Value, Ort::Value, Ort::Value> runEncoder(const std::vector<float> &waveform, float duration,
                                                                  int language) const;

        std::vector<uint8_t> runDur2bd(const std::vector<float> &durations, const std::vector<uint8_t> &maskT) const;

        std::vector<uint8_t> runSegmenter(const Ort::Value &xSeg, const std::vector<uint8_t> &knownBoundaries,
                                          const std::vector<uint8_t> &prevBoundaries, int language,
                                          const Ort::Value &maskT, float threshold, int radius,
                                          const std::vector<float> &d3pmTs) const;

        std::vector<uint8_t> runSegmenterWithConfig(const Ort::Value &xSeg, const std::vector<uint8_t> &knownBoundaries,
                                                    const std::vector<uint8_t> &prevBoundaries, int language,
                                                    const Ort::Value &maskT, float threshold, int radius,
                                                    const std::vector<float> &d3pmTs) const;

        std::tuple<std::vector<float>, std::vector<uint8_t>> runBd2dur(const std::vector<uint8_t> &boundaries,
                                                                       const std::vector<uint8_t> &maskT) const;

        std::tuple<std::vector<float>, std::vector<float>>
        runEstimator(const Ort::Value &xEst, const std::vector<uint8_t> &boundaries, const Ort::Value &maskT,
                     const std::vector<uint8_t> &maskN, float threshold) const;

        InferenceOutput inferSlice(const InferenceInput &input, float segThreshold,
                                   int segRadius,
                                   float estThreshold, const std::vector<float> &d3pmTs) const;
    };
} // namespace Game
