#pragma once
#include <QString>
#include <optional>
#include <utility>

namespace dstools {

/// Lightweight result type for operations that may fail.
template<typename T>
struct Result {
    std::optional<T> value;
    QString error;

    [[nodiscard]] bool ok() const { return value.has_value(); }
    [[nodiscard]] const T &get() const { return value.value(); }
    [[nodiscard]] T &&take() { return std::move(value.value()); }

    static Result success(T val) { return {std::move(val), {}}; }
    static Result fail(const QString &msg) { return {std::nullopt, msg}; }
};

/// Void version
struct Status {
    QString error;
    [[nodiscard]] bool ok() const { return error.isEmpty(); }
    static Status success() { return {}; }
    static Status fail(const QString &msg) { return {msg}; }
};

} // namespace dstools
