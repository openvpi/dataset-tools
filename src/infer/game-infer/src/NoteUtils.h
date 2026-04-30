#ifndef GAME_INFER_NOTEUTILS_H
#define GAME_INFER_NOTEUTILS_H

#include <array>
#include <cmath>
#include <string>

namespace game_infer {

    inline std::string midiToNoteString(double midiFloat) {
        int midiInt = static_cast<int>(std::round(midiFloat));
        int cents = static_cast<int>(std::round((midiFloat - midiInt) * 100));

        if (cents > 50) {
            midiInt++;
            cents -= 100;
        } else if (cents < -50) {
            midiInt--;
            cents += 100;
        }

        if (midiInt < 0) {
            midiInt = 0;
            cents = 0;
        }

        static constexpr std::array<const char *, 12> NoteNames = {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

        const int noteIdx = midiInt % 12;
        const int octave = midiInt / 12 - 1;

        std::string result = std::string(NoteNames[noteIdx]) + std::to_string(octave);
        if (cents > 0)
            result += "+" + std::to_string(cents);
        else if (cents < 0)
            result += std::to_string(cents);

        return result;
    }

} // namespace game_infer

#endif // GAME_INFER_NOTEUTILS_H
