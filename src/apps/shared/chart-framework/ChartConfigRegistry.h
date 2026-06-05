#pragma once

#include "ChartConfigTypes.h"

#include <QMap>
#include <QString>
#include <QStringList>
#include <dsfw/AppSettings.h>

namespace dstools {

namespace chart {

class ChartConfigRegistry {
public:
    ChartConfigRegistry(const ChartConfigRegistry &) = delete;
    ChartConfigRegistry &operator=(const ChartConfigRegistry &) = delete;

    static ChartConfigRegistry &instance();

    void registerChart(const ChartConfigDescriptor &descriptor);
    void setSettings(AppSettings *settings);

    [[nodiscard]] QStringList chartIds() const;
    [[nodiscard]] std::vector<ChartParamDescriptor> descriptorsFor(const QString &chartId) const;
    [[nodiscard]] QString displayNameFor(const QString &chartId) const;

    [[nodiscard]] QVariant value(const QString &chartId, const QString &paramKey) const;

    void setValue(const QString &chartId, const QString &paramKey, const QVariant &value);

    void loadDefaults();
    void saveConfig(const QString &chartId);

private:
    ChartConfigRegistry() = default;

    [[nodiscard]] QVariant defaultValue(const QString &chartId, const QString &paramKey) const;
    [[nodiscard]] const ChartParamDescriptor *findParam(const QString &chartId, const QString &paramKey) const;

    AppSettings *m_settings = nullptr;
    QMap<QString, ChartConfigDescriptor> m_charts;
    QMap<QString, QMap<QString, QVariant>> m_overrides;
};

} // namespace chart
} // namespace dstools