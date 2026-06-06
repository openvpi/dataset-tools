#pragma once

#include <cstdint>
#include <string_view>

namespace dsfw {

enum class ErrorCode : uint16_t {
    // ── Generic errors (0x0000~0x0FFF) ──
    None = 0,
    Unknown = 1,
    NotImplemented = 2,

    // ── I/O errors (0x1000~0x1FFF) ──
    FileNotFound = 0x1000,
    FileAccessDenied = 0x1001,
    FileReadError = 0x1002,
    FileWriteError = 0x1003,
    DirectoryNotFound = 0x1010,
    PathTooLong = 0x1011,

    // ── Data errors (0x2000~0x2FFF) ──
    InvalidFormat = 0x2000,
    EncodingError = 0x2001,
    ParseError = 0x2002,
    ValidationFailed = 0x2003,
    MissingField = 0x2004,
    TypeMismatch = 0x2005,

    // ── Model errors (0x3000~0x3FFF) ──
    ModelNotFound = 0x3000,
    ModelLoadError = 0x3001,
    InferenceError = 0x3002,

    // ── Configuration errors (0x4000~0x4FFF) ──
    ConfigInvalid = 0x4000,
    ConfigKeyNotFound = 0x4001,
};

/// @brief Return a human-readable message for an error code.
constexpr std::string_view errorCodeMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::None:
            return "No error";
        case ErrorCode::Unknown:
            return "Unknown error";
        case ErrorCode::NotImplemented:
            return "Not implemented";
        case ErrorCode::FileNotFound:
            return "File not found";
        case ErrorCode::FileAccessDenied:
            return "File access denied";
        case ErrorCode::FileReadError:
            return "File read error";
        case ErrorCode::FileWriteError:
            return "File write error";
        case ErrorCode::DirectoryNotFound:
            return "Directory not found";
        case ErrorCode::PathTooLong:
            return "Path too long";
        case ErrorCode::InvalidFormat:
            return "Invalid format";
        case ErrorCode::EncodingError:
            return "Encoding error";
        case ErrorCode::ParseError:
            return "Parse error";
        case ErrorCode::ValidationFailed:
            return "Validation failed";
        case ErrorCode::MissingField:
            return "Missing required field";
        case ErrorCode::TypeMismatch:
            return "Type mismatch";
        case ErrorCode::ModelNotFound:
            return "Model not found";
        case ErrorCode::ModelLoadError:
            return "Model load error";
        case ErrorCode::InferenceError:
            return "Inference error";
        case ErrorCode::ConfigInvalid:
            return "Invalid configuration";
        case ErrorCode::ConfigKeyNotFound:
            return "Configuration key not found";
    }
    return "Unknown error code";
}

} // namespace dsfw
