#include "TextGridDocument.h"

#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include <QStringDecoder>

#include <algorithm>
#include <map>
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
        QMessageBox::warning(nullptr, tr("Error"), tr("Failed to open file: %1").arg(file.errorString()));
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
            QMessageBox::warning(nullptr, tr("Error"), tr("Failed to save file: %1").arg(file.errorString()));
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

TimePos TextGridDocument::totalDuration() const {
    if (tierCount() == 0) return 0;
    return secToUs(m_textGrid.GetMaxTime());
}

QString TextGridDocument::tierName(int index) const {
    if (index < 0 || index >= tierCount()) return QString();
    return QString::fromStdString(m_textGrid.GetTier(index)->GetName());
}

bool TextGridDocument::isIntervalTier(int index) const {
    if (index < 0 || index >= tierCount()) return false;
    return std::dynamic_pointer_cast<textgrid::IntervalTier>(m_textGrid.GetTier(index)) != nullptr;
}

void TextGridDocument::moveBoundary(int tierIndex, int boundaryIndex, TimePos newTime) {
    if (tierIndex < 0 || tierIndex >= tierCount()) return;

    auto tier = m_textGrid.GetTierAs<textgrid::IntervalTier>(tierIndex);
    if (!tier) return;

    const int count = static_cast<int>(tier->GetNumberOfIntervals());
    if (boundaryIndex < 0 || boundaryIndex > count) return;

    double newTimeSec = usToSec(newTime);
    const double prevBoundary = (boundaryIndex > 0) ? tier->GetInterval(boundaryIndex - 1).min_time : tier->GetMinTime();
    const double nextBoundary = (boundaryIndex < count) ? tier->GetInterval(boundaryIndex).max_time : tier->GetMaxTime();

    constexpr double kMinInterval = 0.001;
    const double clamped = std::clamp(newTimeSec, prevBoundary + kMinInterval, nextBoundary - kMinInterval);

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
    emit boundaryMoved(tierIndex, boundaryIndex, secToUs(clamped));
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

void TextGridDocument::insertBoundary(int tierIndex, TimePos time) {
    if (tierIndex < 0 || tierIndex >= tierCount()) return;

    auto tier = m_textGrid.GetTierAs<textgrid::IntervalTier>(tierIndex);
    if (!tier) return;

    double timeSec = usToSec(time);
    const size_t count = tier->GetNumberOfIntervals();
    for (size_t i = 0; i < count; ++i) {
        const auto &interval = tier->GetInterval(i);
        if (timeSec > interval.min_time && timeSec < interval.max_time) {
            textgrid::Interval left(interval.min_time, timeSec, interval.text);
            textgrid::Interval right(timeSec, interval.max_time, "");

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

TimePos TextGridDocument::boundaryTime(int tierIndex, int boundaryIndex) const {
    if (tierIndex < 0 || tierIndex >= tierCount()) return 0;

    const auto *tier = intervalTier(tierIndex);
    if (!tier) return 0;

    const int count = intervalCount(tierIndex);
    if (boundaryIndex < 0 || boundaryIndex > count) return 0;

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

TimePos TextGridDocument::intervalStart(int tierIndex, int intervalIndex) const {
    if (tierIndex < 0 || tierIndex >= tierCount()) return 0;
    const auto *tier = intervalTier(tierIndex);
    if (!tier) return 0;
    if (intervalIndex < 0 || intervalIndex >= intervalCount(tierIndex)) return 0;
    return secToUs(tier->GetInterval(intervalIndex).min_time);
}

TimePos TextGridDocument::intervalEnd(int tierIndex, int intervalIndex) const {
    if (tierIndex < 0 || tierIndex >= tierCount()) return 0;
    const auto *tier = intervalTier(tierIndex);
    if (!tier) return 0;
    if (intervalIndex < 0 || intervalIndex >= intervalCount(tierIndex)) return 0;
    return secToUs(tier->GetInterval(intervalIndex).max_time);
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

TimePos TextGridDocument::clampBoundaryTime(int tierIndex, int boundaryIndex, TimePos proposedTime) const {
    if (tierIndex < 0 || tierIndex >= tierCount()) return proposedTime;

    const auto *tier = intervalTier(tierIndex);
    if (!tier) return proposedTime;

    const int count = static_cast<int>(tier->GetNumberOfIntervals());
    if (boundaryIndex < 0 || boundaryIndex > count) return proposedTime;

    double proposedSec = usToSec(proposedTime);
    double prevBoundary = (boundaryIndex > 0) ? tier->GetInterval(boundaryIndex - 1).min_time : tier->GetMinTime();
    double nextBoundary;
    if (boundaryIndex < count) {
        nextBoundary = tier->GetInterval(boundaryIndex).max_time;
    } else {
        nextBoundary = tier->GetMaxTime();
    }

    constexpr double kEpsilon = 0.000001;
    double minClamp = prevBoundary;
    double maxClamp = nextBoundary;
    if (boundaryIndex < count)
        maxClamp = nextBoundary - kEpsilon;

    double clamped = std::clamp(proposedSec, minClamp, maxClamp);
    return secToUs(clamped);
}

TimePos TextGridDocument::snapToNearestBoundary(int tierIndex, TimePos proposedTime, TimePos snapThreshold) const {
    if (tierCount() <= 1) return proposedTime;

    double proposedSec = usToSec(proposedTime);
    double thresholdSec = usToSec(snapThreshold);

    double bestDist = thresholdSec;
    double bestTime = proposedSec;

    for (int t = 0; t < tierCount(); ++t) {
        if (t == tierIndex) continue;

        const auto *tier = intervalTier(t);
        if (!tier) continue;

        int bCount = boundaryCount(t);
        for (int b = 0; b < bCount; ++b) {
            double bTime = usToSec(boundaryTime(t, b));
            double dist = std::abs(bTime - proposedSec);
            if (dist < bestDist) {
                bestDist = dist;
                bestTime = bTime;
            }
        }
    }

    return secToUs(bestTime);
}

void TextGridDocument::loadFromDsText(const QList<IntervalLayer> &layers, TimePos duration) {
    m_textGrid = textgrid::TextGrid();
    double maxTime = usToSec(duration);
    if (maxTime <= 0.0)
        maxTime = 1.0; // fallback

    m_textGrid.SetMinTime(0.0);
    m_textGrid.SetMaxTime(maxTime);

    for (const auto &layer : layers) {
        auto tier = std::make_shared<textgrid::IntervalTier>(
            layer.name.toStdString(), 0.0, maxTime);

        // Build intervals from boundary positions.
        // dstext boundaries represent interval starts; each boundary has .pos (start) and .text.
        // We need to reconstruct intervals: [boundary[i].pos, boundary[i+1].pos) with boundary[i].text.
        if (layer.boundaries.empty()) {
            // Single interval covering entire duration
            tier->AppendInterval(textgrid::Interval(0.0, maxTime, ""));
        } else {
            // Sort boundaries by position (should already be sorted, but be safe)
            auto sorted = layer.boundaries;
            std::sort(sorted.begin(), sorted.end(), [](const Boundary &a, const Boundary &b) {
                return a.pos < b.pos;
            });

            for (size_t i = 0; i < sorted.size(); ++i) {
                double start = usToSec(sorted[i].pos);
                double end = (i + 1 < sorted.size()) ? usToSec(sorted[i + 1].pos) : maxTime;
                if (end <= start)
                    end = start + 0.001;
                tier->AppendInterval(textgrid::Interval(start, end, sorted[i].text.toStdString()));
            }

            // If first boundary doesn't start at 0, insert a silent interval before it
            double firstStart = usToSec(sorted[0].pos);
            if (firstStart > 0.001) {
                tier->InsertInterval(0, textgrid::Interval(0.0, firstStart, ""));
            }
        }

        m_textGrid.AppendTier(tier);
    }

    m_filePath.clear();
    m_modified = false;
    m_activeTierIndex = 0;
    emit documentChanged();
    emit modifiedChanged(false);
    emit activeTierChanged(0);
}

QList<IntervalLayer> TextGridDocument::toDsText() const {
    QList<IntervalLayer> result;

    for (int t = 0; t < tierCount(); ++t) {
        const auto *tier = intervalTier(t);
        if (!tier) continue;

        IntervalLayer layer;
        layer.name = tierName(t);
        layer.type = QStringLiteral("text");

        int count = static_cast<int>(tier->GetNumberOfIntervals());
        for (int i = 0; i < count; ++i) {
            const auto &interval = tier->GetInterval(i);
            Boundary b;
            b.id = i;
            b.pos = secToUs(interval.min_time);
            b.text = QString::fromStdString(interval.text);
            layer.boundaries.push_back(std::move(b));
        }

        result.append(std::move(layer));
    }

    return result;
}

// ── Binding groups ─────────────────────────────────────────────────────

void TextGridDocument::setBindingGroups(const std::vector<BindingGroup> &groups) {
    m_groups = groups;
}

int TextGridDocument::findGroupForBoundary(int boundaryId) const {
    for (size_t g = 0; g < m_groups.size(); ++g) {
        for (int id : m_groups[g]) {
            if (id == boundaryId)
                return static_cast<int>(g);
        }
    }
    return -1;
}

void TextGridDocument::autoDetectBindingGroups() {
    m_groups.clear();

    // Build a map: tier + boundaryIndex → boundaryId
    // When two boundaries from different tiers share the exact same TimePos,
    // they form a binding group.
    struct Loc { int tier; int index; int id; };
    std::vector<Loc> allBoundaries;

    for (int t = 0; t < tierCount(); ++t) {
        int n = boundaryCount(t);
        for (int b = 0; b < n; ++b) {
            allBoundaries.push_back({t, b, b}); // ID = index within tier
        }
    }

    // Group by exact time position
    std::map<TimePos, std::vector<int>> posMap;
    for (const auto &loc : allBoundaries) {
        // Generate a unique-ish cross-tier boundary ID: use tier*N+boundary+1
        // (positive, non-zero, unique across tiers)
        int uniqueId = loc.tier * 10000 + loc.index + 1;
        TimePos pos = boundaryTime(loc.tier, loc.index);
        posMap[pos].push_back(uniqueId);
    }

    for (auto &[pos, ids] : posMap) {
        if (ids.size() > 1) {
            m_groups.push_back(std::move(ids));
        }
    }
}

} // namespace phonemelabeler
} // namespace dstools
