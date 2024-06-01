#include "MatchLyric.h"
#include "QMSystem.h"
#include <G2pglobal.h>
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMessageBox>

#include "../util/LevenshteinDistance.h"

namespace LyricFA {
    MatchLyric::MatchLyric() {
#ifdef Q_OS_MAC
        IKg2p::setDictionaryPath(QApplication::applicationDirPath() + "/../Resources/dict");
#else
        IKg2p::setDictionaryPath(QApplication::applicationDirPath() + "/dict");
#endif
        m_mandarin = std::make_unique<IKg2p::MandarinG2p>();
    }

    MatchLyric::~MatchLyric() = default;

    static QString get_lyrics_from_txt(const QString &lyricPath) {
        QFile lyricFile(lyricPath);
        if (lyricFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(lyricFile.readAll());
        }
        return {};
    }

    static bool writeJsonFile(const QString &fileName, const QJsonObject &jsonObject) {
        QFile file(fileName);
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
            const auto textList = IKg2p::splitString(get_lyrics_from_txt(lyricPath));
            const auto g2pRes = m_mandarin->hanziToPinyin(textList.join(' '), false, false);
            const auto pinyin = m_mandarin->resToStringList(g2pRes);
            m_lyricDict[lyricName] = lyricInfo{textList, pinyin};
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

            const auto asrG2pRes = m_mandarin->hanziToPinyin(asr_list, false, false);
            const auto asrPinyins = m_mandarin->resToStringList(asrG2pRes);
            if (!asrPinyins.isEmpty()) {
                auto faRes =
                    LevenshteinDistance::find_similar_substrings(asrPinyins, pinyinList, textList, true, true, true);

                QStringList asr_rect_list;
                QStringList asr_rect_diff;

                for (int i = 0; i < asrPinyins.size(); i++) {
                    const auto &asrPinyin = asrPinyins[i];
                    const auto text = faRes.match_text[i];
                    const auto matchPinyin = faRes.match_pinyin[i];

                    if (asrPinyin != matchPinyin) {
                        const QStringList candidate = m_mandarin->getDefaultPinyin(text);
                        if (candidate.contains(asrPinyin)) {
                            asr_rect_list.append(asrPinyin);
                            asr_rect_diff.append("(" + matchPinyin + "->" + asrPinyin + ", " +
                                                 QString::number(asrPinyins.indexOf(asrPinyin)) + ")");
                        } else
                            asr_rect_list.append(matchPinyin);
                    } else if (asrPinyin == matchPinyin)
                        asr_rect_list.append(asrPinyin);
                }

                if (asr_rectify)
                    faRes.match_pinyin = asr_rect_list;

                if (asrPinyins != faRes.match_pinyin && !faRes.pinyin_step.empty()) {
                    msg += "\n";
                    msg += "filename: " + filename + "\n";
                    msg += "asr_lab: " + asrPinyins.join(' ') + "\n";
                    msg += "text_res: " + faRes.match_text.join(' ') + "\n";
                    msg += "pyin_res: " + faRes.match_pinyin.join(' ') + "\n";
                    msg += "text_step: " + faRes.text_step.join(' ') + "\n";
                    msg += "pyin_step: " + faRes.pinyin_step.join(' ') + "\n";

                    if (asr_rectify && !asr_rect_diff.isEmpty())
                        msg += "asr_rect_diff: " + asr_rect_diff.join(' ');
                    msg += "------------------------";
                }

                Q_ASSERT(faRes.match_text.size() == faRes.match_pinyin.size());

                QFile jsonFile(jsonPath);
                QJsonObject writeData;
                writeData["lab"] = faRes.match_pinyin.join(' ');
                writeData["raw_text"] = faRes.match_text.join(' ');
                writeData["lab_without_tone"] = faRes.match_pinyin.join(' ');

                if (!writeJsonFile(jsonPath, writeData)) {
                    QMessageBox::critical(nullptr, QApplication::applicationName(),
                                          QString("Failed to write to file %1").arg(QMFs::PathFindFileName(jsonPath)));
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