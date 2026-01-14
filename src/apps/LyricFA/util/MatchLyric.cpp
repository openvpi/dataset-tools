#include "MatchLyric.h"
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMessageBox>

#include "Utils.h"

#include <cpp-pinyin/G2pglobal.h>
#include <cpp-pinyin/U16Str.h>

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
        m_lyricAligner = new LyricAligner();
    }

    MatchLyric::~MatchLyric() = default;

    static QString get_lyrics_from_txt(const QString &lyricPath) {
        if (QFile lyricFile(lyricPath); lyricFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            auto words = QString::fromUtf8(lyricFile.readAll());
            words.remove(QRegularExpression(u8"[^\u4E00-\u9FFF\u3400-\u4DBF\uF900-\uFAFF a-zA-Z]"));
            return words;
        }
        return {};
    }

    static bool writeJsonFile(const QString &fileName, const QJsonObject &jsonObject) {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        const QJsonDocument jsonDoc(jsonObject);

        if (const QByteArray jsonData = jsonDoc.toJson(); file.write(jsonData) == -1) {
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
                m_mandarin->hanziToPinyin(text, Pinyin::ManTone::NORMAL, Pinyin::Error::Default, false, false);

            std::vector<std::string> textList, pinyinList;
            for (const auto &item : g2pRes) {
                textList.emplace_back(item.hanzi.c_str());
                pinyinList.emplace_back(item.pinyin.c_str());
            }
            if (textList.empty() || pinyinList.empty())
                continue;
            m_lyricDict[lyricName] = lyricInfo{textList, pinyinList};
        }
    }

    bool MatchLyric::match(const QString &filename, const QString &labPath, const QString &jsonPath, QString &msg,
                           const bool &asr_rectify) const {

        if (const auto lyricName = filename.left(filename.lastIndexOf('_')); m_lyricDict.contains(lyricName)) {
            const auto asr_list = get_lyrics_from_txt(labPath).toUtf8().toStdString();
            if (asr_list.empty()) {
                msg = QString("%1: Asr result is empty.").arg(lyricName);
                return false;
            }
            const auto textList = m_lyricDict[lyricName].text;
            const auto pinyinList = m_lyricDict[lyricName].pinyin;

            const auto asrTextU16str = Pinyin::utf8strToU16str(asr_list);
            const auto asrTextVecU16str = splitString(asrTextU16str);

            std::vector<std::string> asrTextVec;
            for (const auto &item : asrTextVecU16str) {
                if (!item.empty())
                    asrTextVec.push_back(Pinyin::u16strToUtf8str(item.c_str()));
            }

            const auto asrG2pRes =
                m_mandarin->hanziToPinyin(asrTextVec, Pinyin::ManTone::NORMAL, Pinyin::Error::Default, false, true);

            std::vector<std::string> asrPinyins;
            for (const auto &item : asrG2pRes)
                asrPinyins.emplace_back(item.pinyin.c_str());

            if (!asrPinyins.empty()) {
                auto [match_text, match_pinyin, text_step, pinyin_step] =
                    m_lyricAligner->alignSequences(asrTextVec, asrPinyins, pinyinList, textList, true, true, true);

                std::vector<std::string> asr_rect_list, asr_rect_diff;

                for (int i = 0; i < asrPinyins.size(); i++) {
                    const auto asrPinyin = asrPinyins[i];
                    const auto text = match_text[i];

                    if (const auto matchPinyin = match_pinyin[i]; asrPinyin != matchPinyin) {
                        const std::vector<std::string> candidates =
                            m_mandarin->getDefaultPinyin(text, Pinyin::ManTone::NORMAL, true);

                        if (const auto it = std::find(candidates.begin(), candidates.end(), asrPinyin);
                            it != candidates.end()) {
                            asr_rect_list.push_back(asrPinyin);
                            asr_rect_diff.push_back("(" + matchPinyin + "->" + asrPinyin + ", " + static_cast<char>(i) +
                                                    ")");
                        } else
                            asr_rect_list.push_back(matchPinyin);
                    } else if (asrPinyin == matchPinyin)
                        asr_rect_list.push_back(asrPinyin);
                }

                if (asr_rectify)
                    match_pinyin = asr_rect_list;

                if (asrPinyins != match_pinyin && !pinyin_step.empty()) {
                    msg += "\n";
                    msg += "filename: " + filename + "\n";
                    msg += "asr_lab: " + LyricAligner::join(asrTextVec, " ") + "\n";
                    msg += "text_res: " + LyricAligner::join(match_text, " ") + "\n";
                    msg += "pyin_res: " + LyricAligner::join(match_pinyin, " ") + "\n";
                    msg += "text_step: " + text_step + "\n";
                    msg += "pyin_step: " + pinyin_step + "\n";

                    if (asr_rectify && !asr_rect_diff.empty())
                        msg += "asr_rect_diff: " + LyricAligner::join(asr_rect_diff, " ");
                    msg += "------------------------";
                }

                Q_ASSERT(match_text.size() == match_pinyin.size());

                QFile jsonFile(jsonPath);
                QJsonObject writeData;
                writeData["lab"] = QString::fromUtf8(LyricAligner::join(match_pinyin, " "));
                writeData["raw_text"] = QString::fromUtf8(LyricAligner::join(match_text, " "));
                writeData["lab_without_tone"] = QString::fromUtf8(LyricAligner::join(match_pinyin, " "));

                if (!writeJsonFile(jsonPath, writeData)) {
                    QMessageBox::critical(nullptr, QApplication::applicationName(),
                                          QString("Failed to write to file %1").arg(jsonPath));
                    return false;
                }
            }
        } else {
            msg = "filename: Miss lyric " + lyricName + ".txt or lyric file is empty";
            return false;
        }
        return true;
    }
}