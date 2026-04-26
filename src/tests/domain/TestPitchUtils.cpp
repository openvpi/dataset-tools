#include <QTest>
#include <dstools/PitchUtils.h>

#include <cmath>

using namespace dstools;

class TestPitchUtils : public QObject {
    Q_OBJECT
private slots:
    void testFreqToMidi_A4();
    void testFreqToMidi_zeroReturnsZero();
    void testMidiToFreq_A4();
    void testFreqMidiRoundTrip();
    void testParseNoteName_basic();
    void testParseNoteName_withCents();
    void testParseNoteName_rest();
    void testParseNoteName_invalid();
    void testParseNoteName_flat();
    void testMidiToNoteName_A4();
    void testMidiToNoteName_outOfRange();
    void testMidiToNoteString_noCents();
    void testMidiToNoteString_withCents();
    void testShiftNoteCents_noCarry();
    void testShiftNoteCents_carry();
};

void TestPitchUtils::testFreqToMidi_A4() {
    double midi = freqToMidi(440.0);
    QVERIFY(qFuzzyCompare(midi, 69.0));
}

void TestPitchUtils::testFreqToMidi_zeroReturnsZero() {
    QCOMPARE(freqToMidi(0.0), 0.0);
    QCOMPARE(freqToMidi(-100.0), 0.0);
}

void TestPitchUtils::testMidiToFreq_A4() {
    double freq = midiToFreq(69.0);
    QVERIFY(qFuzzyCompare(freq, 440.0));
}

void TestPitchUtils::testFreqMidiRoundTrip() {
    // C4 = MIDI 60 = 261.626 Hz
    double freq = midiToFreq(60.0);
    double midi = freqToMidi(freq);
    QVERIFY(std::abs(midi - 60.0) < 1e-9);
}

void TestPitchUtils::testParseNoteName_basic() {
    auto p = parseNoteName("C4");
    QVERIFY(p.valid);
    QCOMPARE(p.name, QString("C"));
    QCOMPARE(p.octave, 4);
    QCOMPARE(p.cents, 0);
    QVERIFY(qFuzzyCompare(p.midiNumber, 60.0));
}

void TestPitchUtils::testParseNoteName_withCents() {
    auto p = parseNoteName("C#4+12");
    QVERIFY(p.valid);
    QCOMPARE(p.name, QString("C#"));
    QCOMPARE(p.octave, 4);
    QCOMPARE(p.cents, 12);
    QVERIFY(std::abs(p.midiNumber - 61.12) < 1e-9);
}

void TestPitchUtils::testParseNoteName_rest() {
    auto p = parseNoteName("rest");
    QVERIFY(!p.valid);
}

void TestPitchUtils::testParseNoteName_invalid() {
    auto p = parseNoteName("XYZ");
    QVERIFY(!p.valid);
}

void TestPitchUtils::testParseNoteName_flat() {
    auto p = parseNoteName("Bb3");
    QVERIFY(p.valid);
    QCOMPARE(p.name, QString("A#"));  // Bb normalized to A#
    QCOMPARE(p.octave, 3);
    // Bb3 = A#3 = MIDI 58
    QVERIFY(qFuzzyCompare(p.midiNumber, 58.0));
}

void TestPitchUtils::testMidiToNoteName_A4() {
    QCOMPARE(midiToNoteName(69), QString("A4"));
}

void TestPitchUtils::testMidiToNoteName_outOfRange() {
    QCOMPARE(midiToNoteName(20), QString());
    QCOMPARE(midiToNoteName(109), QString());
}

void TestPitchUtils::testMidiToNoteString_noCents() {
    // MIDI 60 = C4
    QCOMPARE(midiToNoteString(60.0), QString("C4"));
}

void TestPitchUtils::testMidiToNoteString_withCents() {
    // MIDI 61.12 → C#4+12
    QString result = midiToNoteString(61.12);
    QCOMPARE(result, QString("C#4+12"));
}

void TestPitchUtils::testShiftNoteCents_noCarry() {
    // C4+10, shift +5 → C4+15
    QString result = shiftNoteCents("C4+10", 5);
    QCOMPARE(result, QString("C4+15"));
}

void TestPitchUtils::testShiftNoteCents_carry() {
    // C4+48: midiNumber = 60.48, shift +5 cents → newMidi = 60.48 + 0.05 = 60.53
    // midiToNoteString(60.53): midiInt=round(60.53)=61, cents=round((60.53-61)*100)=-47
    // Result: C#4-47
    QString result = shiftNoteCents("C4+48", 5);
    QCOMPARE(result, QString("C#4-47"));
}

QTEST_GUILESS_MAIN(TestPitchUtils)
#include "TestPitchUtils.moc"
