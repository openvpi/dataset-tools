#pragma once

#include <QString>
#include <QStringList>
#include <functional>
#include <vector>

namespace dsfw {

    inline QString formatDur6(double d) {
        QString s = QString::number(d, 'f', 6);
        if (s.contains(QLatin1Char('.'))) {
            while (s.endsWith(QLatin1Char('0')))
                s.chop(1);
            if (s.endsWith(QLatin1Char('.')))
                s.chop(1);
        }
        return s;
    }

    template <typename T>
    inline std::vector<T> splitToVector(const QString &str, QChar sep = QLatin1Char(' '),
                                        std::function<T(const QString &)> convert = nullptr) {
        const QStringList parts = str.split(sep, Qt::SkipEmptyParts);
        std::vector<T> result;
        result.reserve(parts.size());
        for (const auto &s : parts) {
            if (convert)
                result.push_back(convert(s));
            else if constexpr (std::is_same_v<T, QString>)
                result.push_back(s);
            else if constexpr (std::is_same_v<T, std::string>)
                result.push_back(s.toStdString());
            else if constexpr (std::is_same_v<T, float>)
                result.push_back(s.toFloat());
            else if constexpr (std::is_same_v<T, int>)
                result.push_back(s.toInt());
            else if constexpr (std::is_same_v<T, double>)
                result.push_back(s.toDouble());
            else
                result.push_back(T{});
        }
        return result;
    }

} // namespace dsfw