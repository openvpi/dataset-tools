#pragma once
#include <string>
#include <utility>

namespace dstools {

template <typename T>
class Result {
public:
    Result(T value) : m_value(std::move(value)), m_ok(true) {}

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

}
