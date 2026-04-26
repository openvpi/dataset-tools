#pragma once

#include <QString>
#include <cstdint>
#include <dstools/DsTextTypes.h>
#include <vector>

namespace Rmvpe {
    class Rmvpe;
}

namespace Game {
    class Game;
    struct GameNote;
}

namespace dstools {

    struct PitchExtractionResult {
        std::vector<float> f0Mhz;
        float timestep = 0.005f;
    };

    struct MidiTranscriptionResult {
        std::vector<Game::GameNote> notes;
    };

    class PitchExtractionService {
    public:
        static PitchExtractionResult extractPitch(Rmvpe::Rmvpe *rmvpe, const QString &audioPath);

        static MidiTranscriptionResult transcribeMidi(Game::Game *game, const QString &audioPath);

        static void applyPitchToDocument(DsTextDocument &doc, const std::vector<float> &f0Mhz, float timestep);

        static void applyMidiToDocument(DsTextDocument &doc, const std::vector<Game::GameNote> &notes);
    };

} // namespace dstools
