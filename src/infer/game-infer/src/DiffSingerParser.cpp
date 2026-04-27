#include <game-infer/DiffSingerParser.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Game
{

    // Simple CSV line parser that handles quoted fields
    static std::vector<std::string> parseCSVLine(const std::string &line) {
        std::vector<std::string> fields;
        std::string field;
        bool inQuotes = false;

        for (size_t i = 0; i < line.size(); ++i) {
            const char c = line[i];
            if (inQuotes) {
                if (c == '"') {
                    if (i + 1 < line.size() && line[i + 1] == '"') {
                        field += '"';
                        ++i; // skip escaped quote
                    } else {
                        inQuotes = false;
                    }
                } else {
                    field += c;
                }
            } else {
                if (c == '"') {
                    inQuotes = true;
                } else if (c == ',') {
                    fields.push_back(field);
                    field.clear();
                } else {
                    field += c;
                }
            }
        }
        fields.push_back(field);
        return fields;
    }

    // Quote a CSV field if it contains commas, quotes, or newlines
    static std::string quoteCSVField(const std::string &field) {
        if (field.find_first_of(",\"\n\r") != std::string::npos) {
            std::string quoted = "\"";
            for (const char c : field) {
                if (c == '"')
                    quoted += "\"\"";
                else
                    quoted += c;
            }
            quoted += "\"";
            return quoted;
        }
        return field;
    }

    static std::vector<std::string> splitWhitespace(const std::string &s) {
        std::vector<std::string> tokens;
        std::istringstream iss(s);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    static std::vector<float> splitFloats(const std::string &s) {
        std::vector<float> values;
        std::istringstream iss(s);
        std::string token;
        while (iss >> token) {
            values.push_back(std::stof(token));
        }
        return values;
    }

    static std::vector<int> splitInts(const std::string &s) {
        std::vector<int> values;
        std::istringstream iss(s);
        std::string token;
        while (iss >> token) {
            values.push_back(std::stoi(token));
        }
        return values;
    }

    std::vector<DiffSingerItem> parseDiffSingerCSV(const std::filesystem::path &csvPath,
                                                    const std::vector<std::string> &audioExtensions) {
        std::ifstream file(csvPath, std::ios::in);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open CSV file: " + csvPath.string());
        }

        // Read header
        std::string headerLine;
        if (!std::getline(file, headerLine)) {
            throw std::runtime_error("Empty CSV file: " + csvPath.string());
        }
        // Strip BOM if present
        if (headerLine.size() >= 3 && headerLine[0] == '\xEF' && headerLine[1] == '\xBB' && headerLine[2] == '\xBF') {
            headerLine = headerLine.substr(3);
        }
        // Strip trailing \r
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }

        const auto headers = parseCSVLine(headerLine);

        // Find required column indices
        int nameIdx = -1, phSeqIdx = -1, phDurIdx = -1, phNumIdx = -1;
        for (size_t i = 0; i < headers.size(); ++i) {
            if (headers[i] == "name")
                nameIdx = static_cast<int>(i);
            else if (headers[i] == "ph_seq")
                phSeqIdx = static_cast<int>(i);
            else if (headers[i] == "ph_dur")
                phDurIdx = static_cast<int>(i);
            else if (headers[i] == "ph_num")
                phNumIdx = static_cast<int>(i);
        }

        if (nameIdx < 0)
            throw std::runtime_error("CSV missing 'name' column: " + csvPath.string());
        if (phSeqIdx < 0)
            throw std::runtime_error("CSV missing 'ph_seq' column: " + csvPath.string());
        if (phDurIdx < 0)
            throw std::runtime_error("CSV missing 'ph_dur' column: " + csvPath.string());
        if (phNumIdx < 0)
            throw std::runtime_error("CSV missing 'ph_num' column: " + csvPath.string());

        const auto parentDir = csvPath.parent_path();

        std::vector<DiffSingerItem> items;
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.empty())
                continue;

            const auto values = parseCSVLine(line);

            DiffSingerItem item;
            item.columnOrder = headers;

            // Store all fields
            for (size_t i = 0; i < headers.size() && i < values.size(); ++i) {
                item.fields[headers[i]] = values[i];
            }

            item.name = values[nameIdx];
            item.phSeq = splitWhitespace(values[phSeqIdx]);
            item.phDur = splitFloats(values[phDurIdx]);
            item.phNum = splitInts(values[phNumIdx]);

            // Resolve wav path
            bool found = false;
            for (const auto &ext : audioExtensions) {
                auto candidate = parentDir / "wavs" / (item.name + ext);
                if (std::filesystem::is_regular_file(candidate)) {
                    item.wavPath = candidate;
                    found = true;
                    break;
                }
            }
            if (!found) {
                throw std::runtime_error("Waveform file not found for item '" + item.name + "' in " +
                                         csvPath.string());
            }

            items.push_back(std::move(item));
        }

        if (items.empty()) {
            throw std::runtime_error("No items found in CSV: " + csvPath.string());
        }

        return items;
    }

    void writeDiffSingerCSV(const std::filesystem::path &csvPath, const std::vector<DiffSingerItem> &items,
                            const std::vector<std::vector<AlignedNote>> &alignResults) {
        if (items.size() != alignResults.size()) {
            throw std::invalid_argument("items and alignResults must have the same length");
        }

        // Build column order: start from original, ensure note_seq/note_dur/note_slur present, remove note_glide
        std::vector<std::string> columns;
        if (!items.empty()) {
            columns = items[0].columnOrder;
        }

        // Remove note_glide
        columns.erase(std::remove(columns.begin(), columns.end(), "note_glide"), columns.end());

        // Ensure note_seq, note_dur, note_slur are present
        auto ensureColumn = [&columns](const std::string &col) {
            if (std::find(columns.begin(), columns.end(), col) == columns.end()) {
                columns.push_back(col);
            }
        };
        ensureColumn("note_seq");
        ensureColumn("note_dur");
        ensureColumn("note_slur");

        // Create parent directory if needed
        if (csvPath.has_parent_path()) {
            std::filesystem::create_directories(csvPath.parent_path());
        }

        std::ofstream file(csvPath, std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open output CSV: " + csvPath.string());
        }

        // Write header
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0)
                file << ",";
            file << quoteCSVField(columns[i]);
        }
        file << "\n";

        // Write rows
        for (size_t rowIdx = 0; rowIdx < items.size(); ++rowIdx) {
            const auto &item = items[rowIdx];
            const auto &notes = alignResults[rowIdx];

            // Build note_seq, note_dur, note_slur strings
            std::string noteSeqStr, noteDurStr, noteSlurStr;
            for (size_t i = 0; i < notes.size(); ++i) {
                if (i > 0) {
                    noteSeqStr += " ";
                    noteDurStr += " ";
                    noteSlurStr += " ";
                }
                noteSeqStr += notes[i].name;

                // Format duration with 3 decimal places
                char durBuf[32];
                std::snprintf(durBuf, sizeof(durBuf), "%.3f", notes[i].duration);
                noteDurStr += durBuf;

                noteSlurStr += std::to_string(notes[i].slur);
            }

            // Build field map with overrides
            auto fields = item.fields;
            fields["note_seq"] = noteSeqStr;
            fields["note_dur"] = noteDurStr;
            fields["note_slur"] = noteSlurStr;
            fields.erase("note_glide");

            for (size_t i = 0; i < columns.size(); ++i) {
                if (i > 0)
                    file << ",";
                auto it = fields.find(columns[i]);
                if (it != fields.end()) {
                    file << quoteCSVField(it->second);
                }
            }
            file << "\n";
        }
    }

} // namespace Game
