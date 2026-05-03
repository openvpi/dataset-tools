#pragma once

#include <dstools/DsTextTypes.h>
#include <dstools/TranscriptionCsv.h>

#include <QString>
#include <vector>

namespace HFA {
class HFA;
}

namespace Rmvpe {
class Rmvpe;
}

namespace Game {
class Game;
struct GameNote;
}

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
};

struct ExportOptions {
    QString outputDir;
    bool exportCsv = true;
    bool exportDs = true;
    bool exportWavs = true;
    bool autoComplete = true;
    int sampleRate = 44100;
    int hopSize = 512;
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
                                   HFA::HFA *hfa, Rmvpe::Rmvpe *rmvpe,
                                   Game::Game *game, PhNumCalculator *phNumCalc);

    static ExportResult exportDataset(IEditorDataSource *source, const ExportOptions &options,
                                       HFA::HFA *hfa, Rmvpe::Rmvpe *rmvpe,
                                       Game::Game *game, PhNumCalculator *phNumCalc);
};

} // namespace dstools
