#include "ChineseG2p.h"
#include "G2pglobal.h"
#include "ChineseG2p_p.h"

#include <QDebug>
#include <utility>

namespace IKg2p
{
    static const QMap<QChar, QString> numMap = {
        {'0', "零"},
        {'1', "一"},
        {'2', "二"},
        {'3', "三"},
        {'4', "四"},
        {'5', "五"},
        {'6', "六"},
        {'7', "七"},
        {'8', "八"},
        {'9', "九"}
    };

    static QString joinView(const QList<QStringView>& viewList, const QStringView& separator)
    {
        if (viewList.isEmpty())
            return {};

        QString result;
        for (auto it = viewList.begin(); it != viewList.end() - 1; ++it)
        {
            result += it->toString();
            result += separator;
        }
        result += (viewList.end() - 1)->toString();
        return result;
    }

    static QStringList joinViewList(const QList<QStringView>& viewList)
    {
        if (viewList.isEmpty())
            return {};

        QStringList result;
        for (auto it = viewList.begin(); it != viewList.end() - 1; ++it)
        {
            result.append(it->toString());
        }
        result += (viewList.end() - 1)->toString();
        return result;
    }

    // reset pinyin to raw string
    static QStringList resetZH(const QList<QStringView>& input, const QList<QStringView>& res,
                               const QList<int>& positions)
    {
        QList<QStringView> result = input;
        for (int i = 0; i < positions.size(); i++)
        {
            result.replace(positions[i], res.at(i));
        }
        return joinViewList(result);
    }

    // delete elements from the list
    template <class T>
    static inline void removeElements(QList<T>& list, int start, int n)
    {
        list.erase(list.begin() + start, list.begin() + start + n);
    }

    ChineseG2pPrivate::ChineseG2pPrivate(QString language) : m_language(std::move(language))
    {
    }

    ChineseG2pPrivate::~ChineseG2pPrivate() = default;

    static void copyStringViewHash(const QHash<QString, QString>& src, QHash<QStringView, QStringView>& dest)
    {
        for (auto it = src.begin(); it != src.end(); ++it)
        {
            dest.insert(it.key(), it.value());
        }
    }

    static void copyStringViewListHash(const QHash<QString, QStringList>& src,
                                       QHash<QStringView, QList<QStringView>>& dest)
    {
        for (auto it = src.begin(); it != src.end(); ++it)
        {
            QList<QStringView> viewList;
            viewList.reserve(it.value().size());
            for (const auto& item : it.value())
            {
                viewList.push_back(item);
            }
            dest.insert(it.key(), viewList);
        }
    }

    // load zh convert dict
    void ChineseG2pPrivate::init()
    {
        const QString dict_dir = dictionaryPath() + "/" + m_language;

        loadDict(dict_dir, "phrases_map.txt", phrases_map);
        loadDict(dict_dir, "phrases_dict.txt", phrases_dict);
        loadDict(dict_dir, "user_dict.txt", phrases_dict);
        loadDict(dict_dir, "word.txt", word_dict);
        loadDict(dict_dir, "trans_word.txt", trans_dict);

        copyStringViewHash(phrases_map, phrases_map2);
        copyStringViewListHash(phrases_dict, phrases_dict2);
        copyStringViewListHash(word_dict, word_dict2);
        copyStringViewHash(trans_dict, trans_dict2);
    }

    // get all chinese characters and positions in the list
    void ChineseG2pPrivate::zhPosition(const QList<QStringView>& input, QList<QStringView>& res, QList<int>& positions,
                                  const bool& convertNum) const
    {
        for (int i = 0; i < input.size(); ++i)
        {
            const auto& item = input.at(i);
            if (item.isEmpty())
                continue;

            if (word_dict2.contains(item) || trans_dict2.contains(item))
            {
                res.append(item);
                positions.append(i);
                continue;
            }

            if (!convertNum)
            {
                continue;
            }

            auto it = numMap.find(input.at(i).front());
            if (it != numMap.end())
            {
                res.append(it.value());
                positions.append(i);
            }
        }
    }


    ChineseG2p::ChineseG2p(QString language, QObject* parent) : ChineseG2p(*new ChineseG2pPrivate(std::move(language)), parent)
    {
    }

    ChineseG2p::~ChineseG2p() = default;

    QStringList ChineseG2p::hanziToPinyin(const QString& input, bool tone, bool convertNum, errorType error)
    {
        return hanziToPinyin(splitString(input), tone, convertNum, error);
    }

    QStringList ChineseG2p::hanziToPinyin(const QStringList& input, bool tone, bool convertNum, errorType error)
    {
        QList<QStringView> inputList;
        inputList.reserve(input.size());
        for (const auto& item : input)
        {
            inputList.push_back(item);
        }
        return hanziToPinyin(inputList, tone, convertNum, error);
    }

