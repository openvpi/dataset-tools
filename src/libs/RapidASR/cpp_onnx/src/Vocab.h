#ifndef VOCAB_H
#define VOCAB_H

#include <string>
#include <vector>

class Vocab {
  private:
    std::vector<std::string> vocab;
    bool isChinese(std::string ch);
    bool isEnglish(std::string ch);

  public:
    Vocab(const char *filename);
    ~Vocab();
    int size();
    std::string vector2string(std::vector<int> in);
    std::string vector2stringV2(std::vector<int> in);
};

#endif
