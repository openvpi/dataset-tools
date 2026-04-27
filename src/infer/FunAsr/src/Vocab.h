#ifndef VOCAB_H
#define VOCAB_H

#include <string>
#include <vector>

namespace FunAsr {
    class Vocab {
    private:
        std::vector<std::string> vocab;
        static bool isChinese(const std::string &ch);

    public:
        explicit Vocab(const char *filename);
#ifdef _WIN32
        explicit Vocab(const wchar_t *filename);
#endif
        ~Vocab();
        int size() const;
        std::string vector2stringV2(const std::vector<int> &in) const;
    };
}
#endif
