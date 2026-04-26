#pragma once

/// @file InferBridge.h
/// @brief Central bridge header that encapsulates all infer engine types and defines
///        EngineTraits specializations for use by apps through the libs layer.

#include <dsfw/IModelProvider.h>
#include <dsfw/InferenceModelProvider.h>
#include <AsrPipeline.h>
#include <FunAsrModelProvider.h>
#include <game-infer/Game.h>
#include <hubert-infer/DictionaryG2P.h>
#include <hubert-infer/Hfa.h>
#include <rmvpe-infer/Rmvpe.h>

namespace dstools {

    template <typename EngineType>
    struct EngineTraits;

    template <>
    struct EngineTraits<HFA::HFA> {
        using ProviderType = dstools::InferenceModelProvider<HFA::HFA>;

        static bool isOpen(const HFA::HFA *engine) {
            return engine && engine->isOpen();
        }
    };

    template <>
    struct EngineTraits<Rmvpe::Rmvpe> {
        using ProviderType = dstools::InferenceModelProvider<Rmvpe::Rmvpe>;

        static bool isOpen(const Rmvpe::Rmvpe *engine) {
            return engine && engine->is_open();
        }
    };

    template <>
    struct EngineTraits<Game::Game> {
        using ProviderType = dstools::InferenceModelProvider<Game::Game>;

        static bool isOpen(const Game::Game *engine) {
            return engine && engine->isOpen();
        }
    };

    template <>
    struct EngineTraits<LyricFA::Asr> {
        using ProviderType = FunAsrModelProvider;

        static bool isOpen(const LyricFA::Asr *engine) {
            return engine && engine->initialized();
        }
    };

} // namespace dstools

#include <dstools/ExecutionProvider.h>
#include <filesystem>
#include <optional>
#include <string>
#include <type_traits>

namespace InferBridge {

    template <typename Engine>
    struct ProbeResult {
        std::optional<Engine> engine;
        std::string error;
        bool success() const {
            return engine.has_value();
        }
    };

    template <typename Engine, typename Provider>
    [[nodiscard]] ProbeResult<Engine> probeRmvpeEngine(const std::filesystem::path &modelPath, Provider provider,
                                                       int deviceId) {
        static_assert(std::is_same_v<Engine, Rmvpe::Rmvpe>, "Engine must be Rmvpe::Rmvpe");
        Engine engine(modelPath, provider, deviceId);
        if (dstools::EngineTraits<Engine>::isOpen(&engine)) {
            return {std::move(engine), {}};
        }
        return {std::nullopt, "Failed to load RMVPE model"};
    }

    template <typename Engine, typename Provider>
    [[nodiscard]] ProbeResult<Engine> probeGameEngine(const std::filesystem::path &modelPath, Provider provider,
                                                      int deviceId) {
        static_assert(std::is_same_v<Engine, Game::Game>, "Engine must be Game::Game");
        Engine engine;
        auto result = engine.load(modelPath, provider, deviceId);
        if (result) {
            return {std::move(engine), {}};
        }
        return {std::nullopt, result.error()};
    }

} // namespace InferBridge
