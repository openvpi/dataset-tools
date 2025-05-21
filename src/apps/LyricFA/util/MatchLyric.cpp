#include "MatchLyric.h"
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMessageBox>

#include <cpp-pinyin/G2pglobal.h>

#include "../util/LevenshteinDistance.h"

#include <QDir>
#include <QFileInfo>

namespace LyricFA {
    MatchLyric::MatchLyric() {
#ifdef Q_OS_MAC
        Pinyin::setDictionaryPath(QApplication::applicationDirPath() + "/../Resources/dict");
#else
        Pinyin::setDictionaryPath(QApplication::applicationDirPath().toUtf8().toStdString() + "/dict");
#endif
        m_mandarin = std::make_unique<Pinyin::Pinyin>();
    }

    MatchLyric::~MatchLyric() = default;

    static QString get_lyrics_from_txt(const QString &lyricPath) {
        QFile lyricFile(lyricPath.toLocal8Bit());
        if (lyricFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(lyricFile.readAll());
        }
        return {};
    }

    static bool writeJsonFile(const QString &fileName, const QJsonObject &jsonObject) {
        QFile file(fileName.toLocal8Bit());
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        const QJsonDocument jsonDoc(jsonObject);
        const QByteArray jsonData = jsonDoc.toJson();

        if (file.write(jsonData) == -1) {
            return false;
        }

        file.close();
        return true;
    }

    void MatchLyric::initLyric(const QString &lyric_folder) {
        m_lyricDict.clear();

        const QDir lyricDir(lyric_folder);
        QStringList files = lyricDir.entryList();
        QStringList lyricPaths;
        for (const QString &file : files) {
            if (QFileInfo(file).suffix() == "txt")
                lyricPaths << lyricDir.absoluteFilePath(file);
        }

        for (const auto &lyricPath : lyricPaths) {
            const auto lyricName = QFileInfo(lyricPath).completeBaseName();
            const auto text = get_lyrics_from_txt(lyricPath).toUtf8().toStdString();
            const auto g2pRes =
                m_mandarin->hanziToPinyin(text, Pinyin::ManTone::NORMAL, Pinyin::Error::Default, false, true);

            QStringList textList;
            QStringList pinyinList;
            for (const auto &item : g2pRes) {
                textList.append(QString::fromUtf8(item.hanzi.c_str()));
                pinyinList.append(QString::fromUtf8(item.pinyin.c_str()));
            }
            m_lyricDict[lyricName] = lyricInfo{textList, pinyinList};
        }
    }

    bool MatchLyric::match(const QString &filename, const QString &labPath, const QString &jsonPath, QString &msg,
                           const bool &asr_rectify) const {
        const auto lyricName = filename.left(filename.lastIndexOf('_'));

        if (m_lyricDict.contains(lyricName)) {
            const auto asr_list = get_lyrics_from_txt(labPath);
            if (asr_list.isEmpty()) {
                msg = "filename: Asr res is empty.";
                return false;
            }
            const auto textList = m_lyricDict[lyricName].text;
            const auto pinyinList = m_lyricDict[lyricName].pinyin;

            const auto asrG2pRes = m_mandarin->hanziToPinyin(asr_list.toUtf8().toStdString(), Pinyin::ManTone::NORMAL,
                                                             Pinyin::Error::Default, false, true);

            QStringList asrPinyins;
            for (const auto &item : asrG2pRes)
                asrPinyins.append(QString::fromUtf8(item.pinyin.c_str()));

            if (!asrPinyins.isEmpty()) {
                auto [match_text, match_pinyin, text_step, pinyin_step] =
                    LevenshteinDistance::find_similar_substrings(asrPinyins, pinyinList, textList, true, true, true);

                QStringList asr_rect_list;
                QStringList asr_rect_diff;

                for (int i = 0; i < asrPinyins.size(); i++) {
                    const auto &asrPinyin = asrPinyins[i];
                    const auto text = match_text[i];
                    const auto matchPinyin = match_pinyin[i];

                    if (asrPinyin != matchPinyin) {
                        const std::vector<std::string> candidates =
                            m_mandarin->getDefaultPinyin(text.toUtf8().toStdString(), Pinyin::ManTone::NORMAL, true);
                        const auto it =
                            std::find(candidates.begin(), candidates.end(), asrPinyin.toUtf8().toStdString());

                        if (it != candidates.end()) {
                            asr_rect_list.append(asrPinyin);
                            asr_rect_diff.append("(" + matchPinyin + "->" + asrPinyin + ", " +
                                                 QString::number(asrPinyins.indexOf(asrPinyin)) + ")");
                        } else
                            asr_rect_list.append(matchPinyin);
                    } else if (asrPinyin == matchPinyin)
                        asr_rect_list.append(asrPinyin);
                }

                if (asr_rectify)
                    match_pinyin = asr_rect_list;

                if (asrPinyins != match_pinyin && !pinyin_step.empty()) {
                    msg += "\n";
                    msg += "filename: " + filename + "\n";
                    msg += "asr_lab: " + asrPinyins.join(' ') + "\n";
                    msg += "text_res: " + match_text.join(' ') + "\n";
                    msg += "pyin_res: " + match_pinyin.join(' ') + "\n";
                    msg += "text_step: " + text_step.join(' ') + "\n";
                    msg += "pyin_step: " + pinyin_step.join(' ') + "\n";

                    if (asr_rectify && !asr_rect_diff.isEmpty())
                        msg += "asr_rect_diff: " + asr_rect_diff.join(' ');
                    msg += "------------------------";
                }

                Q_ASSERT(match_text.size() == match_pinyin.size());

                QFile jsonFile(jsonPath.toLocal8Bit());
                QJsonObject writeData;
                writeData["lab"] = match_pinyin.join(' ');
                writeData["raw_text"] = match_text.join(' ');
                writeData["lab_without_tone"] = match_pinyin.join(' ');

                if (!writeJsonFile(jsonPath, writeData)) {
                    QMessageBox::critical(nullptr, QApplication::applicationName(),
                                          QString("Failed to write to file %1").arg(jsonPath));
                    return false;
                }
            }
        } else {
            msg = "filename: Miss lyric " + lyricName + ".txt";
            return false;
        }
        return true;
    }
}