#include <dstools/PhNumCalculator.h>
#include <dstools/TranscriptionCsv.h>

#include <dsfw/PathUtils.h>

#include <QFile>
#include <QTextStream>
#include <QVector>

namespace dstools {

dsfw::Result<void> PhNumCalculator::loadDictionary(const QString& dictPath) {
    m_vowels = {QStringLiteral("um")};
    m_consonants.clear();
    const QSet<QString> defaultSpecial = {
        QStringLiteral("SP"), QStringLiteral("AP"), QStringLiteral("EP"), QStringLiteral("GS")};
    m_specialWords = defaultSpecial;

    QFile file(dictPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return dsfw::Result<void>::Error(std::string("Cannot open dictionary: ") +
                                         dsfw::PathUtils::toUtf8(std::filesystem::path(dictPath.toStdWString())));
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        int tabIdx = line.indexOf(QLatin1Char('\t'));
        if (tabIdx < 0) {
            return dsfw::Result<void>::Error(std::string("Invalid dictionary line (no tab): ") + line.toStdString());
        }

        QString phonemeStr = line.mid(tabIdx + 1).trimmed();
        QStringList phonemes = phonemeStr.split(QLatin1Char(' '), Qt::SkipEmptyParts);

        if (phonemes.size() < 1 || phonemes.size() > 2) {
            return dsfw::Result<void>::Error(std::string("Dictionary line has ") + std::to_string(phonemes.size()) +
                                             " phonemes (expected 1-2): " + line.toStdString());
        }

        if (phonemes.size() == 1) {
            m_vowels.insert(phonemes[0]);
        } else {
            m_consonants.insert(phonemes[0]);
            m_vowels.insert(phonemes[1]);
        }
    }

    m_loaded = true;
    return dsfw::Result<void>::Ok();
}

dsfw::Result<void> PhNumCalculator::loadVowelsFromFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return dsfw::Result<void>::Error(std::string("Cannot open vowels file: ") +
                                         dsfw::PathUtils::toUtf8(std::filesystem::path(path.toStdWString())));
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        m_vowels.insert(line);
    }
    m_loaded = true;
    return dsfw::Result<void>::Ok();
}

dsfw::Result<void> PhNumCalculator::loadConsonantsFromFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return dsfw::Result<void>::Error(std::string("Cannot open consonants file: ") +
                                         dsfw::PathUtils::toUtf8(std::filesystem::path(path.toStdWString())));
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        m_consonants.insert(line);
    }
    m_loaded = true;
    return dsfw::Result<void>::Ok();
}

void PhNumCalculator::setVowels(const QSet<QString>& vowels) {
    m_vowels = vowels;
    m_loaded = true;
}

void PhNumCalculator::setConsonants(const QSet<QString>& consonants) {
    m_consonants = consonants;
    m_loaded = true;
}

void PhNumCalculator::setSpecialWords(const QSet<QString>& words) {
    m_specialWords = words;
}

bool PhNumCalculator::isLoaded() const {
    return m_loaded;
}

dsfw::Result<QString> PhNumCalculator::calculate(const QString& phSeq) const {
    if (!m_loaded) {
        return dsfw::Result<QString>::Error("PhNumCalculator not loaded");
    }

    QStringList phones = phSeq.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (phones.isEmpty()) {
        return QString();
    }

    QVector<int> cleanMap(phones.size(), -1);
    QStringList cleanPhones;
    for (int idx = 0; idx < static_cast<int>(phones.size()); ++idx) {
        if (m_specialWords.contains(phones[idx]))
            continue;
        cleanMap[cleanPhones.size()] = idx;
        cleanPhones.append(phones[idx]);
    }

    QStringList cleanResult;
    int i = 0;
    const int n = cleanPhones.size();
    while (i < n) {
        int j = i + 1;
        while (j < n && m_consonants.contains(cleanPhones[j])) {
            ++j;
        }
        if (j - i == 1 && j < n && m_consonants.contains(cleanPhones[i]) && !m_consonants.contains(cleanPhones[j])) {
            ++j;
        }
        cleanResult.append(QString::number(j - i));
        i = j;
    }

    QStringList fullResult;
    int cleanIdx = 0;
    for (int idx = 0; idx < static_cast<int>(phones.size()); ++idx) {
        if (m_specialWords.contains(phones[idx])) {
            fullResult.append(QString::number(1));
        } else if (cleanIdx < cleanResult.size()) {
            fullResult.append(cleanResult[cleanIdx]);
            ++cleanIdx;
        }
    }

    return fullResult.join(QLatin1Char(' '));
}

dsfw::Result<void> PhNumCalculator::calculateBatch(std::vector<TranscriptionRow>& rows) const {
    for (auto& row : rows) {
        auto result = calculate(row.phSeq);
        if (!result.ok())
            return dsfw::Result<void>::Error(result.error());
        row.phNum = std::move(result.value());
    }
    return dsfw::Result<void>::Ok();
}

}  // namespace dstools