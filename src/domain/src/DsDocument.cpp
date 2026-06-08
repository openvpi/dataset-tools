#include <dstools/DsDocument.h>
#include <dsfw/JsonHelper.h>
#include <dsfw/PathUtils.h>

using dsfw::JsonHelper;

#include <QDebug>

namespace dstools {


struct DsDocument::Impl {
    std::vector<nlohmann::json> sentences;
    QString filePath;
};

// ── Construction / destruction ────────────────────────────────────────

DsDocument::DsDocument()
    : m_impl(std::make_unique<Impl>()) {}

DsDocument::~dsfw::DsDocument() = default;

DsDocument::DsDocument(dsfw::DsDocument &&other) noexcept
    : m_impl(std::move(other.m_impl)) {}

dsfw::DsDocument &DsDocument::operator=(dsfw::DsDocument &&other) noexcept {
    if (this != &other)
        m_impl = std::move(other.m_impl);
    return *this;
}

// ── File I/O ──────────────────────────────────────────────────────────

dsfw::Result<dsfw::DsDocument> DsDocument::loadFile(const QString &path) {
    dsfw::DsDocument doc;

    auto textResult = dsfw::PathUtils::readFile(path);
    if (!textResult.ok()) {
        return dsfw::Result<dsfw::DsDocument>::Error(textResult.error());
    }

    nlohmann::json json;
    try {
        json = nlohmann::json::parse(textResult.value().toStdString());
    } catch (const nlohmann::json::parse_error &e) {
        return dsfw::Result<dsfw::DsDocument>::Error(std::string("DS file JSON parse error: ") + e.what());
    } catch (const nlohmann::json::type_error &e) {
        return dsfw::Result<dsfw::DsDocument>::Error(std::string("DS file JSON type error: ") + e.what());
    }

    if (!json.is_array()) {
        return dsfw::Result<dsfw::DsDocument>::Error("DS file must be a JSON array");
    }

    if (json.empty()) {
        return dsfw::Result<dsfw::DsDocument>::Error("DS file is an empty array");
    }

    doc.m_impl->sentences.reserve(json.size());
    int skippedNonObject = 0;
    for (size_t i = 0; i < json.size(); ++i) {
        auto &item = json[i];
        if (item.is_object()) {
            doc.m_impl->sentences.push_back(std::move(item));
        } else {
            ++skippedNonObject;
            qWarning() << "DsDocument::loadFile: array element" << i << "is not an object (type:"
                        << QString::fromStdString(item.type_name()) << "), skipping";
        }
    }

    if (doc.m_impl->sentences.empty()) {
        if (skippedNonObject > 0)
            return dsfw::Result<dsfw::DsDocument>::Error(
                QStringLiteral("DS file contains no valid sentence objects (%1/%2 elements skipped as non-object)")
                    .arg(skippedNonObject)
                    .arg(json.size())
                    .toStdString());
        return dsfw::Result<dsfw::DsDocument>::Error("DS file contains no valid sentence objects");
    }

    doc.m_impl->filePath = path;
    return doc;
}

dsfw::Result<void> DsDocument::saveFile(const QString &path) const {
    QString targetPath = path.isEmpty() ? m_impl->filePath : path;
    if (targetPath.isEmpty()) {
        return dsfw::Result<void>::Error("No file path specified");
    }

    nlohmann::json arr = nlohmann::json::array();
    for (const auto &s : m_impl->sentences) {
        arr.push_back(s);
    }

    auto saveResult = JsonHelper::saveFile(dsfw::PathUtils::toStdPath(targetPath), arr);
    if (!saveResult) {
        return dsfw::Result<void>::Error(saveResult.error());
    }
    return dsfw::Result<void>::Ok();
}

// ── Sentence access ───────────────────────────────────────────────────

int DsDocument::sentenceCount() const {
    return static_cast<int>(m_impl->sentences.size());
}

bool DsDocument::isEmpty() const {
    return m_impl->sentences.empty();
}

// ── Typed sentence access ─────────────────────────────────────────────

static QString jsonStrField(const nlohmann::json &j, const char *key) {
    if (!j.contains(key) || !j[key].is_string())
        return {};
    return QString::fromStdString(j[key].get<std::string>());
}

static double jsonNumField(const nlohmann::json &j, const char *key, double def = 0.0) {
    if (!j.contains(key))
        return def;
    const auto &node = j[key];
    if (node.is_number())
        return node.get<double>();
    if (node.is_string())
        return QString::fromStdString(node.get<std::string>()).toDouble();
    return def;
}

static void setJsonStrField(nlohmann::json &j, const char *key, const QString &val) {
    if (val.isEmpty()) {
        j.erase(key);
    } else {
        j[key] = val.toStdString();
    }
}

static void setJsonNumField(nlohmann::json &j, const char *key, double val, bool force = false) {
    if (!force && val == 0.0) {
        j.erase(key);
    } else {
        j[key] = val;
    }
}

DsDocument::SentenceView DsDocument::sentenceView(int index) const {
    if (index < 0 || index >= static_cast<int>(m_impl->sentences.size())) {
        return {};
    }
    const auto &j = m_impl->sentences[index];
    SentenceView sv;
    sv.text = jsonStrField(j, "text");
    sv.phSeq = jsonStrField(j, "ph_seq");
    sv.phDur = jsonStrField(j, "ph_dur");
    sv.phNum = jsonStrField(j, "ph_num");
    sv.noteSeq = jsonStrField(j, "note_seq");
    sv.noteDur = jsonStrField(j, "note_dur");
    sv.noteSlur = jsonStrField(j, "note_slur");
    sv.noteGlide = jsonStrField(j, "note_glide");
    sv.f0Seq = jsonStrField(j, "f0_seq");
    sv.wordSpan = jsonStrField(j, "word_span");
    sv.offset = jsonNumField(j, "offset");
    sv.f0Timestep = jsonNumField(j, "f0_timestep", 0.005);
    return sv;
}

void DsDocument::setSentenceView(int index, const SentenceView &view) {
    if (index < 0 || index >= static_cast<int>(m_impl->sentences.size())) {
        qWarning() << "DsDocument::setSentenceView: index" << index << "out of range";
        return;
    }
    auto &j = m_impl->sentences[index];
    setJsonStrField(j, "text", view.text);
    setJsonStrField(j, "ph_seq", view.phSeq);
    setJsonStrField(j, "ph_dur", view.phDur);
    setJsonStrField(j, "ph_num", view.phNum);
    setJsonStrField(j, "note_seq", view.noteSeq);
    setJsonStrField(j, "note_dur", view.noteDur);
    setJsonStrField(j, "note_slur", view.noteSlur);
    setJsonStrField(j, "note_glide", view.noteGlide);
    setJsonStrField(j, "f0_seq", view.f0Seq);
    setJsonStrField(j, "word_span", view.wordSpan);
    setJsonNumField(j, "offset", view.offset);
    setJsonNumField(j, "f0_timestep", view.f0Timestep);
}

void DsDocument::addSentenceView(const SentenceView &view) {
    nlohmann::json j;
    setJsonStrField(j, "text", view.text);
    setJsonStrField(j, "ph_seq", view.phSeq);
    setJsonStrField(j, "ph_dur", view.phDur);
    setJsonStrField(j, "ph_num", view.phNum);
    setJsonStrField(j, "note_seq", view.noteSeq);
    setJsonStrField(j, "note_dur", view.noteDur);
    setJsonStrField(j, "note_slur", view.noteSlur);
    setJsonStrField(j, "note_glide", view.noteGlide);
    setJsonStrField(j, "f0_seq", view.f0Seq);
    setJsonStrField(j, "word_span", view.wordSpan);
    setJsonNumField(j, "offset", view.offset, true);
    setJsonNumField(j, "f0_timestep", view.f0Timestep);
    m_impl->sentences.push_back(std::move(j));
}

// ── Raw JSON access ───────────────────────────────────────────────────

std::string DsDocument::rawSentenceJsonString(int index) const {
    if (index < 0 || index >= static_cast<int>(m_impl->sentences.size())) {
        return "{}";
    }
    return m_impl->sentences[index].dump();
}

void DsDocument::addRawSentence(const std::string &jsonStr) {
    try {
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        m_impl->sentences.push_back(std::move(j));
    } catch (const nlohmann::json::parse_error &e) {
        qWarning() << "DsDocument::addRawSentence: JSON parse error:" << e.what();
    }
}

void DsDocument::setRawSentence(int index, const std::string &jsonStr) {
    if (index < 0 || index >= static_cast<int>(m_impl->sentences.size())) {
        qWarning() << "DsDocument::setRawSentence: index" << index << "out of range";
        return;
    }
    try {
        m_impl->sentences[index] = nlohmann::json::parse(jsonStr);
    } catch (const nlohmann::json::parse_error &e) {
        qWarning() << "DsDocument::setRawSentence: JSON parse error:" << e.what();
    }
}

const QString &DsDocument::filePath() const {
    return m_impl->filePath;
}

void DsDocument::setFilePath(const QString &path) {
    m_impl->filePath = path;
}

// ── Field helpers ─────────────────────────────────────────────────────

double DsDocument::durationSec() const {
    double maxEnd = 0.0;
    for (const auto &s : m_impl->sentences) {
        double offset = jsonNumField(s, "offset", 0.0);
        double dur = 0.0;
        if (s.contains("ph_dur") && s["ph_dur"].is_string()) {
            const auto parts = QString::fromStdString(s["ph_dur"].get<std::string>())
                                   .split(QLatin1Char(' '), Qt::SkipEmptyParts);
            for (const auto &tok : parts)
                dur += tok.toDouble();
        }
        double end = offset + dur;
        if (end > maxEnd)
            maxEnd = end;
    }
    return maxEnd;
}

} // namespace dstools
