//
// Created by fluty on 24-9-28.
//

#ifndef GPUINFO_H
#define GPUINFO_H

#include <QString>
#include <QStringView>
#include <QMetaType>

class GpuInfo {
public:
    int index = -1;
    QString description;
    QString deviceId;
    unsigned long long memory = 0;

    static inline QString getIdString(unsigned int device_id, unsigned int vendor_id);
    static inline bool parseIdString(QStringView idString, unsigned int &device_id, unsigned int &vendor_id);

};


inline QString GpuInfo::getIdString(unsigned int device_id, unsigned int vendor_id) {
    constexpr int ID_STR_LEN = 8;
    char s[ID_STR_LEN + 1] = { '\0' };
    snprintf(s, sizeof(s), "%04X%04X", device_id, vendor_id);
    return QString::fromLatin1(s, ID_STR_LEN);
}

inline bool GpuInfo::parseIdString(QStringView idString, unsigned int &device_id, unsigned int &vendor_id) {
    // Guaranteed: If the function returns false, both device_id and vendor_id are not modified.

    QStringView::size_type pos = 0;

    if (idString.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
        pos += 2;
    }

    if (idString.length() - pos < 8) {
        return false;
    }

    unsigned int out_device_id, out_vendor_id;
    bool ok;

    if (out_device_id = idString.mid(pos, 4).toUInt(&ok, 16); !ok) {
        return false;
    }
    if (out_vendor_id = idString.mid(pos + 4, 4).toUInt(&ok, 16); !ok) {
        return false;
    }
    device_id = out_device_id;
    vendor_id = out_vendor_id;

    return true;
}

Q_DECLARE_METATYPE(GpuInfo)

#endif //GPUINFO_H
