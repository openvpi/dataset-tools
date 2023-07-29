#include "jpg2p.h"
#include "jpg2p_p.h"
#include <QDebug>

#include "g2pglobal.h"

namespace IKg2p {
    // keep only letters
    static QString filterString(const QString &str) {
        QString words;
        for (const auto &ch : str) {
            auto u = ch.unicode();
            if (u >= 128 || !ch.isLetter()) {
                if (words.isEmpty() || words.back() != ' ') {
                    words.append(' ');
                }
                continue;
            }
            words.append(ch);
        }
        return words;
    }

    // split RomajiStr to List
    static QStringList splitRomaji(const QString &input) {
        QString cleanStr = filterString(input);
        QStringList res;

        // romaji
        QRegExp rx("((?=[^aiueo])[a-z]){0,2}[aiueo]");
        int pos = 0; // 记录匹配位置的变量

        while ((pos = rx.indexIn(cleanStr, pos)) != -1) {
            res.append(cleanStr.mid(pos, rx.matchedLength()));
            pos += rx.matchedLength(); // 更新匹配位置
        }
        return res;
    }

    JpG2pPrivate::JpG2pPrivate() {
    }

    JpG2pPrivate::~JpG2pPrivate() {
    }

    // load jp convert dict
    void JpG2pPrivate::init() {
        auto dict_dir = dictionaryPath();
        loadDict(dict_dir, "kana2romaji.txt", kanaToRomajiMap);

        for (auto it = kanaToRomajiMap.begin(); it != kanaToRomajiMap.end(); ++it) {
            romajiToKanaMap.insert(it.value(), it.key());
        }
    }

    // convert Hiragana to Katakana
    QStringList JpG2pPrivate::convertKana(const QStringList &kanaList, KanaType kanaType) {
        const ushort hiraganaStart = 0x3041;
        const ushort katakanaStart = 0x30A1;

        QStringList convertedList;
        for (const QString &kana : kanaList) {
            QString convertedKana;
            QRegExp rx("[\u3040-\u309F\u30A0-\u30FF]+");
            if (rx.exactMatch(kana)) {
                for (QChar kanaChar : kana) {
                    if (kanaType == KanaType::Hiragana) {
                        // target is Hiragana
                        if (kanaChar >= katakanaStart && kanaChar < katakanaStart + 0x5E) {
                            // Katakana->Hiragana
                            convertedKana += QChar(kanaChar.unicode() - katakanaStart + hiraganaStart);
                        } else {
                            convertedKana += kanaChar;
                        }
                    } else {
                        // target is Katakana
                        if (kanaChar >= hiraganaStart && kanaChar < hiraganaStart + 0x5E) {
                            // Hiragana->Katakana
                            convertedKana += QChar(kanaChar.unicode() + katakanaStart - hiraganaStart);
                        } else {
                            convertedKana += kanaChar;
                        }
                    }
                }
            } else {
                convertedKana = kana;
            }
            convertedList.append(convertedKana);
        }
        return convertedList;
    }

    JpG2p::JpG2p(QObject *parent) : JpG2p(*new JpG2pPrivate(), parent) {
    }

    JpG2p::~JpG2p() {
    }

    QString JpG2p::kana2romaji(const QStringList &kanaList) const {
        Q_D(const JpG2p);
        QStringList inputList = d->convertKana(kanaList, JpG2pPrivate::KanaType::Hiragana);
        QStringList romajiList;
        for (const QString &kana : inputList) {
            romajiList.append(d->kanaToRomajiMap.value(kana, kana));
        }
        return romajiList.join(" ");
    }

    QString JpG2p::kana2romaji(const QString &kanaStr) const {
        QStringList input = splitString(kanaStr);
        return kana2romaji(input);
    }

    JpG2p::JpG2p(JpG2pPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.init();
    }

    QStringList JpG2p::romaji2kana(const QString &romajiStr) const {
        QStringList input = splitRomaji(romajiStr);
        return romaji2kana(input);
    }

    QStringList JpG2p::romaji2kana(const QStringList &romajiList) const {
        Q_D(const JpG2p);
        QStringList kanaList;
        for (const QString &romaji : romajiList) {
            kanaList.append(d->romajiToKanaMap.value(romaji, romaji));
        }
        return kanaList;
    }
}
