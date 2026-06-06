#pragma once

#include <dstools/DsTextTypes.h>
#include <dsfw/TimePos.h>

#include <QString>
#include <string>
#include <variant>
#include <vector>

namespace dstools {

    struct MidiNote {
        int pitch = 0;
        double onset = 0.0;
        double duration = 0.0;
        bool voiced = false;
    };

    struct SliceBoundary {
        int id = 0;
        TimePos pos = 0;
        TimePos endPos = 0;
        QString text;
    };

    struct TranscriptionEntry {
        QString name;
        QString phSeq;
        QString phDur;
        QString phNum;
        QString noteSeq;
        QString noteDur;
        QString wordSpan;
        double startSec = 0.0;
        double endSec = 0.0;
        bool dirty = false;
    };

    using TextBoundaryLayer = std::vector<Boundary>;
    using PhonemeBoundaryLayer = std::vector<Boundary>;
    using PhonemeNumberLayer = std::vector<int>;
    using PitchCurveLayer = CurveLayer;
    using SliceBoundaryLayer = std::vector<SliceBoundary>;
    using MidiNoteLayer = std::vector<MidiNote>;
    using TranscriptionLayer = std::vector<TranscriptionEntry>;
    using StringLayer = std::string;

    using LayerDataVariant = std::variant<
        TextBoundaryLayer,
        PhonemeNumberLayer,
        PitchCurveLayer,
        SliceBoundaryLayer,
        MidiNoteLayer,
        TranscriptionLayer,
        StringLayer
    >;

} // namespace dstools