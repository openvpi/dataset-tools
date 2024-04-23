def is_letter(c):
    return ('a' <= c <= 'z') or ('A' <= c <= 'Z')


def is_hanzi(c):
    return 0x4e00 <= ord(c) <= 0x9fa5


def is_kana(c):
    return (0x3040 <= ord(c) <= 0x309F) or (0x30A0 <= ord(c) <= 0x30FF)


def is_special_kana(c):
    special_kana = "ャュョゃゅょァィゥェォぁぃぅぇぉ"
    return c in special_kana


class ZhG2p:
    def __init__(self, language):
        self.PhrasesMap = {}
        self.TransDict = {}
        self.WordDict = {}
        self.PhrasesDict = {}

        if language == "mandarin":
            dict_dir = "Dicts/mandarin"
        else:
            dict_dir = "Dicts/cantonese"

        self.load_dict(dict_dir, "phrases_map.txt", self.PhrasesMap)
        self.load_dict_list(dict_dir, "phrases_dict.txt", self.PhrasesDict)
        self.load_dict_list(dict_dir, "user_dict.txt", self.PhrasesDict)
        self.load_dict_list(dict_dir, "word.txt", self.WordDict)
        self.load_dict(dict_dir, "trans_word.txt", self.TransDict)

    @staticmethod
    def load_dict(_dict_dir, _file_name, _result_map):
        dict_path = _dict_dir + "/" + _file_name
        with open(dict_path, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if line:
                    key, value = line.split(':')
                    _result_map[key] = value

    @staticmethod
    def load_dict_list(_dict_dir, _file_name, _result_map):
        dict_path = _dict_dir + "/" + _file_name
        with open(dict_path, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if line:
                    key, value = line.split(':')
                    _result_map[key] = value.split(' ')

    NumMap = {
        "0": "零",
        "1": "一",
        "2": "二",
        "3": "三",
        "4": "四",
        "5": "五",
        "6": "六",
        "7": "七",
        "8": "八",
        "9": "九"
    }

    @staticmethod
    def reset_zh(_input, res, positions):
        result = _input.copy()
        for i, pos in enumerate(positions):
            result[pos] = res[i]
        return " ".join(result)

    @staticmethod
    def remove_elements(lst, start, n):
        if 0 <= start < len(lst) and n > 0:
            count_to_remove = min(n, len(lst) - start)
            del lst[start:start + count_to_remove]

    @staticmethod
    def split_string_no_re(input_str):
        res = []
        pos = 0

        while pos < len(input_str):
            current_char = input_str[pos]

            if is_letter(current_char):
                start = pos
                while pos < len(input_str) and is_letter(input_str[pos]):
                    pos += 1
                res.append(input_str[start:pos])
            elif is_hanzi(current_char) or current_char.isdigit():
                res.append(input_str[pos])
                pos += 1
            elif is_kana(current_char):
                length = 2 if pos + 1 < len(input_str) and is_special_kana(input_str[pos + 1]) else 1
                res.append(input_str[pos:pos + length])
                pos += length
            else:
                pos += 1

        return res

    def convert(self, _input, tone=False, convert_num=False):
        return self.convert_list(self.split_string_no_re(_input), tone, convert_num)

    def zh_position(self, _input, res, positions, convert_num):
        for i, val in enumerate(_input):
            if val in self.WordDict or val in self.TransDict:
                res.append(val)
                positions.append(i)
            elif convert_num and val in self.NumMap:
                res.append(self.NumMap[val])
                positions.append(i)

    def convert_list(self, _input, tone=False, convert_num=False):
        input_list = []
        input_pos = []
        self.zh_position(_input, input_list, input_pos, convert_num)
        clean_input = ''.join(input_list)
        result = []
        cursor = 0

        while cursor < len(input_list):
            raw_current_char = input_list[cursor]
            current_char = self.trad_to_sim(raw_current_char)

            if current_char not in self.WordDict:
                result.append(current_char)
                cursor += 1
                continue

            if not self.is_polyphonic(current_char):
                result.append(self.get_default_pinyin(current_char))
                cursor += 1
            else:
                found = False
                for length in range(4, 1, -1):
                    if cursor + length <= len(input_list):
                        sub_phrase = clean_input[cursor:cursor + length]
                        if sub_phrase in self.PhrasesDict:
                            result.extend(self.PhrasesDict[sub_phrase])
                            cursor += length
                            found = True

                        if cursor >= 1 and not found:
                            sub_phrase_1 = clean_input[cursor - 1:cursor + length - 1]
                            if sub_phrase_1 in self.PhrasesDict:
                                result = result[:-1]
                                result.extend(self.PhrasesDict[sub_phrase_1])
                                cursor += length - 1
                                found = True

                    if 0 <= cursor + 1 - length < cursor + 1 and not found and cursor < len(input_list):
                        x_sub_phrase = clean_input[cursor + 1 - length:cursor + 1]
                        if x_sub_phrase in self.PhrasesDict:
                            self.remove_elements(result, cursor + 1 - length, length - 1)
                            result.extend(self.PhrasesDict[x_sub_phrase])
                            cursor += 1
                            found = True

                    if 0 <= cursor + 2 - length < cursor + 2 and not found and cursor < len(input_list):
                        x_sub_phrase = clean_input[cursor + 2 - length:cursor + 2]
                        if x_sub_phrase in self.PhrasesDict:
                            self.remove_elements(result, cursor + 2 - length, length - 1)
                            result.extend(self.PhrasesDict[x_sub_phrase])
                            cursor += 2
                            found = True

                if not found:
                    result.append(self.get_default_pinyin(current_char))
                    cursor += 1

        if not tone:
            result = [x[:-1] if x[-1].isdigit() else x for x in result]

        return self.reset_zh(_input, result, input_pos)

    def is_polyphonic(self, text):
        return text in self.PhrasesMap

    def trad_to_sim(self, text):
        return self.TransDict[text] if text in self.TransDict else text

    def get_default_pinyin(self, text):
        return self.WordDict[text][0] if text in self.WordDict else None

    def get_all_pinyin(self, text):
        return self.WordDict[text] if text in self.WordDict else None