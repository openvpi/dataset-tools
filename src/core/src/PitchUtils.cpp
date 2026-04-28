#include <dstools/PitchUtils.h>

#include <QRegularExpression>

#include <cmath>

namespace dstools {

static const QStringList NoteNames = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

// Semitones from C for each base note letter
static int baseSemitones(QChar c) {
    switch (c.toUpper().toLatin1()) {
        case 'C': return 0;
        case 'D': return 2;
        case 'E': return 4;
        case 'F': return 5;
        case 'G': return 7;
        case 'A': return 9;
        case 'B': return 11;
        default: return -1;
    }
}

double freqToMidi(double freq) {
    if (freq <= 0.0) return 0.0;
    return 69.0 + 12.0 * std::log2(freq / 440.0);
}

double midiToFreq(double midi) {
    return 440.0 * std::pow(2.0, (midi - 69.0) / 12.0);
}

NotePitch parseNoteName(const QString &noteStr) {
    NotePitch result;
    QString s = noteStr.trimmed();
    if (s.isEmpty() || s.compare("rest", Qt::CaseInsensitive) == 0) {
        return result;
    }

    // Regex: ([A-G][#b]*) (-?\d+) ([+-]\d+)?
    static QRegularExpression re(R"(^([A-G][#b]*)(-?\d+)([+-]\d+)?$)");
    auto match = re.match(s);
    if (!match.hasMatch()) {
        return result;
    }

    QString namePart = match.captured(1);  // e.g. "C#", "Bb", "C##"
    int octave = match.captured(2).toInt();
    int cents = match.captured(3).isEmpty() ? 0 : match.captured(3).toInt();

    // Resolve accidentals to standard sharp name
    QChar base = namePart[0];
    int semitones = baseSemitones(base);
    if (semitones < 0) return result;

    for (int i = 1; i < namePart.length(); ++i) {
        if (namePart[i] == '#') semitones++;
        else if (namePart[i] == 'b') semitones--;
    }

    int octaveOffset = 0;
    while (semitones >= 12) { semitones -= 12; octaveOffset++; }
    while (semitones < 0)   { semitones += 12; octaveOffset--; }
    octave += octaveOffset;

    result.name = NoteNames[semitones];
    result.octave = octave;
    result.cents = cents;
    result.midiNumber = (octave + 1) * 12 + semitones + cents / 100.0;
    result.valid = true;
    return result;
}

QString midiToNoteName(int midiNote) {
    static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    if (midiNote > 108 || midiNote < 21)
        return QString();
    midiNote -= 12;
    return QString("%1%2").arg(names[midiNote % 12]).arg(midiNote / 12);
}

QString midiToNoteString(double midiFloat) {
    int midiInt = static_cast<int>(std::round(midiFloat));
    int cents = static_cast<int>(std::round((midiFloat - midiInt) * 100));
    if (cents > 50) { midiInt++; cents -= 100; }
    else if (cents < -50) { midiInt--; cents += 100; }
    if (midiInt < 0) { midiInt = 0; cents = 0; }
    static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int noteIdx = midiInt % 12;
    int octave = midiInt / 12 - 1;
    if (cents == 0)
        return QString("%1%2").arg(names[noteIdx]).arg(octave);
    return QString("%1%2%3%4").arg(names[noteIdx]).arg(octave).arg(cents > 0 ? "+" : "").arg(cents);
}

QString shiftNoteCents(const QString &noteStr, int deltaCents) {
    auto pitch = parseNoteName(noteStr);
    if (!pitch.valid) return noteStr;
    double newMidi = pitch.midiNumber + deltaCents / 100.0;
    return midiToNoteString(newMidi);
}

} // namespace dstools
