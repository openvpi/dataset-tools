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

    struct InferenceInput {
        std::vector<float> waveform;
        float duration;
        std::vector<float> known_durations;
        int language;
        std::vector<bool> maskT;
        float timestep;
    };

    struct InferenceOutput {
        std::vector<uint8_t> boundaries;
        std::vector<float> durations;
        std::vector<float> presence;
        std::vector<float> scores;
        std::vector<uint8_t> maskN;
    };

    class GAME_INFER_EXPORT GameModel : public dstools::infer::CancellableOnnxModel {
    public:
        GameModel();
        ~GameModel();
        int get_target_sample_rate() const;
        float get_timestep() const;
        bool has_dur2bd() const;
        const std::map<std::string, int> &get_language_map() const;

        bool load_model(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id,
                        std::string &msg);
        bool is_open() const;

        bool forward(const std::vector<float> &waveform_data, std::vector<bool> &boundaries,
                     std::vector<float> &durations, std::vector<float> &presence, std::vector<float> &scores,
                     std::string &msg) const;

        bool forwardWithKnownDurations(const std::vector<float> &waveform_data,
                                       const std::vector<float> &known_durations, std::vector<float> &durations,
                                       std::vector<float> &presence, std::vector<float> &scores,
                                       std::string &msg) const;

        static std::vector<float> generate_d3pm_ts(float t0, int n_steps);

        void set_seg_threshold(float threshold);
        void set_seg_radius_seconds(float radius);
        void set_seg_radius_frames(float radiusFrames);
        void set_est_threshold(float threshold);
        void set_d3pm_ts(const std::vector<float> &ts);
        void set_language(int language);

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
