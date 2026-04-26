#pragma once

namespace dstools::keys {

namespace layers {
    constexpr const char *pitch    = "pitch";
    constexpr const char *midi     = "midi";
    constexpr const char *phoneme  = "phoneme";
    constexpr const char *grapheme = "grapheme";
    constexpr const char *phNum    = "ph_num";
    constexpr const char *note     = "note";
} // namespace layers

namespace engines {
    constexpr const char *pitchExtraction   = "pitch_extraction";
    constexpr const char *midiTranscription = "midi_transcription";
    constexpr const char *phonemeAlignment  = "phoneme_alignment";
    constexpr const char *lyricAlignment    = "lyric_alignment";
    constexpr const char *moeR3             = "moe_r3";
} // namespace engines

namespace csv {
    constexpr const char *name     = "name";
    constexpr const char *phSeq    = "ph_seq";
    constexpr const char *phDur    = "ph_dur";
    constexpr const char *phNum    = "ph_num";
    constexpr const char *noteSeq  = "note_seq";
    constexpr const char *noteDur  = "note_dur";
    constexpr const char *wordSpan = "word_span";
    constexpr const char *dirty    = "dirty";
    constexpr const char *startSec = "start_sec";
    constexpr const char *endSec   = "end_sec";
} // namespace csv

namespace steps {
    constexpr const char *minLabel = "min_label";
} // namespace steps

} // namespace dstools::keys