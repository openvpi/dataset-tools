#include "TextGridDocument.h"

#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QStringDecoder>

#include <sstream>

namespace dstools {
namespace phonemelabeler {

TextGridDocument::TextGridDocument(QObject *parent)
    : QObject(parent)
{
}

bool TextGridDocument::load(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    // Detect encoding and convert to UTF-8
    std::string utf8Content;
    if (data.size() >= 2 &&
        static_cast<unsigned char>(data[0]) == 0xFF &&
        static_cast<unsigned char>(data[1]) == 0xFE) {
        // UTF-16 LE BOM
        qWarning() << "TextGrid file has UTF-16 LE encoding, converting to UTF-8:" << path;
        auto decoder = QStringDecoder(QStringDecoder::Utf16LE);
        QString text = decoder(data.mid(2));
        utf8Content = text.toStdString();
    } else if (data.size() >= 2 &&
               static_cast<unsigned char>(data[0]) == 0xFE &&
               static_cast<unsigned char>(data[1]) == 0xFF) {
        // UTF-16 BE BOM
        qWarning() << "TextGrid file has UTF-16 BE encoding, converting to UTF-8:" << path;
        auto decoder = QStringDecoder(QStringDecoder::Utf16BE);
        QString text = decoder(data.mid(2));
        utf8Content = text.toStdString();
    } else if (data.size() >= 3 &&
               static_cast<unsigned char>(data[0]) == 0xEF &&
               static_cast<unsigned char>(data[1]) == 0xBB &&
               static_cast<unsigned char>(data[2]) == 0xBF) {
        // UTF-8 BOM — strip it
        qWarning() << "TextGrid file has UTF-8 BOM, stripping:" << path;
        utf8Content = std::string(data.constData() + 3, data.size() - 3);
    } else if (data.size() >= 4) {
        // Check for UTF-16 without BOM by looking for null byte patterns
        // ASCII text in UTF-16LE: char, 0x00, char, 0x00
        // ASCII text in UTF-16BE: 0x00, char, 0x00, char
        const auto *bytes = reinterpret_cast<const unsigned char *>(data.constData());
        if (bytes[0] != 0 && bytes[1] == 0 && bytes[2] != 0 && bytes[3] == 0) {
            qWarning() << "TextGrid file appears to be UTF-16 LE (no BOM), converting to UTF-8:" << path;
            auto decoder = QStringDecoder(QStringDecoder::Utf16LE);
            QString text = decoder(data);
            utf8Content = text.toStdString();
        } else if (bytes[0] == 0 && bytes[1] != 0 && bytes[2] == 0 && bytes[3] != 0) {
            qWarning() << "TextGrid file appears to be UTF-16 BE (no BOM), converting to UTF-8:" << path;
            auto decoder = QStringDecoder(QStringDecoder::Utf16BE);
            QString text = decoder(data);
            utf8Content = text.toStdString();
        } else {
            // Assume UTF-8/ASCII
            utf8Content = std::string(data.constData(), data.size());
        }
    } else {
        // Small file, assume UTF-8/ASCII
        utf8Content = std::string(data.constData(), data.size());
    }

    try {
        std::istringstream iss(utf8Content);
        textgrid::Parser parser(iss);
        m_textGrid = parser.Parse();
    } catch (const std::exception &e) {
        qWarning() << "Failed to parse TextGrid:" << e.what();
        return false;
    }

    m_filePath = path;
    m_modified = false;
    m_activeTierIndex = 0;
    emit documentChanged();
    emit modifiedChanged(false);
    emit activeTierChanged(0);
    return true;
}

bool TextGridDocument::save(const QString &path) {
    try {
        std::ostringstream oss;
        oss << m_textGrid;
        const std::string content = oss.str();

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        file.write(QByteArray::fromStdString(content));
        file.close();

        m_filePath = path;
        m_modified = false;
        emit modifiedChanged(false);
        return true;
    } catch (const std::exception &e) {
        qWarning() << "Failed to save TextGrid:" << e.what();
        return false;
    }
}

int TextGridDocument::tierCount() const {
    return static_cast<int>(m_textGrid.GetNumberOfTiers());
}

const textgrid::IntervalTier *TextGridDocument::intervalTier(int index) const {
    if (index < 0 || index >= tierCount()) return nullptr;
    auto tier = std::dynamic_pointer_cast<textgrid::IntervalTier>(m_textGrid.GetTier(index));
    return tier ? tier.get() : nullptr;
}

const textgrid::PointTier *TextGridDocument::pointTier(int index) const {
    if (index < 0 || index >= tierCount()) return nullptr;
    auto tier = std::dynamic_pointer_cast<textgrid::PointTier>(m_textGrid.GetTier(index));
    return tier ? tier.get() : nullptr;
}

double TextGridDocument::totalDuration() const {
    if (tierCount() == 0) return 0.0;
    return m_textGrid.GetMaxTime();
}

QString TextGridDocument::tierName(int index) const {
    if (index < 0 || index >= tierCount()) return QString();
    return QString::fromStdString(m_textGrid.GetTier(index)->GetName());
}

bool TextGridDocument::isIntervalTier(int index) const {
    if (index < 0 || index >= tierCount()) return false;
    return std::dynamic_pointer_cast<textgrid::IntervalTier>(m_textGrid.GetTier(index)) != nullptr;
}

void TextGridDocument::moveBoundary(int tierIndex, int boundaryIndex, double newTime) {
    if (tierIndex < 0 || tierIndex >= tierCount()) return;

    auto tier = m_textGrid.GetTierAs<textgrid::IntervalTier>(tierIndex);
    if (!tier) return;

    const int count = static_cast<int>(tier->GetNumberOfIntervals());
    if (boundaryIndex < 0 || boundaryIndex > count) return;

    // Clamp to valid range: prev/next refer to the neighbouring boundaries, not the current one.
    // Boundary i sits at interval[i].min_time == interval[i-1].max_time.
    // The previous boundary is interval[i-1].min_time; the next is interval[i].max_time.
    const double prevBoundary = (boundaryIndex > 0) ? tier->GetInterval(boundaryIndex - 1).min_time : tier->GetMinTime();
    const double nextBoundary = (boundaryIndex < count) ? tier->GetInterval(boundaryIndex).max_time : tier->GetMaxTime();

    constexpr double kMinInterval = 0.001; // 1ms minimum interval
    const double clamped = std::clamp(newTime, prevBoundary + kMinInterval, nextBoundary - kMinInterval);

    // Apply the move - update the affected intervals
    if (boundaryIndex > 0) {
        textgrid::Interval prev = tier->GetInterval(boundaryIndex - 1);
        prev.max_time = clamped;
        tier->SetInterval(boundaryIndex - 1, prev);
    }
    if (boundaryIndex < count) {
        textgrid::Interval next = tier->GetInterval(boundaryIndex);
        next.min_time = clamped;
        tier->SetInterval(boundaryIndex, next);
    }

    m_modified = true;
    emit boundaryMoved(tierIndex, boundaryIndex, clamped);
    emit documentChanged();
    emit modifiedChanged(true);
}

void TextGridDocument::setIntervalText(int tierIndex, int intervalIndex, const QString &text) {
    if (tierIndex < 0 || tierIndex >= tierCount()) return;

    auto tier = m_textGrid.GetTierAs<textgrid::IntervalTier>(tierIndex);
    if (!tier) return;
    if (intervalIndex < 0 || intervalIndex >= static_cast<int>(tier->GetNumberOfIntervals())) return;

    textgrid::Interval interval = tier->GetInterval(intervalIndex);
    interval.text = text.toStdString();
    tier->SetInterval(intervalIndex, interval);

    m_modified = true;
    emit intervalTextChanged(tierIndex, intervalIndex);
    emit documentChanged();
    emit modifiedChanged(true);
}

void TextGridDocument::insertBoundary(int tierIndex, double time) {
    if (tierIndex < 0 || tierIndex >= tierCount()) return;

    auto tier = m_textGrid.GetTierAs<textgrid::IntervalTier>(tierIndex);
    if (!tier) return;

    const size_t count = tier->GetNumberOfIntervals();
    for (size_t i = 0; i < count; ++i) {
        const auto &interval = tier->GetInterval(i);
        if (time > interval.min_time && time < interval.max_time) {
            // Split this interval
            textgrid::Interval left(interval.min_time, time, interval.text);
            textgrid::Interval right(time, interval.max_time, "");

            tier->SetInterval(i, left);
            tier->InsertInterval(i + 1, right);
            break;
        }
    }

    m_modified = true;
    emit boundaryInserted(tierIndex, static_cast<int>(tier->GetNumberOfIntervals()));
    emit documentChanged();
    emit modifiedChanged(true);
}

void TextGridDocument::removeBoundary(int tierIndex, int boundaryIndex) {
    if (tierIndex < 0 || tierIndex >= tierCount()) return;

    auto tier = m_textGrid.GetTierAs<textgrid::IntervalTier>(tierIndex);
    if (!tier) return;

    const int count = static_cast<int>(tier->GetNumberOfIntervals());
    if (boundaryIndex < 1 || boundaryIndex >= count) return; // Don't remove first/last

    // Merge interval[boundaryIndex-1] and interval[boundaryIndex]
    textgrid::Interval merged(tier->GetInterval(boundaryIndex - 1).min_time,
                              tier->GetInterval(boundaryIndex).max_time,
                              tier->GetInterval(boundaryIndex - 1).text);
    tier->SetInterval(boundaryIndex - 1, merged);
    tier->RemoveInterval(boundaryIndex);

    m_modified = true;
    emit boundaryRemoved(tierIndex, boundaryIndex);
    emit documentChanged();
    emit modifiedChanged(true);
}

double TextGridDocument::boundaryTime(int tierIndex, int boundaryIndex) const {
    if (tierIndex < 0 || tierIndex >= tierCount()) return 0.0;

    const auto *tier = intervalTier(tierIndex);
    if (!tier) return 0.0;

    const int count = intervalCount(tierIndex);
    if (boundaryIndex < 0 || boundaryIndex > count) return 0.0;

    if (boundaryIndex == count) {
        return intervalEnd(tierIndex, boundaryIndex - 1);
    }
    return intervalStart(tierIndex, boundaryIndex);
}

int TextGridDocument::boundaryCount(int tierIndex) const {
    return intervalCount(tierIndex) + 1;
}

int TextGridDocument::intervalCount(int tierIndex) const {
    if (tierIndex < 0 || tierIndex >= tierCount()) return 0;
    const auto *tier = intervalTier(tierIndex);
    if (!tier) return 0;
    return static_cast<int>(tier->GetNumberOfIntervals());
}

QString TextGridDocument::intervalText(int tierIndex, int intervalIndex) const {
    if (tierIndex < 0 || tierIndex >= tierCount()) return QString();
    const auto *tier = intervalTier(tierIndex);
    if (!tier) return QString();
    if (intervalIndex < 0 || intervalIndex >= intervalCount(tierIndex)) return QString();
    return QString::fromStdString(tier->GetInterval(intervalIndex).text);
}

double TextGridDocument::intervalStart(int tierIndex, int intervalIndex) const {
    if (tierIndex < 0 || tierIndex >= tierCount()) return 0.0;
    const auto *tier = intervalTier(tierIndex);
    if (!tier) return 0.0;
    if (intervalIndex < 0 || intervalIndex >= intervalCount(tierIndex)) return 0.0;
    return tier->GetInterval(intervalIndex).min_time;
}

double TextGridDocument::intervalEnd(int tierIndex, int intervalIndex) const {
    if (tierIndex < 0 || tierIndex >= tierCount()) return 0.0;
    const auto *tier = intervalTier(tierIndex);
    if (!tier) return 0.0;
    if (intervalIndex < 0 || intervalIndex >= intervalCount(tierIndex)) return 0.0;
    return tier->GetInterval(intervalIndex).max_time;
}

void TextGridDocument::setModified(bool modified) {
    if (m_modified != modified) {
        m_modified = modified;
        emit modifiedChanged(modified);
    }
}

void TextGridDocument::setActiveTierIndex(int index) {
    if (index < 0 || index >= tierCount()) return;
    if (m_activeTierIndex != index) {
        m_activeTierIndex = index;
        emit activeTierChanged(index);
    }
}

} // namespace phonemelabeler
} // namespace dstools
