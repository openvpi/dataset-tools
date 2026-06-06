#pragma once

namespace dsfw::infer {

    /// Inference execution provider. Unified replacement for the 4 separate enums:
    ///   Game::ExecutionProvider, Some::ExecutionProvider,
    ///   Rmvpe::ExecutionProvider, HFA Provider.h
    enum class ExecutionProvider { CPU = 0, DML = 1, CUDA = 2 };

} // namespace dsfw::infer

// Backward compatibility
namespace dstools::infer {
    using dsfw::infer::ExecutionProvider;
}
