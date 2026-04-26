#pragma once

#include <QString>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace dstools {

    struct ConfigValue {
        using ValueType = std::variant<bool, int64_t, double, QString,
                                       std::vector<QString>, std::vector<int64_t>, std::vector<double>>;
        ValueType value;

        ConfigValue() : value(false) {}
        explicit ConfigValue(ValueType v) : value(std::move(v)) {}

        ConfigValue(bool v) : value(v) {}
        ConfigValue(int v) : value(static_cast<int64_t>(v)) {}
        ConfigValue(int64_t v) : value(v) {}
        ConfigValue(double v) : value(v) {}
        ConfigValue(const QString &v) : value(v) {}
        ConfigValue(const char *v) : value(QString::fromUtf8(v)) {}
        ConfigValue(const std::string &v) : value(QString::fromStdString(v)) {}
        ConfigValue(const std::vector<QString> &v) : value(v) {}
        ConfigValue(const std::vector<int64_t> &v) : value(v) {}
        ConfigValue(const std::vector<double> &v) : value(v) {}

        [[nodiscard]] bool asBool(bool defaultVal = false) const {
            if (auto *p = std::get_if<bool>(&value)) return *p;
            return defaultVal;
        }

        [[nodiscard]] int64_t asInt(int64_t defaultVal = 0) const {
            if (auto *p = std::get_if<int64_t>(&value)) return *p;
            if (auto *p = std::get_if<double>(&value)) return static_cast<int64_t>(*p);
            return defaultVal;
        }

        [[nodiscard]] double asDouble(double defaultVal = 0.0) const {
            if (auto *p = std::get_if<double>(&value)) return *p;
            if (auto *p = std::get_if<int64_t>(&value)) return static_cast<double>(*p);
            return defaultVal;
        }

        [[nodiscard]] QString asString(const QString &defaultVal = {}) const {
            if (auto *p = std::get_if<QString>(&value)) return *p;
            return defaultVal;
        }

        [[nodiscard]] std::vector<QString> asStringList() const {
            if (auto *p = std::get_if<std::vector<QString>>(&value)) return *p;
            return {};
        }

        [[nodiscard]] std::vector<int64_t> asIntList() const {
            if (auto *p = std::get_if<std::vector<int64_t>>(&value)) return *p;
            return {};
        }

        [[nodiscard]] std::vector<double> asDoubleList() const {
            if (auto *p = std::get_if<std::vector<double>>(&value)) return *p;
            return {};
        }

        [[nodiscard]] int typeIndex() const { return static_cast<int>(value.index()); }

        [[nodiscard]] bool isBool() const { return std::holds_alternative<bool>(value); }
        [[nodiscard]] bool isInt() const { return std::holds_alternative<int64_t>(value); }
        [[nodiscard]] bool isDouble() const { return std::holds_alternative<double>(value); }
        [[nodiscard]] bool isString() const { return std::holds_alternative<QString>(value); }
        [[nodiscard]] bool isStringList() const { return std::holds_alternative<std::vector<QString>>(value); }
        [[nodiscard]] bool isIntList() const { return std::holds_alternative<std::vector<int64_t>>(value); }
        [[nodiscard]] bool isDoubleList() const { return std::holds_alternative<std::vector<double>>(value); }

        bool operator==(const ConfigValue &other) const { return value == other.value; }
        bool operator!=(const ConfigValue &other) const { return value != other.value; }
    };

    using ConfigMap = std::map<QString, ConfigValue>;

    inline QString configValueString(const ConfigMap &m, const QString &key, const QString &def = {}) {
        auto it = m.find(key);
        return it != m.end() ? it->second.asString() : def;
    }

    inline int64_t configValueInt(const ConfigMap &m, const QString &key, int64_t def = 0) {
        auto it = m.find(key);
        return it != m.end() ? it->second.asInt() : def;
    }

    inline bool configValueBool(const ConfigMap &m, const QString &key, bool def = false) {
        auto it = m.find(key);
        return it != m.end() ? it->second.asBool() : def;
    }

    inline double configValueDouble(const ConfigMap &m, const QString &key, double def = 0.0) {
        auto it = m.find(key);
        return it != m.end() ? it->second.asDouble() : def;
    }

    inline std::vector<QString> configValueStringList(const ConfigMap &m, const QString &key,
                                                       const std::vector<QString> &def = {}) {
        auto it = m.find(key);
        return it != m.end() ? it->second.asStringList() : def;
    }

} // namespace dstools