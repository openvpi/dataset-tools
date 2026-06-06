#pragma once

#include <dsfw/ConfigTypes.h>
#include <nlohmann/json.hpp>

namespace dsfw {

    inline nlohmann::json configValueToJson(const ConfigValue &cv) {
        if (auto *p = std::get_if<bool>(&cv.value))
            return *p;
        if (auto *p = std::get_if<int64_t>(&cv.value))
            return *p;
        if (auto *p = std::get_if<double>(&cv.value))
            return *p;
        if (auto *p = std::get_if<QString>(&cv.value))
            return p->toStdString();
        if (auto *p = std::get_if<std::vector<QString>>(&cv.value)) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto &s : *p)
                arr.push_back(s.toStdString());
            return arr;
        }
        if (auto *p = std::get_if<std::vector<int64_t>>(&cv.value)) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto &v : *p)
                arr.push_back(v);
            return arr;
        }
        if (auto *p = std::get_if<std::vector<double>>(&cv.value)) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto &v : *p)
                arr.push_back(v);
            return arr;
        }
        return nullptr;
    }

    inline ConfigValue configValueFromJson(const nlohmann::json &j) {
        if (j.is_boolean())
            return ConfigValue(j.get<bool>());
        if (j.is_number_integer())
            return ConfigValue(j.get<int64_t>());
        if (j.is_number_float())
            return ConfigValue(j.get<double>());
        if (j.is_string())
            return ConfigValue(QString::fromStdString(j.get<std::string>()));
        if (j.is_array() && !j.empty()) {
            if (j[0].is_string()) {
                std::vector<QString> list;
                for (const auto &item : j)
                    if (item.is_string())
                        list.push_back(QString::fromStdString(item.get<std::string>()));
                return ConfigValue(list);
            }
            if (j[0].is_number_integer()) {
                std::vector<int64_t> list;
                for (const auto &item : j)
                    if (item.is_number_integer())
                        list.push_back(item.get<int64_t>());
                return ConfigValue(list);
            }
            if (j[0].is_number_float()) {
                std::vector<double> list;
                for (const auto &item : j)
                    if (item.is_number())
                        list.push_back(item.get<double>());
                return ConfigValue(list);
            }
            if (j[0].is_number()) {
                std::vector<double> list;
                for (const auto &item : j)
                    if (item.is_number())
                        list.push_back(item.get<double>());
                return ConfigValue(list);
            }
        }
        if (j.is_array() && j.empty()) {
            return ConfigValue(std::vector<QString>{});
        }
        return ConfigValue();
    }

    inline ConfigMap configMapFromJson(const nlohmann::json &j) {
        ConfigMap result;
        if (!j.is_object()) return result;
        for (auto it = j.begin(); it != j.end(); ++it)
            result[QString::fromStdString(it.key())] = configValueFromJson(it.value());
        return result;
    }

    inline nlohmann::json configMapToJson(const ConfigMap &map) {
        nlohmann::json j = nlohmann::json::object();
        for (const auto &[key, val] : map)
            j[key.toStdString()] = configValueToJson(val);
        return j;
    }

} // namespace dsfw