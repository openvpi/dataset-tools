#include "ChartConfigRegistry.h"

#include <QMetaType>
#include <dsfw/AppSettings.h>
#include <nlohmann/json.hpp>

namespace dstools {

using namespace dsfw;

using namespace dsfw;

ChartConfigRegistry &ChartConfigRegistry::instance() {
    static ChartConfigRegistry inst;
    return inst;
}

void ChartConfigRegistry::setSettings(AppSettings *settings) {
    m_settings = settings;
}

void ChartConfigRegistry::registerChart(const ChartConfigDescriptor &descriptor) {
    m_charts[descriptor.chartId] = descriptor;
}

QStringList ChartConfigRegistry::chartIds() const {
    return m_charts.keys();
}

std::vector<ChartParamDescriptor> ChartConfigRegistry::descriptorsFor(const QString &chartId) const {
    auto it = m_charts.find(chartId);
    if (it != m_charts.end()) {
        return it->params;
    }
    return {};
}

QString ChartConfigRegistry::displayNameFor(const QString &chartId) const {
    auto it = m_charts.find(chartId);
    if (it != m_charts.end()) {
        return it->displayName;
    }
    return {};
}

const ChartParamDescriptor *ChartConfigRegistry::findParam(const QString &chartId, const QString &paramKey) const {
    auto it = m_charts.find(chartId);
    if (it == m_charts.end()) return nullptr;

    for (const auto &p : it->params) {
        if (p.key == paramKey) return &p;
    }
    return nullptr;
}

QVariant ChartConfigRegistry::defaultValue(const QString &chartId, const QString &paramKey) const {
    if (auto *p = findParam(chartId, paramKey)) {
        return p->defaultValue;
    }
    return {};
}

QVariant ChartConfigRegistry::value(const QString &chartId, const QString &paramKey) const {
    auto chartIt = m_overrides.find(chartId);
    if (chartIt != m_overrides.end()) {
        auto paramIt = chartIt->find(paramKey);
        if (paramIt != chartIt->end()) {
            return *paramIt;
        }
    }
    return defaultValue(chartId, paramKey);
}

void ChartConfigRegistry::setValue(const QString &chartId, const QString &paramKey, const QVariant &value) {
    m_overrides[chartId][paramKey] = value;
    saveConfig(chartId);
}

void ChartConfigRegistry::loadDefaults() {
    if (!m_settings) return;
    for (const auto &chartId : chartIds()) {
        std::string chartIdStr = chartId.toStdString();
        QString raw = m_settings->getRawString(chartIdStr.c_str(), QStringLiteral("{}"));
        auto chartJson = nlohmann::json::parse(raw.toStdString(), nullptr, false);
        if (chartJson.is_null() || !chartJson.is_object()) continue;

        for (const auto &param : descriptorsFor(chartId)) {
            std::string key = param.key.toStdString();
            if (chartJson.contains(key)) {
                QVariant v;
                if (param.type == ParamType::Double) {
                    v = QVariant(chartJson[key].get<double>());
                } else {
                    v = QVariant(chartJson[key].get<int>());
                }
                setValue(chartId, param.key, v);
            }
        }
    }
}

void ChartConfigRegistry::saveConfig(const QString &chartId) {
    if (!m_settings) return;
    nlohmann::json chartJson = nlohmann::json::object();
    auto overrideIt = m_overrides.find(chartId);
    if (overrideIt == m_overrides.end()) return;

    for (auto it = overrideIt->begin(); it != overrideIt->end(); ++it) {
        std::string key = it.key().toStdString();
        if (it->typeId() == QMetaType::Double) {
            chartJson[key] = it->toDouble();
        } else {
            chartJson[key] = it->toInt();
        }
    }
    m_settings->setRawString(chartId.toStdString().c_str(), QString::fromStdString(chartJson.dump()));
}

} // namespace dstools