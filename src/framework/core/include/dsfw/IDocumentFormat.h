#pragma once

/// @file IDocumentFormat.h
/// @brief Document format identification and registration.

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace dstools {

/// @brief Extensible document format identifier (replaces fixed enum).
class DocumentFormatId {
public:
    constexpr DocumentFormatId() : m_id(-1) {}
    constexpr explicit DocumentFormatId(int id) : m_id(id) {}
    constexpr int id() const { return m_id; }
    constexpr bool isValid() const { return m_id >= 0; }
    constexpr bool operator==(const DocumentFormatId &o) const { return m_id == o.m_id; }
    constexpr bool operator!=(const DocumentFormatId &o) const { return m_id != o.m_id; }
    constexpr bool operator<(const DocumentFormatId &o) const { return m_id < o.m_id; }

private:
    int m_id;
};

/// @brief Register (or look up) a named document format, returning a stable ID.
/// @note Thread-safe: concurrent calls are serialized via an internal mutex.
inline DocumentFormatId registerDocumentFormat(const std::string &name) {
    static std::mutex s_mutex;
    std::lock_guard<std::mutex> lock(s_mutex);
    static int s_nextId = 0;
    static std::unordered_map<std::string, int> s_registry;
    auto it = s_registry.find(name);
    if (it != s_registry.end())
        return DocumentFormatId(it->second);
    int id = s_nextId++;
    s_registry[name] = id;
    return DocumentFormatId(id);
}

/// @brief Abstract interface for document format metadata.
class IDocumentFormat {
public:
    virtual ~IDocumentFormat() = default;

    /// @brief Return the unique format identifier.
    virtual DocumentFormatId id() const = 0;

    /// @brief Return a human-readable format name.
    virtual std::string name() const = 0;

    /// @brief Return the file extensions associated with this format.
    virtual std::vector<std::string> extensions() const = 0;
};

} // namespace dstools