    QStringList ChineseG2p::hanziToPinyin(const QList<QStringView>& input, bool tone, bool convertNum, errorType error)
    {
        Q_D(const ChineseG2p);
        QList<QStringView> inputList;
        QList<int> inputPos;

        // get char&pos in dict
        d->zhPosition(input, inputList, inputPos, convertNum);

        // Alloc 1
        QString cleanInputRaw = joinView(inputList, QString());
        QStringView cleanInput(cleanInputRaw);

        QList<QStringView> result;
        int cursor = 0;
        while (cursor < inputList.size())
        {
            // get char
            const QStringView& raw_current_char = inputList.at(cursor);
            QStringView current_char;
            current_char = d->tradToSim(raw_current_char);

            // not in dict
            if (d->word_dict2.find(current_char) == d->word_dict2.end())
            {
                result.append(current_char);
                cursor++;
                continue;
            }

            // is polypropylene
            if (!d->isPolyphonic(current_char))
            {
                result.append(d->getDefaultPinyin(current_char));
                cursor++;
            }
            else
            {
                bool found = false;
                for (int length = 4; length >= 2; length--)
                {
                    if (cursor + length <= inputList.size())
                    {
                        // cursor: 地, subPhrase: 地久天长
                        QStringView sub_phrase = cleanInput.mid(cursor, length);
                        if (d->phrases_dict2.find(sub_phrase) != d->phrases_dict2.end())
                        {
                            result.append(d->phrases_dict2[sub_phrase]);
                            cursor += length;
                            found = true;
                        }

                        if (found)
                        {
                            break;
                        }

                        if (cursor >= 1)
                        {
                            // cursor: 重, subPhrase_1: 语重心长
                            QStringView sub_phrase_1 = cleanInput.mid(cursor - 1, length);
                            auto it = d->phrases_dict2.find(sub_phrase_1);
                            if (it != d->phrases_dict2.end())
                            {
                                result.removeAt(result.size() - 1);
                                result.append(it.value());
                                cursor += length - 1;
                                found = true;
                            }
                        }
                    }

                    if (found)
                    {
                        break;
                    }

                    if (cursor + 1 >= length && cursor + 1 <= inputList.size())
                    {
                        // cursor: 好, xSubPhrase: 各有所好
                        QStringView x_sub_phrase = cleanInput.mid(cursor + 1 - length, length);
                        auto it = d->phrases_dict2.find(x_sub_phrase);
                        if (it != d->phrases_dict2.end())
                        {
                            // overwrite pinyin
                            removeElements(result, cursor + 1 - length, length - 1);
                            result.append(it.value());
                            cursor += 1;
                            found = true;
                        }
                    }

                    if (found)
                    {
                        break;
                    }

                    if (cursor + 2 >= length && cursor + 2 <= inputList.size())
                    {
                        // cursor: 好, xSubPhrase: 叶公好龙
                        QStringView x_sub_phrase_1 = cleanInput.mid(cursor + 2 - length, length);
                        auto it = d->phrases_dict2.find(x_sub_phrase_1);
                        if (it != d->phrases_dict2.end())
                        {
                            // overwrite pinyin
                            removeElements(result, cursor + 2 - length, length - 2);
                            result.append(it.value());
                            cursor += 2;
                            found = true;
                        }
                    }

                    if (found)
                    {
                        break;
                    }
                }

                // not found, use default pinyin
                if (!found)
                {
                    result.append(d->getDefaultPinyin(current_char));
                    cursor++;
                }
            }
        }

        if (!tone)
        {
            for (QStringView& item : result)
            {
                if (item.back().isDigit())
                {
                    item.chop(1);
                }
            }
        }

        // Alloc 2
        if (error == errorType::Ignore)
        {
            return joinViewList(result);
        }
        return resetZH(input, result, inputPos);
    }

    ChineseG2p::ChineseG2p(ChineseG2pPrivate& d, QObject* parent) : QObject(parent), d_ptr(&d)
    {
        d.q_ptr = this;

        d.init();
    }

    QString ChineseG2p::tradToSim(const QString& text) const
    {
        Q_D(const ChineseG2p);
        return d->tradToSim(text).toString();
    }

    bool ChineseG2p::isPolyphonic(const QString& text) const
    {
        Q_D(const ChineseG2p);
        return d->isPolyphonic(text);
    }

    QStringList ChineseG2p::getDefaultPinyin(const QString& text, bool tone) const
    {
        Q_D(const ChineseG2p);
        QStringList res = d->word_dict.value(d->tradToSim(text).toString(), {});
        if (!tone)
        {
            for (QString& item : res)
            {
                if (item.back().isDigit())
                {
                    item.chop(1);
                }
            }
            res.removeDuplicates();
        }
        return res;
    }
}
