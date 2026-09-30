#ifndef TIFA_VOCABULARY_H
#define TIFA_VOCABULARY_H

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Tifa {

    // Reserved token IDs of the TIFA vocabulary.
    constexpr int kPadToken = 0;
    constexpr int kMaskToken = 1;
    constexpr int kSpaceToken = 2;
    constexpr int kNumReservedTokens = 3;

    /**
     * Symbol table of an exported TIFA model (vocabulary.json).
     *
     * The file format is a single object with a "symbols" member mapping
     * symbols to token IDs, where a symbol is either a global symbol such as
     * "AP" or a language-qualified phoneme such as "zh/zh". Multiple symbols
     * may share one ID when they were merged during training.
     */
    class Vocabulary {
    public:
        bool load(const std::filesystem::path &path, std::string &error);
        bool valid() const {
            return !m_symbolToId.empty();
        }

        /** Output channels of the token classifier, matching Vocabulary::vocab_size. */
        int vocabSize() const;

        /**
         * Resolve a phoneme symbol. The symbol itself is looked up first, then
         * its language-qualified forms in the given priority order.
         *
         * @param symbol Phoneme symbol produced by G2P.
         * @param languages Language tags to try as prefixes.
         * @param token Receives the token ID when the symbol is known.
         * @param resolvedSymbol Receives the exact symbol that matched.
         * @return True when the symbol is encodable by this vocabulary.
         */
        bool resolve(const std::string &symbol, const std::vector<std::string> &languages, int &token,
                     std::string &resolvedSymbol) const;

        /** All symbols that share a token ID; empty for unknown tokens. */
        std::vector<std::string> decode(int token) const;

    private:
        std::unordered_map<std::string, int> m_symbolToId;
        std::unordered_map<int, std::vector<std::string>> m_idToSymbols;
        int m_maxId = -1;
    };

} // namespace Tifa

#endif // TIFA_VOCABULARY_H
