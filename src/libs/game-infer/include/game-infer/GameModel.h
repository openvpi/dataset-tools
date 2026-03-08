#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>

#include "Game.h"

namespace Game
{

    struct InferenceInput {
        std::vector<float> waveform; // [1, samples]
        float duration; // seconds
        std::vector<float> known_durations; // [1, N_known]
        int language; // int64
        std::vector<bool> maskT; // [1, T], bool (用于 dur2bd/bd2dur 辅助)
        float timestep;
    };

    struct InferenceOutput {
        std::vector<uint8_t> boundaries; // [1, T], float
        std::vector<float> durations; // [1, N], float (seconds)
        std::vector<float> presence; // [1, N], float (confidence)
        std::vector<float> scores; // [1, N], float (MIDI pitch)
        std::vector<uint8_t> maskN; // [1, N], bool (valid notes)
    };

    class GameModel {
    public:
        GameModel(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id);
        ~GameModel();
        int get_target_sample_rate() const;

        static bool is_open();
        static void terminate();

        bool forward(const std::vector<float> &waveform_data, std::vector<bool> &boundaries,
                     std::vector<float> &durations, std::vector<float> &presence, std::vector<float> &scores,
                     std::string &msg) const;

        static std::vector<float> generate_d3pm_ts(float t0, int n_steps);

        // Parameter setter methods
        void set_seg_threshold(float threshold);
        void set_seg_radius_seconds(float radius);
        void set_seg_radius_frames(float radiusFrames);
        void set_est_threshold(float threshold);
        void set_d3pm_ts(const std::vector<float> &ts);
        void set_language(int language);

    private:
        // ONNX sessions
        std::unique_ptr<Ort::Session> sessEncoder;
        std::unique_ptr<Ort::Session> sessSegmenter;
        std::unique_ptr<Ort::Session> sessEstimator;
        std::unique_ptr<Ort::Session> sessDur2bd;

        Ort::Env env;
        Ort::SessionOptions sessionOptions;

        std::filesystem::path modelDir;
        float timestep;
        int sampleRate;
        int embeddingDim;

        float m_seg_threshold = 0.2f;
        float m_seg_radius_seconds = 0.02f;
        float m_est_threshold = 0.2f;
        std::vector<float> m_d3pm_ts;

        float m_timestep = 0.01f;
        int m_language = 0;
        int m_target_sample_rate;

        std::tuple<Ort::Value, Ort::Value, Ort::Value> runEncoder(const std::vector<float> &waveform, float duration,
                                                                  int language) const;

        std::vector<uint8_t> runDur2bd(const std::vector<float> &knownDurations,
                                       const std::vector<uint8_t> &maskT) const;

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
                                   int segRadius, // 帧单位
                                   float estThreshold, const std::vector<float> &d3pmTs) const;
    };
} // namespace Game
