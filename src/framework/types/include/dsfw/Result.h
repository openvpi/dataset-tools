#pragma once
/// @file Result.h
/// @brief Canonical include for Result<T>.

#include "ErrorCode.h"
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace dsfw {

    template <typename T>
    class Result {
    public:
        // Non-explicit by design: allows `return value;` syntax for ergonomic Result construction.
        Result(T value) : m_value(std::move(value)), m_ok(true) {
        } // NOLINT(google-explicit-constructor)

        static Result Ok(T value) {
            return Result(std::move(value));
        }

        static Result Error(std::string error) {
            Result r;
            r.m_ok = false;
            r.m_error = std::move(error);
            r.m_errcode = ErrorCode::Unknown;
            return r;
        }

        static Result Error(const char *error) {
            return Error(std::string(error));
        }

        static Result Error(ErrorCode code, std::string error) {
            Result r;
            r.m_ok = false;
            r.m_error = std::move(error);
            r.m_errcode = code;
            return r;
        }

        static Result Error(ErrorCode code, const char *error) {
            return Error(code, std::string(error));
        }

        static Result Error(ErrorCode code) {
            Result r;
            r.m_ok = false;
            r.m_error = std::string(errorCodeMessage(code));
            r.m_errcode = code;
            return r;
        }

        bool ok() const noexcept {
            return m_ok;
        }
        explicit operator bool() const noexcept {
            return m_ok;
        }

        const T &value() const & noexcept {
            return *m_value;
        }
        T &value() & noexcept {
            return *m_value;
        }
        T &&value() && noexcept {
            return std::move(*m_value);
        }

        const T &operator*() const & noexcept {
            return *m_value;
        }
        T &operator*() & noexcept {
            return *m_value;
        }

        const T *operator->() const noexcept {
            return &*m_value;
        }
        T *operator->() noexcept {
            return &*m_value;
        }

        const std::string &error() const noexcept {
            return m_error;
        }

        ErrorCode code() const noexcept {
            return m_errcode;
        }

        T value_or(T default_value) const {
            return m_ok ? *m_value : std::move(default_value);
        }

    private:
        Result() = default;

        std::optional<T> m_value;
        std::string m_error;
        ErrorCode m_errcode = ErrorCode::None;
        bool m_ok = false;
    };

    template <>
    class Result<void> {
    public:
        static Result Ok() {
            return Result(true);
        }
        static Result Error(std::string error) {
            Result r;
            r.m_ok = false;
            r.m_error = std::move(error);
            r.m_errcode = ErrorCode::Unknown;
            return r;
        }
        static Result Error(const char *error) {
            return Error(std::string(error));
        }

        static Result Error(ErrorCode code, std::string error) {
            Result r;
            r.m_ok = false;
            r.m_error = std::move(error);
            r.m_errcode = code;
            return r;
        }

        static Result Error(ErrorCode code, const char *error) {
            return Error(code, std::string(error));
        }

        static Result Error(ErrorCode code) {
            Result r;
            r.m_ok = false;
            r.m_error = std::string(errorCodeMessage(code));
            r.m_errcode = code;
            return r;
        }

        bool ok() const noexcept {
            return m_ok;
        }
        explicit operator bool() const noexcept {
            return m_ok;
        }

        const std::string &error() const noexcept {
            return m_error;
        }

        ErrorCode code() const noexcept {
            return m_errcode;
        }

    private:
        Result(bool ok) : m_ok(ok) {
        }
        Result() = default;

        std::string m_error;
        ErrorCode m_errcode = ErrorCode::None;
        bool m_ok = false;
    };

    inline Result<void> Ok() {
        return Result<void>::Ok();
    }

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

    inline Result<void> Err(ErrorCode code, std::string error) {
        return Result<void>::Error(code, std::move(error));
    }

    inline Result<void> Err(ErrorCode code, const char *error) {
        return Result<void>::Error(code, error);
    }

    inline Result<void> Err(ErrorCode code) {
        return Result<void>::Error(code);
    }

    template <typename T>
    inline Result<T> Err(std::string error) {
        return Result<T>::Error(std::move(error));
    }

    template <typename T>
    inline Result<T> Err(const char *error) {
        return Result<T>::Error(error);
    }

    template <typename T>
    inline Result<T> Err(ErrorCode code, std::string error) {
        return Result<T>::Error(code, std::move(error));
    }

    template <typename T>
    inline Result<T> Err(ErrorCode code, const char *error) {
        return Result<T>::Error(code, error);
    }

    template <typename T>
    inline Result<T> Err(ErrorCode code) {
        return Result<T>::Error(code);
    }

} // namespace dsfw
