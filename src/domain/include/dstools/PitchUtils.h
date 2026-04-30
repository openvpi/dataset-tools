#pragma once

#include <QString>
#include <QStringList>

namespace dstools {

    /// Parsed representation of a DS note string (e.g. "C#4+12")
    struct NotePitch {
        QString name;            ///< Note name, e.g. "C#"
        int octave = 0;          ///< Octave, e.g. 4
        int cents = 0;           ///< Cents offset, always in [-50, +50]
        double midiNumber = 0.0; ///< Full precision MIDI number
        bool valid = false;      ///< Whether parsing succeeded
    };

    /// Convert frequency in Hz to MIDI note number.
    /// Returns 0.0 for freq <= 0.
    double freqToMidi(double freq);

    /// Convert MIDI note number to frequency in Hz.
    double midiToFreq(double midi);

    /// Parse a DS note string like "C#4+12", "Bb3", "rest" into NotePitch.
    /// Returns invalid NotePitch for "rest" or unparseable strings.
    NotePitch parseNoteName(const QString &noteStr);

    /// Convert integer MIDI note number to display name (e.g. 69 → "A4").
    /// Returns empty string for out-of-range (< 21 or > 108).
    QString midiToNoteName(int midiNote);

    /// Convert a fractional MIDI number to a DS note string with auto carry-over.
    /// e.g. 60.5 → "C4+50", 61.12 → "C#4+12"
    QString midiToNoteString(double midiFloat);

    /// Shift a note name string by deltaCents, with auto carry-over at ±50.
    /// e.g. shiftNoteCents("C4+48", 5) → "C#4+3"
    QString shiftNoteCents(const QString &noteStr, int deltaCents);

} // namespace dstools
