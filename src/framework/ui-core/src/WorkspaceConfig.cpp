#include <dsfw/WorkspaceConfig.h>
#include <dsfw/AppSettings.h>
#include <dsfw/CommonKeys.h>

#include <QSplitter>
#include <QJsonDocument>
#include <QJsonObject>

#include <nlohmann/json.hpp>

namespace dsfw {

WorkspaceConfig::WorkspaceConfig(dsfw::AppSettings *settings)
    : m_settings(settings) {}

int WorkspaceConfig::iconSize() const {
    return m_settings ? m_settings->get(CommonKeys::WorkspaceIconSize) : 24;
}

void WorkspaceConfig::setIconSize(int size) {
    if (m_settings)
        m_settings->set(CommonKeys::WorkspaceIconSize, size);
}

void WorkspaceConfig::saveSplitterState(const QString &key, QSplitter *splitter) {
    if (!m_settings || !splitter) return;

    auto layoutStr = m_settings->get(CommonKeys::WorkspaceLayout);
    auto j = nlohmann::json::parse(layoutStr.toStdString(), nullptr, false);
    if (j.is_discarded()) j = nlohmann::json::object();

    if (!j.contains("splitters"))
        j["splitters"] = nlohmann::json::object();
    j["splitters"][key.toStdString()] = splitter->saveState().toBase64().toStdString();

    m_settings->set(CommonKeys::WorkspaceLayout, QString::fromStdString(j.dump()));
}

void WorkspaceConfig::restoreSplitterState(const QString &key, QSplitter *splitter) {
    if (!m_settings || !splitter) return;

    auto layoutStr = m_settings->get(CommonKeys::WorkspaceLayout);
    auto j = nlohmann::json::parse(layoutStr.toStdString(), nullptr, false);
    if (j.is_discarded()) return;
    if (!j.contains("splitters")) return;

    auto it = j["splitters"].find(key.toStdString());
    if (it == j["splitters"].end()) return;
    if (!it->is_string()) return;

    auto state = QByteArray::fromBase64(QByteArray::fromStdString(it->get<std::string>()));
    if (!state.isEmpty())
        splitter->restoreState(state);
}

void WorkspaceConfig::saveSidebarCollapsed(const QString &key, bool collapsed) {
    if (!m_settings) return;

    auto layoutStr = m_settings->get(CommonKeys::WorkspaceLayout);
    auto j = nlohmann::json::parse(layoutStr.toStdString(), nullptr, false);
    if (j.is_discarded()) j = nlohmann::json::object();

    if (!j.contains("sidebars"))
        j["sidebars"] = nlohmann::json::object();
    j["sidebars"][key.toStdString()] = collapsed;

    m_settings->set(CommonKeys::WorkspaceLayout, QString::fromStdString(j.dump()));
}

bool WorkspaceConfig::restoreSidebarCollapsed(const QString &key, bool defaultVal) const {
    if (!m_settings) return defaultVal;

    auto layoutStr = m_settings->get(CommonKeys::WorkspaceLayout);
    auto j = nlohmann::json::parse(layoutStr.toStdString(), nullptr, false);
    if (j.is_discarded()) return defaultVal;
    if (!j.contains("sidebars")) return defaultVal;

    auto it = j["sidebars"].find(key.toStdString());
    if (it == j["sidebars"].end()) return defaultVal;
    if (!it->is_boolean()) return defaultVal;

    return it->get<bool>();
}

void WorkspaceConfig::saveAll() {
    if (m_settings)
        m_settings->flush();
}

void WorkspaceConfig::restoreAll() {}

} // namespace dsfw