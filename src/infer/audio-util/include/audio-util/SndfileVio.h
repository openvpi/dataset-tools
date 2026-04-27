#ifndef SNDFILEVIO_H
#define SNDFILEVIO_H

#include <sndfile.hh>
#include <vector>

#include <audio-util/AudioUtilGlobal.h>

namespace AudioUtil
{
    sf_count_t AUDIO_UTIL_EXPORT qvio_get_filelen(void *user_data);
    sf_count_t AUDIO_UTIL_EXPORT qvio_seek(sf_count_t offset, int whence, void *user_data);
    sf_count_t AUDIO_UTIL_EXPORT qvio_read(void *ptr, sf_count_t count, void *user_data);
    sf_count_t AUDIO_UTIL_EXPORT qvio_write(const void *ptr, sf_count_t count, void *user_data);
    sf_count_t AUDIO_UTIL_EXPORT qvio_tell(void *user_data);

    struct AUDIO_UTIL_EXPORT QVIO {
        uint64_t seek{};
        std::vector<char> byteArray;
    };

    struct AUDIO_UTIL_EXPORT SF_VIO {
        SF_VIRTUAL_IO vio{};
        QVIO data;
        SF_INFO info;

        SF_VIO() {
            vio.get_filelen = qvio_get_filelen;
            vio.seek = qvio_seek;
            vio.read = qvio_read;
            vio.write = qvio_write;
            vio.tell = qvio_tell;
        }

        size_t size() const { return data.byteArray.size(); }
        const char *constData() const { return data.byteArray.data(); }
    };
} // namespace AudioUtil

#endif // SNDFILEVIO_H
