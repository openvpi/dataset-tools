#include <dstools/Config.h>
#include <QApplication>
#include <QDir>

namespace dstools {

Config::Config(const QString &appName)
    : m_settings(QApplication::applicationDirPath() + "/config/" + appName + ".ini",
                 QSettings::IniFormat) {
}

QString Config::getString(const QString &key, const QString &defaultValue) const {
    return m_settings.value(key, defaultValue).toString();
}

void Config::setString(const QString &key, const QString &value) {
    m_settings.setValue(key, value);
}

int Config::getInt(const QString &key, int defaultValue) const {
    return m_settings.value(key, defaultValue).toInt();
}

void Config::setInt(const QString &key, int value) {
    m_settings.setValue(key, value);
}

double Config::getDouble(const QString &key, double defaultValue) const {
    return m_settings.value(key, defaultValue).toDouble();
}

void Config::setDouble(const QString &key, double value) {
    m_settings.setValue(key, value);
}

bool Config::getBool(const QString &key, bool defaultValue) const {
    return m_settings.value(key, defaultValue).toBool();
}

void Config::setBool(const QString &key, bool value) {
    m_settings.setValue(key, value);
}

QKeySequence Config::shortcut(const QString &action, const QKeySequence &defaultSeq) const {
    return QKeySequence(m_settings.value("Shortcuts/" + action, defaultSeq.toString()).toString());
}

void Config::setShortcut(const QString &action, const QKeySequence &seq) {
    m_settings.setValue("Shortcuts/" + action, seq.toString());
}

void Config::beginGroup(const QString &group) {
    m_settings.beginGroup(group);
}

void Config::endGroup() {
    m_settings.endGroup();
}

void Config::sync() {
    m_settings.sync();
}

QSettings &Config::settings() {
    return m_settings;
}

} // namespace dstools
