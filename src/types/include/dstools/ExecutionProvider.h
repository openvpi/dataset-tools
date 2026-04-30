#pragma once

namespace dstools::infer {

/// Inference execution provider. Unified replacement for the 4 separate enums:
///   Game::ExecutionProvider, Some::ExecutionProvider,
///   Rmvpe::ExecutionProvider, HFA Provider.h
enum class ExecutionProvider {
    CPU = 0,
    DML = 1,
    CUDA = 2
};

} // namespace dstools::infer
