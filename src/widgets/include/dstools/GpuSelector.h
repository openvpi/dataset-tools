#pragma once
#include <dstools/WidgetsGlobal.h>
#include <QComboBox>
#include <QVector>
#include <QString>

namespace dstools::widgets {

struct GpuInfo {
    int index = -1;
    QString name;
    size_t dedicatedMemory = 0;
};

/// GPU selection combo box. Auto-enumerates DirectML (DXGI) GPU devices.
class DSTOOLS_WIDGETS_API GpuSelector : public QComboBox {
    Q_OBJECT
public:
    explicit GpuSelector(QWidget *parent = nullptr);

    int selectedDeviceId() const;
    QVector<GpuInfo> availableGpus() const;
    void refresh();

private:
    QVector<GpuInfo> m_gpus;
    void enumerateGpus();
};

} // namespace dstools::widgets
