#include "NoteBoundaryModel.h"
#include <dstools/DsPitchDocument.h>

#include <algorithm>
#include <cmath>

namespace dstools {
namespace pitchlabeler {
namespace ui {

NoteBoundaryModel::NoteBoundaryModel(QObject *parent)
    : QObject(parent)
{
}

void NoteBoundaryModel::setDsPitchDocument(std::shared_ptr<DsPitchDocument> file)
{
    m_file = std::move(file);
    emit modelInvalidated();
}

int NoteBoundaryModel::boundaryCount(int tierIndex) const
{
    if (tierIndex != 0 || !m_file || m_file->notes.empty()) {
        return 0;
    }
    // Each note has 2 boundaries: start and end
    return static_cast<int>(m_file->notes.size() * 2);
}

dsfw::TimePos NoteBoundaryModel::boundaryTime(int tierIndex, int boundaryIndex) const
{
    if (tierIndex != 0 || !m_file || m_file->notes.empty()) {
        return dsfw::TimePos(0);
    }

    int noteCount = static_cast<int>(m_file->notes.size());
    if (boundaryIndex < 0 || boundaryIndex >= noteCount * 2) {
        return dsfw::TimePos(0);
    }

    int noteIdx = boundaryIndex / 2;
    bool isStart = (boundaryIndex % 2 == 0);
    const auto &note = m_file->notes[noteIdx];

    if (isStart) {
        return dsfw::TimePos(note.start);
    } else {
        return dsfw::TimePos(note.end());
    }
}

void NoteBoundaryModel::moveBoundary(int tierIndex, int boundaryIndex, dsfw::TimePos newTime)
{
    if (tierIndex != 0 || !m_file || m_file->notes.empty()) {
        return;
    }

    int noteCount = static_cast<int>(m_file->notes.size());
    if (boundaryIndex < 0 || boundaryIndex >= noteCount * 2) {
        return;
    }

    int noteIdx = boundaryIndex / 2;
    bool isStart = (boundaryIndex % 2 == 0);
    auto &note = m_file->notes[noteIdx];

    // Clamp the new time to valid range
    dsfw::TimePos clampedTime = clampBoundaryTime(tierIndex, boundaryIndex, newTime);
    qint64 newTimeUs = clampedTime;

    if (isStart) {
        // Update start time
        note.start = newTimeUs;
        // If start > end, adjust end to maintain minimum duration
        if (note.start > note.end()) {
            note.duration = 0;
        }
    } else {
        // Update end time (via duration)
        qint64 newDuration = newTimeUs - note.start;
        if (newDuration >= 0) {
            note.duration = newDuration;
        }
    }

    // Handle overlapping boundaries: if this boundary overlaps with adjacent note's boundary,
    // adjust the adjacent boundary to maintain consistency
    if (isStart && noteIdx > 0) {
        auto &prevNote = m_file->notes[noteIdx - 1];
        if (prevNote.end() > note.start) {
            prevNote.duration = note.start - prevNote.start;
        }
    } else if (!isStart && noteIdx + 1 < noteCount) {
        auto &nextNote = m_file->notes[noteIdx + 1];
        if (note.end() > nextNote.start) {
            nextNote.start = note.end();
        }
    }

    emit noteBoundaryChanged(noteIdx, isStart);
}

dsfw::TimePos NoteBoundaryModel::totalDuration() const
{
    if (!m_file) {
        return dsfw::TimePos(0);
    }
    return dsfw::TimePos(m_file->getTotalDuration());
}

dsfw::TimePos NoteBoundaryModel::clampBoundaryTime(int tierIndex, int boundaryIndex, dsfw::TimePos proposedTime) const
{
    if (tierIndex != 0 || !m_file || m_file->notes.empty()) {
        return proposedTime;
    }

    int noteCount = static_cast<int>(m_file->notes.size());
    if (boundaryIndex < 0 || boundaryIndex >= noteCount * 2) {
        return proposedTime;
    }

    int noteIdx = boundaryIndex / 2;
    bool isStart = (boundaryIndex % 2 == 0);
    const auto &note = m_file->notes[noteIdx];

    // Basic constraints: start < end for the same note
    dsfw::TimePos minTime(0);
    dsfw::TimePos maxTime = totalDuration();

    if (isStart) {
        // Start boundary: cannot be after note's end
        dsfw::TimePos noteEnd = dsfw::TimePos(note.end());
        maxTime = std::min(maxTime, noteEnd);
        
        // Cannot be before previous note's end (if exists)
        if (noteIdx > 0) {
            const auto &prevNote = m_file->notes[noteIdx - 1];
            minTime = std::max(minTime, dsfw::TimePos(prevNote.end()));
        }
    } else {
        // End boundary: cannot be before note's start
        dsfw::TimePos noteStart = dsfw::TimePos(note.start);
        minTime = std::max(minTime, noteStart);
        
        // Cannot be after next note's start (if exists)
        if (noteIdx + 1 < noteCount) {
            const auto &nextNote = m_file->notes[noteIdx + 1];
            maxTime = std::min(maxTime, dsfw::TimePos(nextNote.start));
        }
    }

    // Apply constraints
    if (proposedTime < minTime) return minTime;
    if (proposedTime > maxTime) return maxTime;
    return proposedTime;
}

dsfw::TimePos NoteBoundaryModel::snapToNearestBoundary(int tierIndex, dsfw::TimePos proposedTime, dsfw::TimePos snapThreshold) const
{
    if (tierIndex != 0 || !m_file || m_file->notes.empty() || snapThreshold <= dsfw::TimePos(0)) {
        return proposedTime;
    }

    dsfw::TimePos bestTime = proposedTime;
    dsfw::TimePos bestDist = snapThreshold + dsfw::TimePos(1); // Initialize larger than threshold

    // Check all boundaries for snapping
    int count = boundaryCount(tierIndex);
    for (int i = 0; i < count; ++i) {
        dsfw::TimePos boundary = boundaryTime(tierIndex, i);
        dsfw::TimePos dist = std::abs(boundary - proposedTime);
        if (dist < bestDist) {
            bestDist = dist;
            bestTime = boundary;
        }
    }

    return bestTime;
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
