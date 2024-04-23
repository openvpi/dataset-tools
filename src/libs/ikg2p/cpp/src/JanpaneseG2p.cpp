#include "JapaneseG2p.h"
#include "JapaneseG2p_p.h"

#include "G2pglobal.h"

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#  include <QtCore5Compat>
#endif

namespace IKg2p
{
    // keep only letters
    static QString filterString(const QString& str)
    {
        QString words;
        for (const auto& ch : str)
        {
            const auto u = ch.unicode();
            if (u >= 128 || !ch.isLetter())
            {
                if (words.isEmpty() || words.back() != ' ')
                {
                    words.append(' ');
                }
                continue;
            }
            words.append(ch);
        }
        return words;
    }

    // split RomajiStr to List
    static QStringList splitRomaji(const QString& input)
    {
        const QString cleanStr = filterString(input);
        QStringList res;

        // romaji
        const QRegExp rx("((?=[^aiueo])[a-z]){0,2}[aiueo]");
        int pos = 0; // 记录匹配位置的变量

        while ((pos = rx.indexIn(cleanStr, pos)) != -1)
        {
            res.append(cleanStr.mid(pos, rx.matchedLength()));
            pos += rx.matchedLength(); // 更新匹配位置
        }
        return res;
    }

    JpG2pPrivate::JpG2pPrivate()
    {
    }

    JpG2pPrivate::~JpG2pPrivate() = default;

    // load jp convert dict
    void JpG2pPrivate::init()
    {
        const auto dict_dir = dictionaryPath();
        loadDict(dict_dir, "kana2romaji.txt", kanaToRomajiMap);

        for (auto it = kanaToRomajiMap.begin(); it != kanaToRomajiMap.end(); ++it)
        {
            romajiToKanaMap.insert(it.value(), it.key());
        }
    }

    // convert Hiragana to Katakana
    QStringList JpG2pPrivate::convertKana(const QStringList& kanaList, KanaType kanaType)
    {
        const ushort hiraganaStart = 0x3041;
        const ushort katakanaStart = 0x30A1;

        QStringList convertedList;
        for (const QString& kana : kanaList)
        {
            QString convertedKana;
            QRegExp rx("[\u3040-\u309F\u30A0-\u30FF]+");
            if (rx.exactMatch(kana))
            {
                for (QChar kanaChar : kana)
                {
                    if (kanaType == KanaType::Hiragana)
                    {
                        // target is Hiragana
                        if (kanaChar >= katakanaStart && kanaChar < QChar(katakanaStart + 0x5E))
                        {
                            // Katakana->Hiragana
                            convertedKana +=
                                QChar(kanaChar.unicode() - katakanaStart + hiraganaStart);
                        }
                        else
                        {
                            convertedKana += kanaChar;
                        }
                    }
                    else
                    {
                        // target is Katakana
                        if (kanaChar >= hiraganaStart && kanaChar < QChar(hiraganaStart + 0x5E))
                        {
                            // Hiragana->Katakana
                            convertedKana +=
                                QChar(kanaChar.unicode() + katakanaStart - hiraganaStart);
                        }
                        else
                        {
                            convertedKana += kanaChar;
                        }
                    }
                }
            }
            else
            {
                convertedKana = kana;
            }
            convertedList.append(convertedKana);
        }
        return convertedList;
    }

    JapaneseG2p::JapaneseG2p(QObject* parent) : JapaneseG2p(*new JpG2pPrivate(), parent)
    {
    }

    JapaneseG2p::~JapaneseG2p() = default;

    QStringList JapaneseG2p::kanaToRomaji(const QStringList& kanaList, const bool& doubleWrittenSokuon) const
    {
        Q_D(const JpG2p);
        QStringList inputList = IKg2p::JpG2pPrivate::convertKana(kanaList, JpG2pPrivate::KanaType::Hiragana);
        QStringList romajiList;
        for (const QString& kana : inputList)
        {
            if (kana != "゜" && kana != "ー")
            {
                romajiList.append(d->kanaToRomajiMap.value(kana, kana));
            }
        }

        for (int i = 0; i < romajiList.size() - 1 && doubleWrittenSokuon; ++i)
        {
            const QChar nextChar = d->romajiToKanaMap.value(romajiList[i + 1], " ").at(0);
            if (romajiList[i] == "cl" && isKana(nextChar) &&
                !QString("あいうえおアイウエオっんを").contains(nextChar))
            {
                romajiList[i + 1].prepend(romajiList[i + 1][0]);
                romajiList.removeAt(i);
            }
        }
        return romajiList;
    }

    QStringList JapaneseG2p::kanaToRomaji(const QString& kanaStr, const bool& doubleWrittenSokuon) const
    {
        return kanaToRomaji(splitString(kanaStr), doubleWrittenSokuon);
    }

    JapaneseG2p::JapaneseG2p(JpG2pPrivate& d, QObject* parent) : QObject(parent), d_ptr(&d)
    {
        d.q_ptr = this;

        d.init();
    }

    QStringList JapaneseG2p::romajiToKana(const QString& romajiStr) const
    {
        const QStringList input = splitRomaji(romajiStr);
        return romajiToKana(input);
    }

    QStringList JapaneseG2p::romajiToKana(const QStringList& romajiList) const
    {
        Q_D(const JpG2p);
        QStringList kanaList;
        for (const QString& romaji : romajiList)
        {
            kanaList.append(d->romajiToKanaMap.value(romaji, romaji));
        }
        return kanaList;
    }
}
