#pragma once

#include <QString>
#include <dsfw/infer/IInferenceService.h>
#include <dstools/DsTextTypes.h>

namespace dstools {

    class PhNumCalculator;

    struct AutoCompleteResult {
        DsTextDocument doc;
        bool modified = false;
    };

    AutoCompleteResult autoCompleteSlice(DsTextDocument doc, const QString &audioPath,
                                         dsfw::infer::IInferenceService *inferService,
                                         PhNumCalculator *phNumCalc);

} // namespace dstools
