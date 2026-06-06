#pragma once

#include <QString>
#include <QStringList>
#include <dsfw/signal/music_math.h>

namespace dstools {

    inline double freqToMidi(double freq) {
        return dsfw::signal::freqToMidi(freq);
    }

    inline double midiToFreq(double midi) {
        return dsfw::signal::midiToFreq(midi);
    }

    struct NotePitch {
        QString name;
        int octave = 0;
        int cents = 0;
        double midiNumber = 0.0;
        bool valid = false;
    };

    NotePitch parseNoteName(const QString &noteStr);

    QString midiToNoteName(int midiNote);

    QString midiToNoteString(double midiFloat);

    QString shiftNoteCents(const QString &noteStr, int deltaCents);

} // namespace dstools