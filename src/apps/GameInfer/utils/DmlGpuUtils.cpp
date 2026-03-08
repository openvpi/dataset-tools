#ifdef _WIN32

#include "DmlGpuUtils.h"

#include <dxgi1_6.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

static ComPtr<IDXGIFactory6> g_dxgiFactory;

enum VendorIdList: unsigned int {
    VID_AMD = 0x1002,
    VID_NVIDIA = 0x10DE,
    VID_SAPPHIRE = 0x174B,
};

static GpuInfo createGpuInfoFromDesc(const DXGI_ADAPTER_DESC1 &desc, int index) {
    GpuInfo info;
    info.index = index;
    info.description = QString::fromWCharArray(desc.Description);
    info.deviceId = GpuInfo::getIdString(desc.DeviceId, desc.VendorId);
    info.memory = desc.DedicatedVideoMemory;
    return info;
}

static bool initDxgi() {
    if (!g_dxgiFactory) {
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&g_dxgiFactory)))) {
            // Failed to create DXGI factory.
            return false;
        }
    }
    return true;
}

GpuInfo DmlGpuUtils::getGpuByIndex(int adapterIndex) {
    // Create DXGI factory
    if (!initDxgi()) {
        return {};
    }

    // Enumerate adapters
    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(g_dxgiFactory->EnumAdapters1(adapterIndex, &adapter))) {
        return {};
    }
    DXGI_ADAPTER_DESC1 desc;
    if (FAILED(adapter->GetDesc1(&desc))) {
        return {};
    }

    return createGpuInfoFromDesc(desc, adapterIndex);
}

QList<GpuInfo> DmlGpuUtils::getGpuList() {
    QList<GpuInfo> gpuList;

    // Create DXGI factory
    if (!initDxgi()) {
        return {};
    }

    // Enumerate adapters
    ComPtr<IDXGIAdapter1> adapter;
    for (int adapterIndex = 0;
         g_dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND;
         ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        if (FAILED(adapter->GetDesc1(&desc))) {
            continue;
        }

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            // Skip software adapters
            continue;
        }

        gpuList.push_back(createGpuInfoFromDesc(desc, adapterIndex));
    }

    // Sort gpuList by DedicatedVideoMemory in descending order
    std::sort(gpuList.begin(), gpuList.end(),
              [](const GpuInfo &a, const GpuInfo &b) { return a.memory > b.memory; });
    return gpuList;

}

GpuInfo DmlGpuUtils::getGpuByPciDeviceVendorId(unsigned int pciDeviceId, unsigned int pciVendorId, int indexHint) {
    if (!initDxgi()) {
        return {};
    }

    GpuInfo info;

    // Enumerate adapters
    ComPtr<IDXGIAdapter1> adapter;
    for (int adapterIndex = 0;
         g_dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND;
         ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        if (FAILED(adapter->GetDesc1(&desc))) {
            continue;
        }

        if (pciDeviceId == desc.DeviceId && pciVendorId == desc.VendorId) {
            info = createGpuInfoFromDesc(desc, adapterIndex);
            if (adapterIndex >= indexHint) {
                break;
            }
        }
    }
    return info;
}

GpuInfo DmlGpuUtils::getGpuByPciDeviceVendorIdString(const QString &idString, int indexHint) {
    unsigned int pciDeviceId = 0;
    unsigned int pciVendorId = 0;
    if (!GpuInfo::parseIdString(idString, pciDeviceId, pciVendorId)) {
        return {};
    }
    return getGpuByPciDeviceVendorId(pciDeviceId, pciVendorId, indexHint);
}

GpuInfo DmlGpuUtils::getRecommendedGpu() {
    // Create DXGI factory
    if (!initDxgi()) {
        return {};
    }

    bool hasPreferredGpu = false;
    GpuInfo preferredGpu;
    GpuInfo otherGpu;

    // Enumerate adapters
    ComPtr<IDXGIAdapter1> adapter;
    for (int adapterIndex = 0;
         g_dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND;
         ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        if (FAILED(adapter->GetDesc1(&desc))) {
            continue;
        }

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            // Skip software adapters
            continue;
        }

        bool flag = desc.VendorId == VID_NVIDIA || desc.VendorId == VID_AMD
                    || desc.VendorId == VID_SAPPHIRE;
        if (!flag) {
            QString name = QString::fromWCharArray(desc.Description);
            flag = name.compare("NVIDIA", Qt::CaseInsensitive) == 0;
        }

        if (flag) {
            if (desc.DedicatedVideoMemory > preferredGpu.memory) {
                preferredGpu = createGpuInfoFromDesc(desc, adapterIndex);
            }
            hasPreferredGpu = true;
        } else {
            if (desc.DedicatedVideoMemory > otherGpu.memory) {
                otherGpu = createGpuInfoFromDesc(desc, adapterIndex);
            }
        }
    }
    if (hasPreferredGpu) {
        return preferredGpu;
    }
    return otherGpu;
}

#endif // defined(_WIN32)