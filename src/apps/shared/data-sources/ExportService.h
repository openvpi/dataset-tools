#pragma once

#include <QString>
#include <dsfw/infer/IInferenceService.h>
#include <dstools/Constants.h>
#include <dstools/DsTextTypes.h>
#include <dstools/TranscriptionCsv.h>
#include <vector>

namespace dstools {

    class IEditorDataSource;
    class PhNumCalculator;

    struct ExportValidationResult {
        int readyForCsv = 0;
        int readyForDs = 0;
        int dirtyCount = 0;
        int totalSlices = 0;
        int missingFa = 0;
        int missingPitch = 0;
        int missingMidi = 0;
        int missingPhNum = 0;

        int readySlices = 0;
        int missingLayerSlices = 0;
        int dirtySlices = 0;

        // Per-column coverage counts (order: name, ph_seq, ph_dur, ph_num, note_seq, note_dur, note_slur)
        int colCoverage[7] = {};
    };

    struct ExportOptions {
        QString outputDir;
        bool exportCsv = true;
        bool exportDs = true;
        bool exportWavs = true;
        bool autoComplete = true;
        int sampleRate = constants::kDefaultSampleRate;
        int hopSize = constants::kDefaultHopSize;
        bool includeDiscarded = false;
    };

    struct ExportResult {
        int exportedCount = 0;
        int errorCount = 0;
    };

    class ExportService {
    public:
        static ExportValidationResult validate(IEditorDataSource *source);

        static void autoCompleteSlice(IEditorDataSource *source, const QString &sliceId,
                                      dsfw::infer::IInferenceService *inferService, PhNumCalculator *phNumCalc);

        static ExportResult exportDataset(IEditorDataSource *source, const ExportOptions &options,
                                          dsfw::infer::IInferenceService *inferService, PhNumCalculator *phNumCalc);
    };

} // namespace dstools
