#include "zhg2p.h"
#include "zhg2p_p.h"

#include <QDebug>

#include "g2pglobal.h"

namespace IKg2p {
    // reset pinyin to raw string
    static QString resetZH(const QStringList &input, const QStringList &res, QList<int> &positions) {
        QStringList result = input;
        for (int i = 0; i < positions.size(); i++) {
            result.replace(positions[i], res.at(i));
        }

        return result.join(" ");
    }

    // split the value of pinyin dictionary
    static void addString(const QString &text, QStringList &res) {
        QStringList temp = text.split(" ");
        for (auto &pinyin : qAsConst(temp)) {
            res.append(pinyin);
        }
    }

    // delete elements from the list
    static inline void removeElements(QStringList &list, int start, int n) {
        list.erase(list.begin() + start, list.begin() + start + n);
    }

    ZhG2pPrivate::ZhG2pPrivate(QString language) : m_language(language) {
    }

    ZhG2pPrivate::~ZhG2pPrivate() {
    }

    // load zh convert dict
    void ZhG2pPrivate::init() {
        QString dict_dir;
        if (m_language == "mandarin") {
            dict_dir = dictionaryPath() + "/mandarin";
        } else {
            dict_dir = dictionaryPath() + "/cantonese";
        }

        loadDict(dict_dir, "phrases_map.txt", phrases_map);
        loadDict(dict_dir, "phrases_dict.txt", phrases_dict);
        loadDict(dict_dir, "user_dict.txt", phrases_dict);
        loadDict(dict_dir, "word.txt", word_dict);
        loadDict(dict_dir, "trans_word.txt", trans_dict);
    }

    bool ZhG2pPrivate::isPolyphonic(const QString &text) const {
        return phrases_map.contains(text);
    }

    QString ZhG2pPrivate::tradToSim(const QString &text) const {
        return trans_dict.value(text, text);
    }

    QString ZhG2pPrivate::getDefaultPinyin(const QString &text) const {
        return word_dict.value(text, {});
    }

    // get all chinese characters and positions in the list
    void ZhG2pPrivate::zhPosition(const QStringList &input, QStringList &res, QList<int> &positions) const {
        for (int i = 0; i < input.size(); i++) {
            if (word_dict.find(input.at(i)) != word_dict.end() || trans_dict.find(input.at(i)) != trans_dict.end()) {
                res.append(input.mid(i, 1));
                positions.append(i);
            }
        }
    }


    ZhG2p::ZhG2p(QString language, QObject *parent) : ZhG2p(*new ZhG2pPrivate(language), parent) {
    }

    ZhG2p::~ZhG2p() {
    }

    QString ZhG2p::convert(const QString &input) {
        return convert(splitString(input));
    }

    QString ZhG2p::convert(const QStringList &input) {
        Q_D(const ZhG2p);
        //    qDebug() << input;
        QStringList inputList;
        QList<int> inputPos;
        // get char&pos in dict
        d->zhPosition(input, inputList, inputPos);
        QStringList result;
        int cursor = 0;
        while (cursor < inputList.size()) {
            // get char
            const QString &raw_current_char = inputList.at(cursor);
            QString current_char;
            current_char = d->tradToSim(raw_current_char);
            //        qDebug() << raw_current_char << raw_current_char;
            // not in dict
            if (d->word_dict.find(current_char) == d->word_dict.end()) {
                result.append(current_char);
                cursor++;
                continue;
            }

            //        qDebug() << current_char << isPolyphonic(current_char);
            // is polypropylene
            if (!d->isPolyphonic(current_char)) {
                result.append(d->getDefaultPinyin(current_char));
                cursor++;
            } else {
                bool found = false;
                for (int length = 4; length >= 2 && !found; length--) {
                    // forward lookup
                    QString sub_phrase = inputList.mid(cursor, length).join("");
                    if (d->phrases_dict.find(sub_phrase) != d->phrases_dict.end()) {
                        //                    qDebug() << "get:sub_phrase:" << sub_phrase << phrases_dict[sub_phrase];
                        addString(d->phrases_dict[sub_phrase], result);
                        cursor += length;
                        found = true;
                    } else if ((cursor + 1) >= length) {
                        // reverse lookup
                        QString x_sub_phrase = inputList.mid(cursor + 1 - length, length).join("");
                        if (d->phrases_dict.find(x_sub_phrase) != d->phrases_dict.end()) {
                            //                        qDebug() << "get:x_sub_phrase:" << x_sub_phrase <<
                            //                        phrases_dict[x_sub_phrase];
                            int pos = x_sub_phrase.indexOf(current_char);
                            // overwrite pinyin
                            removeElements(result, cursor + 1 - length, pos);
                            addString(d->phrases_dict[x_sub_phrase], result);
                            cursor += 1;
                            found = true;
                        }
                    }
                }
                // not found, use default pinyin
                if (!found) {
                    result.append(d->getDefaultPinyin(current_char));
                    cursor++;
                }
            }
        }

        return resetZH(input, result, inputPos);
    }

    ZhG2p::ZhG2p(ZhG2pPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.init();
    }

}
