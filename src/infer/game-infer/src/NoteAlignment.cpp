#include <game-infer/NoteAlignment.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace Game
{

    // Compute cumulative sum with leading zero: [0, d0, d0+d1, ...]
    static std::vector<double> cumulativeSumWithZero(const std::vector<float> &durations) {
        std::vector<double> result(durations.size() + 1, 0.0);
        for (size_t i = 0; i < durations.size(); ++i) {
            result[i + 1] = result[i] + static_cast<double>(durations[i]);
        }
        return result;
    }

    // Find index of element closest to target in a sorted array
    static size_t argminAbs(const std::vector<double> &arr, const size_t start, const size_t end, const double target) {
        size_t bestIdx = start;
        double bestDist = std::abs(arr[start] - target);
        for (size_t i = start + 1; i < end; ++i) {
            const double dist = std::abs(arr[i] - target);
            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = i;
            }
        }
        return bestIdx;
    }

    std::vector<AlignedNote> alignNotesToWords(const std::vector<WordInfo> &words,
                                                const std::vector<std::string> &noteSeq,
                                                const std::vector<float> &noteDur, const float tol,
                                                const bool applyWordUv) {
        if (noteSeq.size() != noteDur.size()) {
            throw std::invalid_argument("noteSeq and noteDur must have the same length");
        }

        // Compute word start/end times
        std::vector<float> wordDurVec;
        wordDurVec.reserve(words.size());
        for (const auto &w : words)
            wordDurVec.push_back(w.duration);

        const auto wordCumsum = cumulativeSumWithZero(wordDurVec);
        // wordCumsum[i] = start of word i, wordCumsum[i+1] = end of word i

        // Compute note start/end times
        const auto noteCumsum = cumulativeSumWithZero(noteDur);
        // noteCumsum[i] = start of note i, noteCumsum[i+1] = end of note i

        const size_t numNotes = noteSeq.size();

        std::vector<AlignedNote> result;

        for (size_t wordIdx = 0; wordIdx < words.size(); ++wordIdx) {
            const double wordStart = wordCumsum[wordIdx];
            const double wordEnd = wordCumsum[wordIdx + 1];

            if (applyWordUv && !words[wordIdx].voiced) {
                // Unvoiced word → single rest note
                result.push_back(AlignedNote{"rest", static_cast<float>(wordEnd - wordStart), 0});
                continue;
            }

            // Find closest note start to word start
            // note_start array = noteCumsum[0..numNotes-1]
            size_t noteStartIdx = argminAbs(noteCumsum, 0, numNotes, wordStart);
            if (wordStart < noteCumsum[noteStartIdx] - tol && noteStartIdx > 0) {
                noteStartIdx--;
            }

            // Find closest note end to word end (searching from noteStartIdx)
            // note_end array = noteCumsum[1..numNotes]
            // We search noteCumsum[noteStartIdx+1 .. numNotes]
            size_t noteEndIdx = noteStartIdx;
            {
                double bestDist = std::abs(noteCumsum[noteStartIdx + 1] - wordEnd);
                for (size_t i = noteStartIdx + 1; i < numNotes; ++i) {
                    const double dist = std::abs(noteCumsum[i + 1] - wordEnd);
                    if (dist < bestDist) {
                        bestDist = dist;
                        noteEndIdx = i;
                    }
                }
            }
            if (wordEnd > noteCumsum[noteEndIdx + 1] + tol && noteEndIdx + 1 < numNotes) {
                noteEndIdx++;
            }

            // Build aligned notes for this word
            std::vector<std::string> wordNoteSeq;
            std::vector<float> wordNoteDur;

            for (size_t ni = noteStartIdx; ni <= noteEndIdx; ++ni) {
                double start = (ni == noteStartIdx) ? wordStart : noteCumsum[ni];
                double end = (ni == noteEndIdx) ? wordEnd : noteCumsum[ni + 1];
                const float segDur = static_cast<float>(end - start);

                if (!wordNoteSeq.empty() && wordNoteSeq.back() == noteSeq[ni]) {
                    // Same note as previous → merge durations
                    wordNoteDur.back() += segDur;
                } else {
                    wordNoteSeq.push_back(noteSeq[ni]);
                    wordNoteDur.push_back(segDur);
                }
            }

            // Append to result with slur flags: first note = 0, rest = 1
            for (size_t i = 0; i < wordNoteSeq.size(); ++i) {
                result.push_back(AlignedNote{wordNoteSeq[i], wordNoteDur[i], (i == 0) ? 0 : 1});
            }
        }

        return result;
    }

} // namespace Game
