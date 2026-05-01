#pragma once

#include <dsfw/ISlicerService.h>

class SndfileHandle;

class SlicerService : public dstools::ISlicerService {
public:
    SlicerService() = default;
    ~SlicerService() override = default;

    dstools::Result<dstools::SliceResult> slice(const QString &audioPath, double threshold, int minLength,
                                                int minInterval, int hopSize, int maxSilKept) override;
};
