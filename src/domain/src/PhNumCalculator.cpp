#include <dstools/PhNumCalculator.h>
#include <dstools/TranscriptionCsv.h>

#include <QFile>
#include <QTextStream>

namespace dstools {

bool PhNumCalculator::loadDictionary(const QString &dictPath, QString &error) {
    // Python L21: hardcoded default vowels (NOTE: includes 'um')
    m_vowels = {
        QStringLiteral("SP"), QStringLiteral("AP"),
        QStringLiteral("EP"), QStringLiteral("GS"),
        QStringLiteral("um")
    };
    m_consonants.clear();

    QFile file(dictPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("Cannot open dictionary: ") + dictPath;
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        // Python L26: split by tab
        int tabIdx = line.indexOf(QLatin1Char('\t'));
        if (tabIdx < 0) {
            error = QStringLiteral("Invalid dictionary line (no tab): ") + line;
            return false;
        }

        // syllable is before tab, phonemes after
        QString phonemeStr = line.mid(tabIdx + 1).trimmed();
        QStringList phonemes = phonemeStr.split(QLatin1Char(' '), Qt::SkipEmptyParts);

        // Python L28: assert len(phonemes) <= 2
        if (phonemes.size() < 1 || phonemes.size() > 2) {
            error = QStringLiteral("Dictionary line has ") +
                    QString::number(phonemes.size()) +
                    QStringLiteral(" phonemes (expected 1-2): ") + line;
            return false;
        }

        if (phonemes.size() == 1) {
            // Python L30: single phoneme -> vowel
            m_vowels.insert(phonemes[0]);
        } else {
            // Python L32-33: two phonemes -> consonant + vowel
            m_consonants.insert(phonemes[0]);
            m_vowels.insert(phonemes[1]);
        }
    }

    m_loaded = true;
    return true;
}

void PhNumCalculator::setVowels(const QSet<QString> &vowels) {
    m_vowels = vowels;
}

void PhNumCalculator::setConsonants(const QSet<QString> &consonants) {
    m_consonants = consonants;
}

bool PhNumCalculator::isLoaded() const {
    return m_loaded;
}

bool PhNumCalculator::calculate(const QString &phSeq, QString &phNum, QString &/*error*/) const {
    QStringList phones = phSeq.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (phones.isEmpty()) {
        phNum.clear();
        return true;
    }

    QStringList result;
    int i = 0;
    const int n = phones.size();

    // Python L64-69: scan algorithm
    // Non-consonant phonemes act as word boundaries (vowels).
    // Unknown phonemes (not in vowels or consonants) are treated as vowel boundaries.
    while (i < n) {
        int j = i + 1;
        while (j < n && m_consonants.contains(phones[j])) {
            ++j;
        }
        result.append(QString::number(j - i));
        i = j;
    }

    phNum = result.join(QLatin1Char(' '));
    return true;
}

bool PhNumCalculator::calculateBatch(std::vector<TranscriptionRow> &rows, QString &error) const {
    for (auto &row : rows) {
        if (!calculate(row.phSeq, row.phNum, error))
            return false;
    }
    return true;
}

} // namespace dstools
