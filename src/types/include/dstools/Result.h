#pragma once
#include <string>
#include <type_traits>
#include <utility>

namespace dstools {

template <typename T>
class Result {
public:
    // Non-explicit by design: allows `return value;` syntax for ergonomic Result construction.
    Result(T value) : m_value(std::move(value)), m_ok(true) {} // NOLINT(google-explicit-constructor)

    static Result Ok(T value) { return Result(std::move(value)); }

    static Result Error(std::string error) {
        Result r;
        r.m_ok = false;
        r.m_error = std::move(error);
        return r;
    }

    static Result Error(const char *error) {
        return Error(std::string(error));
    }

    bool ok() const { return m_ok; }
    explicit operator bool() const { return m_ok; }

    const T &value() const & { return m_value; }
    T &value() & { return m_value; }
    T &&value() && { return std::move(m_value); }

    const T &operator*() const & { return m_value; }
    T &operator*() & { return m_value; }

    const T *operator->() const { return &m_value; }
    T *operator->() { return &m_value; }

    const std::string &error() const { return m_error; }

    T value_or(T default_value) const {
        return m_ok ? m_value : std::move(default_value);
    }

private:
    Result() = default;

    T m_value{};
    std::string m_error;
    bool m_ok = false;
};

template <>
class Result<void> {
public:
    static Result Ok() { return Result(true); }
    static Result Error(std::string error) {
        Result r;
        r.m_ok = false;
        r.m_error = std::move(error);
        return r;
    }
    static Result Error(const char *error) {
        return Error(std::string(error));
    }

    bool ok() const { return m_ok; }
    explicit operator bool() const { return m_ok; }

    const std::string &error() const { return m_error; }

private:
    Result(bool ok) : m_ok(ok) {}
    Result() = default;

    std::string m_error;
    bool m_ok = false;
};

inline Result<void> Ok() { return Result<void>::Ok(); }

template <typename T>
inline Result<std::decay_t<T>> Ok(T &&value) {
    return Result<std::decay_t<T>>::Ok(std::forward<T>(value));
}

inline Result<void> Err(std::string error) {
    return Result<void>::Error(std::move(error));
}

inline Result<void> Err(const char *error) {
    return Result<void>::Error(error);
}

template <typename T>
inline Result<T> Err(std::string error) {
    return Result<T>::Error(std::move(error));
}

template <typename T>
inline Result<T> Err(const char *error) {
    return Result<T>::Error(error);
}

}
