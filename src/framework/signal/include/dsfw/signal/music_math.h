#pragma once

#include <cmath>
#include <cstdint>

namespace dsfw::signal {

    constexpr const char *kNoteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    constexpr const char *semitoneName(int semitone) {
        semitone %= 12;
        if (semitone < 0)
            semitone += 12;
        return kNoteNames[semitone];
    }

    constexpr int semitoneIndex(int midiNote) {
        return midiNote % 12;
    }

    constexpr int midiOctave(int midiNote) {
        return midiNote / 12 - 1;
    }

    constexpr int midiNote(int semitone, int octave) {
        return (octave + 1) * 12 + semitone;
    }

    inline double freqToMidi(double freq) {
        if (freq <= 0.0)
            return 0.0;
        return 69.0 + 12.0 * std::log2(freq / 440.0);
    }

    inline double midiToFreq(double midi) {
        return 440.0 * std::pow(2.0, (midi - 69.0) / 12.0);
    }

} // namespace dsfw::signal