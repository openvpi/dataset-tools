#include <audio-util/SndfileVio.h>

#include <cstring>

namespace AudioUtil
{
    sf_count_t qvio_get_filelen(void *user_data) {
        const auto *qvio = static_cast<QVIO *>(user_data);
        return qvio->byteArray.size();
    }

    sf_count_t qvio_seek(const sf_count_t offset, const int whence, void *user_data) {
        auto *qvio = static_cast<QVIO *>(user_data);
        switch (whence) {
        case SEEK_SET:
            qvio->seek = offset;
            break;
        case SEEK_CUR:
            qvio->seek += offset;
            break;
        case SEEK_END:
            qvio->seek = qvio->byteArray.size() + offset;
            break;
        default:
            break;
        }
        return qvio->seek;
    }

    sf_count_t qvio_read(void *ptr, const sf_count_t count, void *user_data) {
        auto *qvio = static_cast<QVIO *>(user_data);
        const sf_count_t remainingBytes = qvio->byteArray.size() - qvio->seek;
        const sf_count_t bytesToRead = std::min(count, remainingBytes);
        if (bytesToRead > 0) {
            std::memcpy(ptr, qvio->byteArray.data() + qvio->seek, bytesToRead);
            qvio->seek += bytesToRead;
        }
        return bytesToRead;
    }

    sf_count_t qvio_write(const void *ptr, const sf_count_t count, void *user_data) {
        auto *qvio = static_cast<QVIO *>(user_data);
        auto *data = static_cast<const char *>(ptr);
        qvio->byteArray.reserve(qvio->byteArray.size() + count);
        qvio->byteArray.insert(qvio->byteArray.end(), data, data + count);
        return count;
    }

    sf_count_t qvio_tell(void *user_data) {
        const auto *qvio = static_cast<QVIO *>(user_data);
        return qvio->seek;
    }
} // namespace Rmvpe
