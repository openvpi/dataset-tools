#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "GameGlobal.h"
#include "NoteAlignment.h"

namespace Game
{

    /**
     * Represents one item in a DiffSinger transcription CSV.
     * Core fields: name, ph_seq, ph_dur, ph_num.
     * All other columns are preserved in 'fields' for round-trip fidelity.
     */
    struct DiffSingerItem {
        std::string name;
        std::filesystem::path wavPath; // Resolved path to waveform file

        std::vector<std::string> phSeq;
        std::vector<float> phDur;
        std::vector<int> phNum;

        // All CSV fields (including name, ph_seq, ph_dur, ph_num) for round-trip
        std::map<std::string, std::string> fields;
        // Ordered column names from original CSV header
        std::vector<std::string> columnOrder;
    };

    /**
     * Parse a DiffSinger transcription CSV file.
     * Expects columns: name, ph_seq, ph_dur, ph_num.
     * Resolves wav paths relative to CSV parent directory / wavs / {name}.{ext}
     *
     * @param csvPath Path to the CSV file
     * @param audioExtensions Audio file extensions to try (default: .wav, .flac)
     * @return Vector of parsed items
     */
    GAME_INFER_EXPORT std::vector<DiffSingerItem>
    parseDiffSingerCSV(const std::filesystem::path &csvPath,
                       const std::vector<std::string> &audioExtensions = {".wav", ".flac"});

    /**
     * Write updated DiffSinger transcription CSV with alignment results.
     * Injects/replaces: note_seq, note_dur, note_slur.
     * Removes: note_glide (if present).
     * Preserves all other columns from original items.
     *
     * @param csvPath Output CSV path
     * @param items Original parsed items (provides column order and extra fields)
     * @param alignResults Alignment results per item (must be same length as items)
     */
    GAME_INFER_EXPORT void writeDiffSingerCSV(const std::filesystem::path &csvPath,
                                               const std::vector<DiffSingerItem> &items,
                                               const std::vector<std::vector<AlignedNote>> &alignResults);

} // namespace Game
