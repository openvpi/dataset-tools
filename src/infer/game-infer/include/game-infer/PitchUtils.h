#pragma once

#include <string>

#include "GameGlobal.h"

namespace Game
{
    /**
     * Convert a MIDI pitch number to a note name string.
     * Examples: 69.0 -> "A4", 69.5 -> "A4+50", 60.0 -> "C4", 59.7 -> "B3-30"
     * @param midiPitch Floating-point MIDI pitch (A4 = 69.0)
     * @param cents If true, append cents deviation (e.g. "+50", "-30"). If false, round to nearest note.
     * @return Note name string
     */
    GAME_INFER_EXPORT std::string midiToNoteName(float midiPitch, bool cents = true);

    /**
     * Convert a note name string to a MIDI pitch number.
     * Accepts formats: "C4", "C#4", "Db4", "A4+50", "B3-30"
     * @param name Note name string
     * @return Floating-point MIDI pitch
     */
    GAME_INFER_EXPORT float noteNameToMidi(const std::string &name);

} // namespace Game
