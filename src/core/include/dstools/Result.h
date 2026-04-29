#pragma once

#include <QString>
#include <utility>
#include <variant>

namespace dstools {

/// Lightweight result type for operations that can fail.
/// Similar to std::expected<T, QString> (C++23), but usable with C++17.
///
/// Usage:
///   Result<int> parseNumber(const QString &s);
///   auto result = parseNumber("42");
///   if (result) { use(*result); }
///   else { report(result.error()); }
///
/// For void results, use Result<void>.
template <typename T>
class Result {
public:
    /// Construct a success result.
    Result(T value) : m_data(std::move(value)) {} // NOLINT(google-explicit-constructor)

    /// Construct a failure result.
    static Result fail(const QString &error) {
        Result r;
        r.m_data = error;
        return r;
    }

    /// Check if the result is successful.
    explicit operator bool() const { return std::holds_alternative<T>(m_data); }
    bool hasValue() const { return std::holds_alternative<T>(m_data); }

    /// Get the value (undefined if failed).
    const T &value() const & { return std::get<T>(m_data); }
    T &value() & { return std::get<T>(m_data); }
    T &&value() && { return std::get<T>(std::move(m_data)); }

    const T &operator*() const & { return value(); }
    T &operator*() & { return value(); }
    T &&operator*() && { return std::move(*this).value(); }

    const T *operator->() const { return &value(); }
    T *operator->() { return &value(); }

    /// Get the error message (empty if successful).
    QString error() const {
        if (auto *e = std::get_if<QString>(&m_data))
            return *e;
        return {};
    }

private:
    Result() = default;
    std::variant<T, QString> m_data;
};

/// Specialization for void results (success/fail with no value).
template <>
class Result<void> {
public:
    /// Construct a success result.
    Result() : m_error() {}

    /// Construct a failure result.
    static Result fail(const QString &error) {
        Result r;
        r.m_error = error;
        return r;
    }

    explicit operator bool() const { return m_error.isEmpty(); }
    bool hasValue() const { return m_error.isEmpty(); }
    QString error() const { return m_error; }

private:
    QString m_error;
};

} // namespace dstools
