#pragma once

/// @file ModelProviderInit.h
/// @brief Registers all inference model providers with the ModelManager.

namespace dstools {

class IModelManager;

void registerModelProviders(IModelManager &mm);

} // namespace dstools
