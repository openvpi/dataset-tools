#include "MatchLyric.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QMessageBox>
#include <QTextStream>

namespace LyricFA {

    MatchLyric::MatchLyric() : m_matcher(std::make_unique<LyricMatcher>()) {
    }

    MatchLyric::~MatchLyric() = default;

    void MatchLyric::initLyric(const QString &lyric_folder) {
        m_lyricDict.clear();
        const QDir dir(lyric_folder);
        QStringList filters;
        filters << "*.txt";
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

        for (const QFileInfo &fi : files) {
            QString name = fi.completeBaseName();
            try {
                const LyricData data = m_matcher->process_lyric_file(fi.absoluteFilePath());
                m_lyricDict[name] = {data.text_list, data.phonetic_list};
            } catch (const std::exception &e) {
                qWarning() << "Error processing lyric" << name << ":" << e.what();
            }
        }
    }

    bool MatchLyric::match(const QString &filename, const QString &labPath, const QString &jsonPath,
                           QString &msg) const {
        QString lyricName = filename;
        const int lastUnderscore = filename.lastIndexOf('_');
        if (lastUnderscore != -1) {
            lyricName = filename.left(lastUnderscore);
        }

        if (!m_lyricDict.contains(lyricName)) {
            msg = QString("Missing lyric file: %1.txt").arg(lyricName);
            return false;
        }

        QFile labFile(labPath);
        if (!labFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            msg = QString("Cannot read lab file: %1").arg(labPath);
            return false;
        }
        QTextStream labStream(&labFile);
        const QString labContent = labStream.readAll().trimmed();
        labFile.close();

        const auto &[text, pinyin] = m_lyricDict[lyricName];
        auto [asr_text, asr_phonetic] = m_matcher->process_asr_content(labContent);

        if (asr_phonetic.isEmpty()) {
            msg = QString("ASR result empty for %1").arg(filename);
            return false;
        }

        auto [matched_text, matched_phonetic, reason] = m_matcher->align_lyric_with_asr(asr_phonetic, text, pinyin);

        if (matched_phonetic.isEmpty()) {
            msg =
                QString(
                    "lab_name:         %1  -> 未能匹配到任何歌词片段\n失败原因:         %2\nasr_result (全部多余): %3")
                    .arg(filename, reason, asr_phonetic.join(' '));
            LyricMatcher::save_to_json(jsonPath, "", "");
            return true;
        }

        // 计算差异
        const int diff =
            calculate_difference_count(asr_phonetic, matched_phonetic.split(' ', Qt::SkipEmptyParts).toVector());
        if (diff > m_diffThreshold) {
            auto [hl_asr, hl_phonetic, hl_text, ops] =
                m_matcher->highlighter().highlight_differences(asr_phonetic.join(' '), matched_phonetic, matched_text);
            msg = QString("\nlab_name:             %1\nmatch_text:           %2\nasr_result:             "
                          "%3\nmatch_phonetic:   "
                          "%4\ndiff count:             %5")
                      .arg(filename, hl_text, hl_asr, hl_phonetic)
                      .arg(ops);
        } else {
            msg = "";
        }

        LyricMatcher::save_to_json(jsonPath, matched_text, matched_phonetic);
        return true;
    }

    bool MatchLyric::matchText(const QString &filename, const QString &asrText, QString &matchedText,
                               QString &msg) const {
        QString lyricName = filename;
        const int lastUnderscore = filename.lastIndexOf('_');
        if (lastUnderscore != -1) {
            lyricName = filename.left(lastUnderscore);
        }

        if (!m_lyricDict.contains(lyricName)) {
            msg = QString("Missing lyric file: %1.txt").arg(lyricName);
            return false;
        }

        const auto &[text, pinyin] = m_lyricDict[lyricName];
        auto [asr_text, asr_phonetic] = m_matcher->process_asr_content(asrText);

        if (asr_phonetic.isEmpty()) {
            msg = QString("ASR result empty for %1").arg(filename);
            return false;
        }

        auto [matched_text, matched_phonetic, reason] = m_matcher->align_lyric_with_asr(asr_phonetic, text, pinyin);

        if (matched_text.isEmpty()) {
            msg = QString("No match found for %1: %2").arg(filename, reason);
            return false;
        }

        matchedText = matched_text;
        return true;
    }

} // namespace LyricFA