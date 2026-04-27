#include <game-infer/PitchUtils.h>

#include <array>
#include <cmath>
#include <stdexcept>

namespace Game
{
    // Note names in chromatic order (sharps)
    static const std::array<const char *, 12> kNoteNames = {"C",  "C#", "D",  "D#", "E",  "F",
                                                            "F#", "G",  "G#", "A",  "A#", "B"};

    std::string midiToNoteName(const float midiPitch, const bool cents) {
        const int rounded = static_cast<int>(std::round(midiPitch));
        const int centsDeviation = static_cast<int>(std::round((midiPitch - rounded) * 100.0f));

        // MIDI note 0 = C-1, note 12 = C0, note 60 = C4
        const int octave = (rounded / 12) - 1;
        const int noteIndex = ((rounded % 12) + 12) % 12; // handle negative values

        std::string result = std::string(kNoteNames[noteIndex]) + std::to_string(octave);

        if (cents && centsDeviation != 0) {
            if (centsDeviation > 0) {
                result += "+" + std::to_string(centsDeviation);
            } else {
                result += std::to_string(centsDeviation); // negative sign included
            }
        }

        return result;
    }

    float noteNameToMidi(const std::string &name) {
        if (name.empty()) {
            throw std::invalid_argument("Empty note name");
        }

        size_t pos = 0;

        // Parse note letter
        char letter = name[pos++];
        if (letter < 'A' || letter > 'G') {
            throw std::invalid_argument("Invalid note letter: " + name);
        }

        // Map letter to semitone offset from C
        static const int letterToSemitone[] = {
            // A  B  C  D  E  F  G
            9, 11, 0, 2, 4, 5, 7};
        int semitone = letterToSemitone[letter - 'A'];

        // Parse optional sharp/flat
        if (pos < name.size()) {
            if (name[pos] == '#') {
                semitone++;
                pos++;
            } else if (name[pos] == 'b') {
                semitone--;
                pos++;
            }
        }

        // Parse octave (may be negative)
        bool negativeOctave = false;
        if (pos < name.size() && name[pos] == '-') {
            negativeOctave = true;
            pos++;
        }

        int octave = 0;
        bool hasOctave = false;
        while (pos < name.size() && name[pos] >= '0' && name[pos] <= '9') {
            octave = octave * 10 + (name[pos] - '0');
            hasOctave = true;
            pos++;
        }
        if (!hasOctave) {
            throw std::invalid_argument("Missing octave in note name: " + name);
        }
        if (negativeOctave)
            octave = -octave;

        // MIDI = (octave + 1) * 12 + semitone
        float midi = static_cast<float>((octave + 1) * 12 + semitone);

        // Parse optional cents deviation (+50, -30)
        if (pos < name.size()) {
            if (name[pos] == '+' || name[pos] == '-') {
                const bool negative = (name[pos] == '-');
                pos++;
                int centsVal = 0;
                while (pos < name.size() && name[pos] >= '0' && name[pos] <= '9') {
                    centsVal = centsVal * 10 + (name[pos] - '0');
                    pos++;
                }
                if (negative)
                    centsVal = -centsVal;
                midi += static_cast<float>(centsVal) / 100.0f;
            }
        }

        return midi;
    }

} // namespace Game
