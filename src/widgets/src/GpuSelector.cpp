#include <dstools/GpuSelector.h>

#ifdef _WIN32
#include <dxgi1_6.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace dstools::widgets {

GpuSelector::GpuSelector(QWidget *parent) : QComboBox(parent) {
    enumerateGpus();
}

int GpuSelector::selectedDeviceId() const {
    if (currentIndex() < 0 || currentIndex() >= m_gpus.size())
        return -1;
    return m_gpus[currentIndex()].index;
}

QVector<GpuInfo> GpuSelector::availableGpus() const {
    return m_gpus;
}

void GpuSelector::refresh() {
    clear();
    m_gpus.clear();
    enumerateGpus();
}

void GpuSelector::enumerateGpus() {
#ifdef _WIN32
    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        addItem("Default (CPU)", -1);
        return;
    }

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        if (FAILED(adapter->GetDesc1(&desc))) continue;
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

        GpuInfo info;
        info.index = static_cast<int>(i);
        info.name = QString::fromWCharArray(desc.Description);
        info.dedicatedMemory = desc.DedicatedVideoMemory;
        m_gpus.append(info);

        double memGB = static_cast<double>(info.dedicatedMemory) / (1024.0 * 1024.0 * 1024.0);
        addItem(QString("%1 (%2 GB)").arg(info.name).arg(memGB, 0, 'f', 1), info.index);
    }
#endif
    addItem("Default (CPU)", -1);
}

} // namespace dstools::widgets
