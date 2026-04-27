#pragma once
#include <dstools/AppSettings.h>

/// Pipeline settings key schema -- all persisted keys in one place.
namespace PipelineKeys {
    inline const dstools::SettingsKey<int> LastTab("General/lastTab", 0);
}
