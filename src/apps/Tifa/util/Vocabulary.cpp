#include "Vocabulary.h"

#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

#include "PathUtils.h"

using json = nlohmann::json;

namespace Tifa {

    bool Vocabulary::load(const std::filesystem::path &path, std::string &error) {
        std::ifstream stream(path);
        if (!stream.is_open()) {
            error = "Failed to open vocabulary file: " + pathToUtf8(path);
            return false;
        }
        json data;
        try {
            data = json::parse(stream);
        } catch (const std::exception &e) {
            error = "Failed to parse vocabulary file: " + pathToUtf8(path) + " (" + e.what() + ")";
            return false;
        }
        if (!data.contains("symbols") || !data["symbols"].is_object()) {
            error = "Vocabulary file has no 'symbols' object: " + pathToUtf8(path);
            return false;
        }

        m_symbolToId.clear();
        m_idToSymbols.clear();
        m_maxId = -1;
        for (const auto &[symbol, id] : data["symbols"].items()) {
            if (!id.is_number_integer()) {
                error = "Vocabulary entry '" + symbol + "' is not an integer ID.";
                return false;
            }
            const int token = id.get<int>();
            m_symbolToId[symbol] = token;
            m_idToSymbols[token].push_back(symbol);
            m_maxId = std::max(m_maxId, token);
        }
        if (m_symbolToId.empty()) {
            error = "Vocabulary file is empty: " + pathToUtf8(path);
            return false;
        }
        return true;
    }

    int Vocabulary::vocabSize() const {
        if (m_maxId < 0) {
            return kNumReservedTokens + 1;
        }
        return std::max(kNumReservedTokens, m_maxId) + 1;
    }

    bool Vocabulary::resolve(const std::string &symbol, const std::vector<std::string> &languages, int &token,
                             std::string &resolvedSymbol) const {
        const auto it = m_symbolToId.find(symbol);
        if (it != m_symbolToId.end()) {
            token = it->second;
            resolvedSymbol = symbol;
            return true;
        }
        // A symbol that already carries a language prefix is never re-qualified.
        if (symbol.find('/') != std::string::npos) {
            return false;
        }
        for (const auto &language : languages) {
            const std::string qualified = language + "/" + symbol;
            const auto qualifiedIt = m_symbolToId.find(qualified);
            if (qualifiedIt != m_symbolToId.end()) {
                token = qualifiedIt->second;
                resolvedSymbol = qualified;
                return true;
            }
        }
        return false;
    }

    std::vector<std::string> Vocabulary::decode(int token) const {
        const auto it = m_idToSymbols.find(token);
        if (it == m_idToSymbols.end()) {
            return {};
        }
        return it->second;
    }

} // namespace Tifa
