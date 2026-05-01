/// @file SndfileVio.h
/// @brief Virtual I/O adapter for libsndfile using in-memory byte arrays.

#pragma once
#include <sndfile.hh>
#include <vector>

#include <audio-util/AudioUtilGlobal.h>

namespace AudioUtil
{
    /// @brief Get total length of the in-memory buffer.
    /// @param user_data Pointer to a QVIO instance.
    /// @return Buffer size in bytes.
    sf_count_t AUDIO_UTIL_EXPORT qvio_get_filelen(void *user_data);

    /// @brief Seek within the in-memory buffer.
    /// @param offset Byte offset.
    /// @param whence Seek origin (SEEK_SET, SEEK_CUR, SEEK_END).
    /// @param user_data Pointer to a QVIO instance.
    /// @return New absolute position.
    sf_count_t AUDIO_UTIL_EXPORT qvio_seek(sf_count_t offset, int whence, void *user_data);

    /// @brief Read bytes from the in-memory buffer.
    /// @param ptr Destination buffer.
    /// @param count Number of bytes to read.
    /// @param user_data Pointer to a QVIO instance.
    /// @return Number of bytes actually read.
    sf_count_t AUDIO_UTIL_EXPORT qvio_read(void *ptr, sf_count_t count, void *user_data);

    /// @brief Write bytes to the in-memory buffer.
    /// @param ptr Source data.
    /// @param count Number of bytes to write.
    /// @param user_data Pointer to a QVIO instance.
    /// @return Number of bytes actually written.
    sf_count_t AUDIO_UTIL_EXPORT qvio_write(const void *ptr, sf_count_t count, void *user_data);

    /// @brief Get current seek position in the in-memory buffer.
    /// @param user_data Pointer to a QVIO instance.
    /// @return Current position in bytes.
    sf_count_t AUDIO_UTIL_EXPORT qvio_tell(void *user_data);

    /// @brief In-memory byte buffer with seek position.
    struct AUDIO_UTIL_EXPORT QVIO {
        uint64_t seek{};               ///< Current read/write position.
        std::vector<char> byteArray;    ///< Raw byte storage.
    };

    /// @brief Combined virtual I/O callbacks and buffer for libsndfile.
    struct AUDIO_UTIL_EXPORT SF_VIO {
        SF_VIRTUAL_IO vio{};  ///< libsndfile virtual I/O callback table.
        QVIO data;            ///< In-memory byte buffer.
        SF_INFO info;         ///< Audio format information.

        /// @brief Initialize virtual I/O callbacks.
        SF_VIO() {
            vio.get_filelen = qvio_get_filelen;
            vio.seek = qvio_seek;
            vio.read = qvio_read;
            vio.write = qvio_write;
            vio.tell = qvio_tell;
        }

        /// @brief Get buffer size in bytes.
        /// @return Number of bytes stored.
        size_t size() const { return data.byteArray.size(); }

        /// @brief Get read-only pointer to buffer data.
        /// @return Pointer to the first byte.
        const char *constData() const { return data.byteArray.data(); }
    };
} // namespace AudioUtil
