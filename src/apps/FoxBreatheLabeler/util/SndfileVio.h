#ifndef SNDFILEVIO_H
#define SNDFILEVIO_H

#include <QByteArray>

#include <sndfile.hh>

namespace FBL {
    sf_count_t qvio_get_filelen(void *user_data);
    sf_count_t qvio_seek(sf_count_t offset, int whence, void *user_data);
    sf_count_t qvio_read(void *ptr, sf_count_t count, void *user_data);
    sf_count_t qvio_write(const void *ptr, sf_count_t count, void *user_data);
    sf_count_t qvio_tell(void *user_data);

    struct QVIO {
        qint64 seek{};
        QByteArray byteArray;
    };

    struct SF_VIO {
        SF_VIRTUAL_IO vio{};
        QVIO data;

        SF_VIO() {
            vio.get_filelen = qvio_get_filelen;
            vio.seek = qvio_seek;
            vio.read = qvio_read;
            vio.write = qvio_write;
            vio.tell = qvio_tell;
        }

        [[nodiscard]] int size() const {
            return data.byteArray.size();
        }
        [[nodiscard]] const char *constData() const {
            return data.byteArray.constData();
        }
    };
} // LyricFA

#endif // SNDFILEVIO_H
