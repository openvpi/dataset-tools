#include <dstools/TranscriptionCsv.h>

#include <dsfw/PathUtils.h>

#include <QHash>

namespace dstools {

using namespace dsfw;

// ── CSV parsing helpers ──────────────────────────────────────────────

namespace {

/// Parse fields from a CSV record starting at \a pos in \a content.
/// Advances \a pos past the line ending. Handles RFC 4180 quoted fields
/// (embedded commas, newlines, escaped quotes as "").
QStringList parseCsvRecord(const QString &content, int &pos) {
    QStringList fields;
    QString field;
    const int len = content.size();
    bool inQuotes = false;

    while (pos < len) {
        const QChar ch = content[pos];

        if (inQuotes) {
            if (ch == QLatin1Char('"')) {
                // Look ahead for escaped quote
                if (pos + 1 < len && content[pos + 1] == QLatin1Char('"')) {
                    field += QLatin1Char('"');
                    pos += 2;
                } else {
                    // End of quoted section
                    inQuotes = false;
                    ++pos;
                }
            } else {
                field += ch;
                ++pos;
            }
        } else {
            if (ch == QLatin1Char('"') && field.isEmpty()) {
                inQuotes = true;
                ++pos;
            } else if (ch == QLatin1Char(',')) {
                fields.append(field);
                field.clear();
                ++pos;
            } else if (ch == QLatin1Char('\r')) {
                // \r\n or bare \r — end of record
                ++pos;
                if (pos < len && content[pos] == QLatin1Char('\n'))
                    ++pos;
                break;
            } else if (ch == QLatin1Char('\n')) {
                ++pos;
                break;
            } else {
                field += ch;
                ++pos;
            }
        }
    }

    // Append last field (always — even if empty, to match column count)
    fields.append(field);
    return fields;
}

/// Quote a field for CSV output if it contains special characters.
QString quoteField(const QString &field) {
    if (field.contains(QLatin1Char(',')) || field.contains(QLatin1Char('"')) ||
        field.contains(QLatin1Char('\n'))) {
        QString escaped = field;
        escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return field;
}

/// Column descriptors: CSV column name, pointer-to-member, required flag.
struct ColumnDef {
    const char *csvName;
    QString TranscriptionRow::*member;
    bool required;
};

static const ColumnDef kColumns[] = {
    {"name",       &TranscriptionRow::name,      true},
    {"ph_seq",     &TranscriptionRow::phSeq,     true},
    {"ph_dur",     &TranscriptionRow::phDur,     true},
    {"ph_num",     &TranscriptionRow::phNum,     true},
    {"note_seq",   &TranscriptionRow::noteSeq,   true},
    {"note_dur",   &TranscriptionRow::noteDur,   true},
    {"word_span",  &TranscriptionRow::wordSpan,  true},
    {"note_slur",  &TranscriptionRow::noteSlur,  false},
    {"note_glide", &TranscriptionRow::noteGlide, false},
};

static constexpr int kColumnCount = sizeof(kColumns) / sizeof(kColumns[0]);

} // anonymous namespace

// ── Public API ───────────────────────────────────────────────────────

Result<std::vector<TranscriptionRow>> TranscriptionCsv::parse(const QString &content) {
    std::vector<TranscriptionRow> rows;
    if (content.isEmpty()) {
        return Result<std::vector<TranscriptionRow>>::Error("Empty content");
    }

    // Skip BOM
    int pos = 0;
    if (content.startsWith(QChar(0xFEFF)))
        pos = 1;

    // Parse header
    const QStringList header = parseCsvRecord(content, pos);
    if (header.isEmpty()) {
        return Result<std::vector<TranscriptionRow>>::Error("Missing header row");
    }

    // Build column index map: CSV column name → index in header
    QHash<QString, int> colIndex;
    for (int i = 0; i < static_cast<int>(header.size()); ++i)
        colIndex.insert(header[i].trimmed().toLower(), i);

    // Map known columns to header indices
    int colMap[kColumnCount];
    for (int i = 0; i < kColumnCount; ++i) {
        const auto it = colIndex.constFind(QString::fromLatin1(kColumns[i].csvName));
        if (it != colIndex.constEnd()) {
            colMap[i] = it.value();
        } else if (kColumns[i].required) {
            return Result<std::vector<TranscriptionRow>>::Error(
                std::string("Missing required column: ") + kColumns[i].csvName);
        } else {
            colMap[i] = -1;
        }
    }

    // Parse data rows
    while (pos < content.size()) {
        // Skip blank lines
        if (content[pos] == QLatin1Char('\r') || content[pos] == QLatin1Char('\n')) {
            ++pos;
            continue;
        }

        const QStringList fields = parseCsvRecord(content, pos);

        // Skip if all fields empty (trailing newline artifact)
        bool allEmpty = true;
        for (const auto &f : fields) {
            if (!f.isEmpty()) {
                allEmpty = false;
                break;
            }
        }
        if (allEmpty)
            continue;

        TranscriptionRow row;
        for (int i = 0; i < kColumnCount; ++i) {
            if (colMap[i] >= 0 && colMap[i] < fields.size())
                row.*(kColumns[i].member) = fields[colMap[i]];
        }
        rows.push_back(std::move(row));
    }

    return rows;
}

Result<std::vector<TranscriptionRow>> TranscriptionCsv::read(const QString &path) {
    auto textResult = dsfw::PathUtils::readFile(path);
    if (!textResult.ok()) {
        return Result<std::vector<TranscriptionRow>>::Error(textResult.error());
    }
    return parse(textResult.value());
}

Result<void> TranscriptionCsv::write(const QString &path,
                                   const std::vector<TranscriptionRow> &rows) {
    // Determine which optional columns to include
    bool includeCol[kColumnCount];
    for (int i = 0; i < kColumnCount; ++i) {
        if (kColumns[i].required) {
            includeCol[i] = true;
        } else {
            includeCol[i] = false;
            for (const auto &row : rows) {
                if (!(row.*(kColumns[i].member)).isEmpty()) {
                    includeCol[i] = true;
                    break;
                }
            }
        }
    }

    // Build output
    QString out;
    out.reserve(static_cast<int>(rows.size()) * 200);

    // Header
    bool first = true;
    for (int i = 0; i < kColumnCount; ++i) {
        if (!includeCol[i])
            continue;
        if (!first)
            out += QLatin1Char(',');
        out += QString::fromLatin1(kColumns[i].csvName);
        first = false;
    }
    out += QLatin1Char('\n');

    // Data rows
    for (const auto &row : rows) {
        first = true;
        for (int i = 0; i < kColumnCount; ++i) {
            if (!includeCol[i])
                continue;
            if (!first)
                out += QLatin1Char(',');
            out += quoteField(row.*(kColumns[i].member));
            first = false;
        }
        out += QLatin1Char('\n');
    }

    // Write file
    auto writeResult = dsfw::PathUtils::writeFile(path, out);
    if (!writeResult.ok()) {
        return Result<void>::Error(writeResult.error());
    }
    return Result<void>::Ok();
}

} // namespace dstools