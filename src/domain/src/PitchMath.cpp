#include <dstools/CurveTools.h>
#include <dstools/PitchUtils.h>

#include <dsfw/signal/music_math.h>

#include <QRegularExpression>

#include <algorithm>
#include <cmath>

namespace dstools {

namespace {
    // MIDI音符范围常量
    constexpr int kMidiNoteMin = 21;   // A0 - 钢琴最低音
    constexpr int kMidiNoteMax = 108;  // C8 - 钢琴最高音
    constexpr int kMidiNoteOffset = 12; // MIDI到音符名的偏移
}

// ─── PitchUtils ───────────────────────────────────────────────────────────────

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

NotePitch parseNoteName(const QString &noteStr) {
    NotePitch result;
    QString s = noteStr.trimmed();
    if (s.isEmpty() || s.compare("rest", Qt::CaseInsensitive) == 0) {
        return result;
    }

    static QRegularExpression re(R"(^([A-G][#b]*)(-?\d+)([+-]\d+)?$)");
    auto match = re.match(s);
    if (!match.hasMatch()) {
        return result;
    }

    QString namePart = match.captured(1);
    int octave = match.captured(2).toInt();
    int cents = match.captured(3).isEmpty() ? 0 : match.captured(3).toInt();

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

    result.name = QString::fromUtf8(dsfw::signal::semitoneName(semitones));
    result.octave = octave;
    result.cents = cents;
    result.midiNumber = (octave + 1) * 12 + semitones + cents / 100.0;
    result.valid = true;
    return result;
}

QString midiToNoteName(int midiNote) {
    if (midiNote > kMidiNoteMax || midiNote < kMidiNoteMin)
        return QString();
    midiNote -= kMidiNoteOffset;
    return QString("%1%2")
        .arg(QLatin1String(dsfw::signal::semitoneName(midiNote % 12)))
        .arg(midiNote / 12);
}

QString midiToNoteString(double midiFloat) {
    int midiInt = static_cast<int>(std::round(midiFloat));
    int cents = static_cast<int>(std::round((midiFloat - midiInt) * 100));
    if (cents > 50) { midiInt++; cents -= 100; }
    else if (cents < -50) { midiInt--; cents += 100; }
    if (midiInt < 0) { midiInt = 0; cents = 0; }
    int noteIdx = midiInt % 12;
    int octave = midiInt / 12 - 1;
    if (cents == 0)
        return QString("%1%2").arg(QLatin1String(dsfw::signal::semitoneName(noteIdx))).arg(octave);
    return QString("%1%2%3%4")
        .arg(QLatin1String(dsfw::signal::semitoneName(noteIdx)))
        .arg(octave)
        .arg(cents > 0 ? "+" : "")
        .arg(cents);
}

QString shiftNoteCents(const QString &noteStr, int deltaCents) {
    auto pitch = parseNoteName(noteStr);
    if (!pitch.valid) return noteStr;
    double newMidi = pitch.midiNumber + deltaCents / 100.0;
    return midiToNoteString(newMidi);
}

} // namespace dstools