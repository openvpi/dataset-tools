#pragma once

#include <string>
#include <vector>

#include "GameGlobal.h"
#include "WordParser.h"

namespace Game
{

    struct AlignedNote {
        std::string name; // Note name (e.g. "C4+10") or "rest"
        float duration;   // Duration in seconds
        int slur;         // 0 = non-slur, 1 = slur (continuation of previous word's note)
    };

    /**
     * Align predicted notes to word boundaries.
     * Port of Python inference/utils.py align_notes_to_words().
     *
     * @param words Word durations and v/uv flags
     * @param noteSeq Predicted note names (from model output, already converted via midiToNoteName)
     * @param noteDur Predicted note durations in seconds
     * @param tol Alignment tolerance in seconds (default 0.01)
     * @param applyWordUv If true, force unvoiced words to "rest" (used when uv_note_cond == "follow")
     * @return Aligned notes with slur flags
     */
    GAME_INFER_EXPORT std::vector<AlignedNote> alignNotesToWords(const std::vector<WordInfo> &words,
                                                                  const std::vector<std::string> &noteSeq,
                                                                  const std::vector<float> &noteDur,
                                                                  float tol = 0.01f, bool applyWordUv = false);

} // namespace Game
