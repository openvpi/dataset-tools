#pragma once

#include <QColor>
#include <QRgb>
#include <QString>
#include <vector>
#include <cmath>
#include <algorithm>

namespace dstools {
namespace phonemelabeler {

class SpectrogramColorPalette {
public:
    struct KeyColor {
        QColor color;
        float weight;
    };

    SpectrogramColorPalette() = default;

    explicit SpectrogramColorPalette(const QString &name, const std::vector<KeyColor> &keys)
        : m_name(name) {
        buildLUT(keys);
    }

    [[nodiscard]] QRgb map(float intensity) const {
        if (m_lut.empty()) return qRgb(0, 0, 0);
        float v = std::clamp(intensity, 0.0f, 1.0f);
        int idx = static_cast<int>(std::round(v * (m_lut.size() - 1)));
        return m_lut[idx];
    }

    [[nodiscard]] const QString &name() const { return m_name; }

    static SpectrogramColorPalette orangeYellow() {
        return SpectrogramColorPalette("Orange-Yellow", {
            {QColor(0, 0, 0, 0),       0.0f},
            {QColor(60, 20, 0),         3.0f},
            {QColor(180, 80, 0),        4.0f},
            {QColor(230, 160, 20),      2.0f},
            {QColor(255, 230, 100),     1.0f},
            {QColor(255, 255, 220),     0.5f},
        });
    }

    static SpectrogramColorPalette grayscale() {
        return SpectrogramColorPalette("Grayscale", {
            {QColor(0, 0, 0),           0.0f},
            {QColor(255, 255, 255),     1.0f},
        });
    }

    static SpectrogramColorPalette sunset() {
        return SpectrogramColorPalette("Sunset", {
            {QColor(0, 0, 0),           0.0f},
            {QColor(2, 6, 62),          4.0f},
            {QColor(242, 30, 7),        5.0f},
            {QColor(237, 237, 12),      1.0f},
            {QColor(252, 254, 240),     0.5f},
        });
    }

    static SpectrogramColorPalette midnight() {
        return SpectrogramColorPalette("Midnight", {
            {QColor(0, 0, 0),           0.0f},
            {QColor(31, 31, 71),        3.0f},
            {QColor(196, 90, 15),       2.0f},
            {QColor(249, 202, 58),      0.5f},
            {QColor(255, 255, 255),     0.5f},
        });
    }

    static SpectrogramColorPalette dawn() {
        return SpectrogramColorPalette("Dawn", {
            {QColor(0, 0, 0),           0.0f},
            {QColor(2, 7, 36),          2.0f},
            {QColor(2, 134, 175),       4.0f},
            {QColor(191, 202, 184),     2.0f},
            {QColor(230, 170, 171),     1.0f},
            {QColor(255, 255, 255),     0.5f},
        });
    }

    static SpectrogramColorPalette inferno() {
        return SpectrogramColorPalette("Inferno", {
            {QColor(0, 0, 4),           0.0f},
            {QColor(40, 11, 84),        2.0f},
            {QColor(101, 21, 110),      2.0f},
            {QColor(159, 42, 99),       2.0f},
            {QColor(212, 72, 66),       2.0f},
            {QColor(245, 125, 21),      2.0f},
            {QColor(250, 193, 39),      1.5f},
            {QColor(252, 255, 164),     1.0f},
        });
    }

    static std::vector<SpectrogramColorPalette> builtinPresets() {
        return {
            orangeYellow(),
            grayscale(),
            sunset(),
            midnight(),
            dawn(),
            inferno(),
        };
    }

    static SpectrogramColorPalette fromName(const QString &name) {
        for (const auto &p : builtinPresets()) {
            if (p.name().compare(name, Qt::CaseInsensitive) == 0)
                return p;
        }
        return orangeYellow();
    }

private:
    static constexpr int kLUTSize = 256;

    void buildLUT(const std::vector<KeyColor> &keys) {
        if (keys.size() < 2) {
            m_lut.assign(kLUTSize, qRgb(0, 0, 0));
            return;
        }

        constexpr int kInterpolationBase = 255;

        std::vector<QRgb> expanded;
        expanded.push_back(keys[0].color.rgba());

        for (size_t i = 1; i < keys.size(); ++i) {
            int stepCount = std::max(1, static_cast<int>(std::round(keys[i].weight * kInterpolationBase)));
            const QColor &c0 = (i > 0) ? keys[i - 1].color : keys[0].color;
            const QColor &c1 = keys[i].color;

            for (int s = 1; s <= stepCount; ++s) {
                float t = static_cast<float>(s) / stepCount;
                int r = static_cast<int>(c0.red()   + t * (c1.red()   - c0.red()));
                int g = static_cast<int>(c0.green() + t * (c1.green() - c0.green()));
                int b = static_cast<int>(c0.blue()  + t * (c1.blue()  - c0.blue()));
                int a = static_cast<int>(c0.alpha() + t * (c1.alpha() - c0.alpha()));
                expanded.push_back(qRgba(std::clamp(r, 0, 255),
                                         std::clamp(g, 0, 255),
                                         std::clamp(b, 0, 255),
                                         std::clamp(a, 0, 255)));
            }
        }

        m_lut.resize(kLUTSize);
        int n = static_cast<int>(expanded.size());
        for (int i = 0; i < kLUTSize; ++i) {
            int idx = static_cast<int>(std::round(static_cast<double>(i) / (kLUTSize - 1) * (n - 1)));
            m_lut[i] = expanded[std::clamp(idx, 0, n - 1)];
        }
    }

    QString m_name;
    std::vector<QRgb> m_lut;
};

} // namespace phonemelabeler
} // namespace dstools