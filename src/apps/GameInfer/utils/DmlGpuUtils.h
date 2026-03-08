//
// Created by fluty on 24-9-28.
//

#ifndef DMLGPUUTILS_H
#define DMLGPUUTILS_H

#include "GpuInfo.h"

#include <QList>

class DmlGpuUtils {
public:
    static GpuInfo getGpuByIndex(int adapterIndex);
    static QList<GpuInfo> getGpuList();
    static GpuInfo getGpuByPciDeviceVendorId(unsigned int pciDeviceId, unsigned int pciVendorId, int indexHint = 0);
    static GpuInfo getGpuByPciDeviceVendorIdString(const QString &idString, int indexHint = 0);
    static GpuInfo getRecommendedGpu();
};

#ifndef _WIN32
    inline GpuInfo DmlGpuUtils::getGpuByIndex(int index) {
        Q_UNUSED(index)
        return {};
    }

    inline QList<GpuInfo> DmlGpuUtils::getGpuList() {
        return {};
    }

    inline GpuInfo DmlGpuUtils::getGpuByPciDeviceVendorId(unsigned int pciDeviceId, unsigned int pciVendorId, int indexHint) {
        Q_UNUSED(pciDeviceId)
        Q_UNUSED(pciVendorId)
        Q_UNUSED(indexHint)
        return {};
    }

    inline GpuInfo DmlGpuUtils::getGpuByPciDeviceVendorIdString(const QString &idString, int indexHint) {
        Q_UNUSED(idString)
        Q_UNUSED(indexHint)
        return {};
    }

    inline GpuInfo DmlGpuUtils::getRecommendedGpu() {
        return {};
    }
#endif

#endif // DMLGPUUTILS_H
