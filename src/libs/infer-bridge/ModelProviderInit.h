#pragma once

/// @file ModelProviderInit.h
/// @brief Registers all inference model providers with the ModelManager.

namespace dstools {

    class ModelManager;

    void registerModelProviders(ModelManager &mm);

} // namespace dstools