#include "zhg2p.h"
#include "g2pglobal.h"
#include "zhg2p_p.h"

#include <QDebug>
#include <iostream>
#include <regex>
#include <sstream>
#include <utility>

namespace IKg2p {
    static const std::unordered_map<std::string, std::string> numMap = {
        {"0", "零"},
        {"1", "一"},
        {"2", "二"},
        {"3", "三"},
        {"4", "四"},
        {"5", "五"},
        {"6", "六"},
        {"7", "七"},
        {"8", "八"},
        {"9", "九"}
    };

    std::string concatenateStrings(const std::vector<std::string> &stringVector) {
        std::string result;

        for (const std::string &str : stringVector) {
            result += " " + str;
        }

        return result;
    }

    // reset pinyin to raw string
    static std::string resetZH(const std::vector<std::string> &input, const std::vector<std::string> &res,
                               std::vector<int> &positions) {
        std::vector<std::string> result = input;
        for (int i = 0; i < positions.size(); i++) {
            result[positions[i]] = res[i];
        }

        return concatenateStrings(result);
    }

    // split the value of pinyin dictionary
    static void addString(const std::string &text, std::vector<std::string> &res) {
        std::istringstream iss(text);

        // 使用 std::getline 以空格为分隔符进行分割
        std::string token;
        while (std::getline(iss, token, ' ')) {
            res.push_back(token);
        }
    }

    // delete elements from the list
    static inline void removeElements(std::vector<std::string> &list, int start, int n) {
        list.erase(list.begin() + start, list.begin() + start + n);
    }

    ZhG2pPrivate::ZhG2pPrivate(QString language) : m_language(std::move(language)) {
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

    bool ZhG2pPrivate::isPolyphonic(const std::string &text) const {
        return phrases_map.find(text) != phrases_map.end();
    }

    std::string ZhG2pPrivate::tradToSim(const std::string &text) const {
        if (trans_dict.find(text) != trans_dict.end()) {
            return trans_dict.find(text)->second;
        } else {
            return text;
        }
    }

    std::string ZhG2pPrivate::getDefaultPinyin(const std::string &text) const {
        if (word_dict.find(text) != word_dict.end()) {
            return word_dict.find(text)->second;
        } else {
            return text;
        }
    }

    // get all chinese characters and positions in the list
    void ZhG2pPrivate::zhPosition(const std::vector<std::string> &input, std::vector<std::string> &res,
                                  std::vector<int> &positions, bool covertNum) const {
        for (int i = 0; i < input.size(); i++) {
            if (word_dict.find(input.at(i)) != word_dict.end() || trans_dict.find(input.at(i)) != trans_dict.end()) {
                res.push_back(input.at(i));
                positions.push_back(i);
            } else if (covertNum && numMap.find(input.at(i)) != numMap.end()) {
                res.push_back(numMap.find(input.at(i))->second);
                positions.push_back(i);
            }
        }
    }


    ZhG2p::ZhG2p(QString language, QObject *parent) : ZhG2p(*new ZhG2pPrivate(std::move(language)), parent) {
    }

    ZhG2p::~ZhG2p() {
    }

    QString ZhG2p::convert(const QString &input, bool tone, bool covertNum) {
        return convert(splitString(input), tone, covertNum);
    }

    QString ZhG2p::convert(const std::vector<std::string> &input, bool tone, bool covertNum) {
        Q_D(const ZhG2p);
        std::vector<std::string> inputList;
        std::vector<int> inputPos;
        // get char&pos in dict
        d->zhPosition(input, inputList, inputPos, covertNum);
        std::vector<std::string> result;

        int cursor = 0;
        while (cursor < inputList.size()) {
            // get char
            const std::string &raw_current_char = inputList.at(cursor);
            std::string current_char;
            current_char = d->tradToSim(raw_current_char);

            // not in dict
            if (d->word_dict.find(current_char) == d->word_dict.end()) {
                result.push_back(current_char);
                cursor++;
                continue;
            }

            //        qDebug() << current_char << isPolyphonic(current_char);
            // is polypropylene
            if (!d->isPolyphonic(current_char)) {
                result.push_back(d->getDefaultPinyin(current_char));
                cursor++;
            } else {
                bool found = false;
                for (int length = 4; length >= 2 && !found; length--) {
                    if (cursor + length <= inputList.size()) {
                        // cursor: 地, subPhrase: 地久天长
                        std::string sub_phrase = concatenateStrings(
                            std::vector<std::string>{inputList.begin() + cursor, inputList.begin() + cursor + length});
                        if (d->phrases_dict.find(sub_phrase) != d->phrases_dict.end()) {
                            addString(d->phrases_dict.find(sub_phrase)->second, result);
                            cursor += length;
                            found = true;
                        }

                        if (cursor >= 1 && !found) {
                            // cursor: 重, subPhrase_1: 语重心长
                            std::string sub_phrase_1 = concatenateStrings(std::vector<std::string>{
                                inputList.begin() + cursor - 1, inputList.begin() + cursor + length - 1});
                            if (d->phrases_dict.find(sub_phrase_1) != d->phrases_dict.end()) {
                                result.erase(result.end() - 1); // remove last element
                                addString(d->phrases_dict.find(sub_phrase_1)->second, result);
                                cursor += length - 1;
                                found = true;
                            }
                        }
                    }

                    if (cursor + 1 >= length && !found && cursor + 1 <= inputList.size()) {
                        // cursor: 好, xSubPhrase: 各有所好
                        std::string x_sub_phrase = concatenateStrings(std::vector<std::string>{
                            inputList.begin() + cursor + 1 - length, inputList.begin() + cursor + 1});
                        if (d->phrases_dict.find(x_sub_phrase) != d->phrases_dict.end()) {
                            // overwrite pinyin
                            removeElements(result, cursor + 1 - length, length - 1);
                            addString(d->phrases_dict.find(x_sub_phrase)->second, result);
                            cursor += 1;
                            found = true;
                        }
                    }

                    if (cursor + 2 >= length && !found && cursor + 2 <= inputList.size()) {
                        // cursor: 好, xSubPhrase: 叶公好龙
                        std::string x_sub_phrase_1 = concatenateStrings(std::vector<std::string>{
                            inputList.begin() + cursor + 2 - length, inputList.begin() + cursor + 2});
                        if (d->phrases_dict.find(x_sub_phrase_1) != d->phrases_dict.end()) {
                            // overwrite pinyin
                            removeElements(result, cursor + 2 - length, length - 2);
                            addString(d->phrases_dict.find(x_sub_phrase_1)->second, result);
                            cursor += 2;
                            found = true;
                        }
                    }
                }

                // not found, use default pinyin
                if (!found) {
                    result.push_back(d->getDefaultPinyin(current_char));
                    cursor++;
                }
            }
        }

        auto exp = std::regex("[^a-z]");
        if (!tone) {
            for (std::string &item : result) {
                item = std::regex_replace(item, exp, "");
            }
        }

        return QString::fromStdString(resetZH(inputList, result, inputPos));
    }

    ZhG2p::ZhG2p(ZhG2pPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.init();
    }
    QString ZhG2p::convert(const QStringList &input, bool tone, bool covertNum) {
        return QString();
    }

}
