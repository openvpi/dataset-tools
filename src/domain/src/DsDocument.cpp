#include <dstools/DsDocument.h>
#include <dsfw/JsonHelper.h>

#include <QDebug>

namespace dstools {

// ── Path encoding ─────────────────────────────────────────────────────

std::filesystem::path DsDocument::toFsPath(const QString &qpath) {
#ifdef _WIN32
    return std::filesystem::path(qpath.toStdWString());
#else
    return std::filesystem::path(qpath.toStdString());
#endif
}

// ── File I/O ──────────────────────────────────────────────────────────

DsDocument DsDocument::load(const QString &path, QString &error) {
    DsDocument doc;
    error.clear();

    std::string err;
    auto json = JsonHelper::loadFile(toFsPath(path), err);
    if (!err.empty()) {
        error = QString::fromStdString(err);
        return doc;
    }

    if (!json.is_array()) {
        error = QStringLiteral("DS file must be a JSON array");
        return doc;
    }

    if (json.empty()) {
        error = QStringLiteral("DS file is an empty array");
        return doc;
    }

    doc.m_sentences.reserve(json.size());
    for (size_t i = 0; i < json.size(); ++i) {
        auto &item = json[i];
        if (item.is_object()) {
            doc.m_sentences.push_back(std::move(item));
        } else {
            qWarning() << "DsDocument::load: array element" << i << "is not an object (type:"
                        << QString::fromStdString(item.type_name()) << "), skipping";
        }
    }

    if (doc.m_sentences.empty()) {
        error = QStringLiteral("DS file contains no valid sentence objects");
        return doc;
    }

    doc.m_filePath = path;
    return doc;
}

bool DsDocument::save(const QString &path, QString &error) const {
    error.clear();

    QString targetPath = path.isEmpty() ? m_filePath : path;
    if (targetPath.isEmpty()) {
        error = QStringLiteral("No file path specified");
        return false;
    }

    nlohmann::json arr = nlohmann::json::array();
    for (const auto &s : m_sentences) {
        arr.push_back(s);
    }

    std::string err;
    if (!JsonHelper::saveFile(toFsPath(targetPath), arr, err)) {
        error = QString::fromStdString(err);
        return false;
    }
    return true;
}

bool DsDocument::save(QString &error) const {
    return save(QString(), error);
}

Result<DsDocument> DsDocument::loadFile(const QString &path) {
    QString error;
    auto doc = load(path, error);
    if (!error.isEmpty())
        return Result<DsDocument>::Error(error.toStdString());
    return doc;
}

Result<void> DsDocument::saveFile(const QString &path) const {
    QString error;
    if (!save(path.isEmpty() ? QString() : path, error))
        return Result<void>::Error(error.toStdString());
    return Result<void>::Ok();
}

// ── Sentence access ───────────────────────────────────────────────────

int DsDocument::sentenceCount() const {
    return static_cast<int>(m_sentences.size());
}

bool DsDocument::isEmpty() const {
    return m_sentences.empty();
}

nlohmann::json &DsDocument::sentence(int index) {
    Q_ASSERT(index >= 0 && index < static_cast<int>(m_sentences.size()));
    return m_sentences[index];
}

const nlohmann::json &DsDocument::sentence(int index) const {
    Q_ASSERT(index >= 0 && index < static_cast<int>(m_sentences.size()));
    return m_sentences[index];
}

std::vector<nlohmann::json> &DsDocument::sentences() {
    return m_sentences;
}

const std::vector<nlohmann::json> &DsDocument::sentences() const {
    return m_sentences;
}

const QString &DsDocument::filePath() const {
    return m_filePath;
}

void DsDocument::setFilePath(const QString &path) {
    m_filePath = path;
}

// ── Field helpers ─────────────────────────────────────────────────────

double DsDocument::numberOrString(const nlohmann::json &obj, const std::string &key,
                                  double defaultValue) {
    if (!obj.is_object() || !obj.contains(key))
        return defaultValue;

    const auto &node = obj[key];
    if (node.is_number()) {
        return node.get<double>();
    }
    if (node.is_string()) {
        bool ok = false;
        double val = QString::fromStdString(node.get<std::string>()).toDouble(&ok);
        return ok ? val : defaultValue;
    }
    return defaultValue;
}

} // namespace dstools
