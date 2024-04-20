#include "MatchLyric.h"
#include "QMSystem.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMessageBox>
#include <g2pglobal.h>

#include "../util/LevenshteinDistance.h"

MatchLyric::MatchLyric() {
#ifdef Q_OS_MAC
    IKg2p::setDictionaryPath(qApp->applicationDirPath() + "/../Resources/dict");
#else
    IKg2p::setDictionaryPath(qApp->applicationDirPath() + "/dict");
#endif
    m_mandarin = new IKg2p::Mandarin();
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

static void generate_json(const QString &jsonFilePath, const QString &text, const QString &pinyin) {
    QFile jsonFile(jsonFilePath);
    QJsonObject writeData;
    writeData["lab"] = pinyin;
    writeData["raw_text"] = text;
    writeData["lab_without_tone"] = pinyin;

    if (!writeJsonFile(jsonFilePath, writeData)) {
        QMessageBox::critical(nullptr, qApp->applicationName(),
                              QString("Failed to write to file %1").arg(QMFs::PathFindFileName(jsonFilePath)));
        ::exit(-1);
    }
}

void MatchLyric::match(QPlainTextEdit *out, const QString &lyric_folder, const QString &lab_folder,
                       const QString &json_folder, const bool &asr_rectify) const {
    if (!QDir(json_folder).exists())
        qDebug() << QDir(json_folder).mkpath(json_folder);

    struct lyricInfo {
        QStringList text, pinyin;
    };
    QMap<QString, lyricInfo> lyric_dict;

    const QDir lyricDir(lyric_folder);
    QStringList files = lyricDir.entryList();
    QStringList lyricPaths;
    for (const QString &file : files) {
        if (QFileInfo(file).suffix() == "txt")
            lyricPaths << lyricDir.absoluteFilePath(file);
    }

    for (const auto &lyricPath : lyricPaths) {
        const auto lyric_name = QFileInfo(lyricPath).completeBaseName();
        const auto text_list = IKg2p::splitStringToList(get_lyrics_from_txt(lyricPath));
        lyric_dict[lyric_name] =
            lyricInfo{text_list, m_mandarin->convert(text_list.join(' '), false, false).split(' ')};
    }

    int file_num = 0;
    int success_num = 0;
    QSet<QString> miss_lyric;
    int diff_num = 0;

    const QDir labDir(lab_folder);
    QStringList labFiles = labDir.entryList();
    QStringList labPaths;
    for (const QString &file : labFiles) {
        if (QFileInfo(file).suffix() == "lab")
            labPaths << labDir.absoluteFilePath(file);
    }
    for (const auto &labPath : labPaths) {
        file_num++;
        const auto lab_name = QFileInfo(labPath).completeBaseName();
        const auto lyric_name = lab_name.left(lab_name.lastIndexOf('_'));

        if (lyric_dict.contains(lyric_name)) {
            const auto asr_list = get_lyrics_from_txt(labPath);
            const auto text_list = lyric_dict[lyric_name].text;
            const auto pinyin_list = lyric_dict[lyric_name].pinyin;

            const auto asrPinyins = m_mandarin->convert(asr_list, false, false).split(' ');
            if (!asrPinyins.isEmpty()) {
                auto faRes =
                    LevenshteinDistance::find_similar_substrings(asrPinyins, pinyin_list, text_list, true, true, true);

                QStringList asr_rect_list;
                QStringList asr_rect_diff;

                for (int i = 0; i < asrPinyins.size(); i++) {
                    const auto &asrPinyin = asrPinyins[i];
                    const auto text = faRes.match_text[i];
                    const auto matchPinyin = faRes.match_pinyin[i];

                    if (asrPinyin != matchPinyin) {
                        QStringList candidate = m_mandarin->getDefaultPinyin(text);
                        if (candidate.contains(asrPinyin)) {
                            asr_rect_list.append(asrPinyin);
                            asr_rect_diff.append("(" + matchPinyin + "->" + asrPinyin + ", " +
                                                 asrPinyins.indexOf(asrPinyin) + ")");
                        } else
                            asr_rect_list.append(matchPinyin);
                    } else if (asrPinyin == matchPinyin)
                        asr_rect_list.append(asrPinyin);
                }

                if (asr_rectify)
                    faRes.match_pinyin = asr_rect_list;

                if (asrPinyins != faRes.match_pinyin && !faRes.pinyin_step.empty()) {
                    out->appendPlainText("lab_name: " + lab_name);
                    out->appendPlainText("asr_lab: " + asrPinyins.join(' '));
                    out->appendPlainText("text_res: " + faRes.match_text.join(' '));
                    out->appendPlainText("pyin_res: " + faRes.match_pinyin.join(' '));
                    out->appendPlainText("text_step: " + faRes.text_step.join(' '));
                    out->appendPlainText("pyin_step: " + faRes.pinyin_step.join(' '));
                    if (asr_rectify && !asr_rect_diff.isEmpty())
                        out->appendPlainText("asr_rect_diff: " + asr_rect_diff.join(' '));
                    out->appendPlainText("------------------------");
                    diff_num++;
                }

                Q_ASSERT(faRes.match_text.size() == faRes.match_pinyin.size());
                generate_json(json_folder + QDir::separator() + lab_name + ".json", faRes.match_text.join(' '),
                              faRes.match_pinyin.join(' '));
                success_num++;
            }
        } else {
            miss_lyric.insert(lyric_name);
        }
    }
    if (!miss_lyric.isEmpty())
        out->appendPlainText("miss lyrics: " + miss_lyric.values().join(' '));
    out->appendPlainText(QString::number(diff_num) + " files are different from the original lyrics.");
    out->appendPlainText(QString::number(file_num) + " file in total, " + QString::number(success_num) +
                         " success file.");
}
