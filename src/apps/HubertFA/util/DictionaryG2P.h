#ifndef DICTIONARY_G2P_H
#define DICTIONARY_G2P_H

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace HFA {

    class DictionaryG2P {
    public:
        DictionaryG2P(const std::string &dictionaryPath, const std::string &language);

        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<int>>
            convert(const std::string &inputText, const std::string &language);

    private:
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>*> dictionary_;
        std::string language_;
    };

} // namespace HFA
#endif DICTIONARY_G2P_H