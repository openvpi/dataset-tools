/// @file TextGridDocument.h
/// @brief TextGrid document model with undo/redo mutation support.

#pragma once

#include <QObject>
#include <QString>
#include <vector>
#include <memory>

// textgrid.hpp is in src/libs/textgrid/
#include <textgrid.hpp>

namespace dstools {
namespace phonemelabeler {

/// @brief QObject wrapping a textgrid::TextGrid with methods for tier/interval/boundary
///        access and mutation, emitting signals on changes.
class TextGridDocument : public QObject {
    Q_OBJECT

public:
    /// @brief Constructs an empty TextGrid document.
    /// @param parent Optional parent QObject.
    explicit TextGridDocument(QObject *parent = nullptr);

    /// @brief Loads a TextGrid file from disk.
    /// @param path File path to load.
    /// @return True on success.
    bool load(const QString &path);

    /// @brief Saves the TextGrid to disk.
    /// @param path File path to save to.
    /// @return True on success.
    bool save(const QString &path);

    /// @brief Returns whether the document has unsaved modifications.
    [[nodiscard]] bool isModified() const { return m_modified; }

    /// @brief Returns the file path of the loaded document.
    [[nodiscard]] QString filePath() const { return m_filePath; }

    /// @brief Returns the number of tiers.
    [[nodiscard]] int tierCount() const;

    /// @brief Returns the interval tier at the given index, or nullptr.
    /// @param index Tier index.
    [[nodiscard]] const textgrid::IntervalTier *intervalTier(int index) const;

    /// @brief Returns the point tier at the given index, or nullptr.
    /// @param index Tier index.
    [[nodiscard]] const textgrid::PointTier *pointTier(int index) const;

    /// @brief Returns the total duration of the TextGrid in seconds.
    [[nodiscard]] double totalDuration() const;

    /// @brief Returns the name of a tier.
    /// @param index Tier index.
    [[nodiscard]] QString tierName(int index) const;

    /// @brief Returns whether the tier at the given index is an interval tier.
    /// @param index Tier index.
    [[nodiscard]] bool isIntervalTier(int index) const;

    /// @brief Moves a boundary to a new time position.
    /// @param tierIndex Tier index.
    /// @param boundaryIndex Boundary index within the tier.
    /// @param newTime New time in seconds.
    void moveBoundary(int tierIndex, int boundaryIndex, double newTime);

    /// @brief Sets the text of an interval.
    /// @param tierIndex Tier index.
    /// @param intervalIndex Interval index.
    /// @param text New text value.
    void setIntervalText(int tierIndex, int intervalIndex, const QString &text);

    /// @brief Inserts a new boundary at the given time.
    /// @param tierIndex Tier index.
    /// @param time Time position in seconds.
    void insertBoundary(int tierIndex, double time);

    /// @brief Removes a boundary from the tier.
    /// @param tierIndex Tier index.
    /// @param boundaryIndex Boundary index to remove.
    void removeBoundary(int tierIndex, int boundaryIndex);

    /// @brief Returns the time of a boundary.
    /// @param tierIndex Tier index.
    /// @param boundaryIndex Boundary index.
    [[nodiscard]] double boundaryTime(int tierIndex, int boundaryIndex) const;

    /// @brief Returns the number of boundaries in a tier.
    /// @param tierIndex Tier index.
    [[nodiscard]] int boundaryCount(int tierIndex) const;

    /// @brief Returns the number of intervals in a tier.
    /// @param tierIndex Tier index.
    [[nodiscard]] int intervalCount(int tierIndex) const;

    /// @brief Returns the text of an interval.
    /// @param tierIndex Tier index.
    /// @param intervalIndex Interval index.
    [[nodiscard]] QString intervalText(int tierIndex, int intervalIndex) const;

    /// @brief Returns the start time of an interval.
    /// @param tierIndex Tier index.
    /// @param intervalIndex Interval index.
    [[nodiscard]] double intervalStart(int tierIndex, int intervalIndex) const;

    /// @brief Returns the end time of an interval.
    /// @param tierIndex Tier index.
    /// @param intervalIndex Interval index.
    [[nodiscard]] double intervalEnd(int tierIndex, int intervalIndex) const;

    /// @brief Returns the index of the active tier.
    [[nodiscard]] int activeTierIndex() const { return m_activeTierIndex; }

    /// @brief Sets the active tier index.
    /// @param index Tier index to activate.
    void setActiveTierIndex(int index);

signals:
    void documentChanged();                                      ///< Document content changed.
    void boundaryMoved(int tierIndex, int boundaryIndex, double newTime); ///< Boundary was moved.
    void intervalTextChanged(int tierIndex, int intervalIndex);   ///< Interval text was changed.
    void boundaryInserted(int tierIndex, int boundaryIndex);     ///< Boundary was inserted.
    void boundaryRemoved(int tierIndex, int boundaryIndex);      ///< Boundary was removed.
    void modifiedChanged(bool modified);                         ///< Modified flag changed.
    void activeTierChanged(int tierIndex);                       ///< Active tier changed.

private:
    void setModified(bool modified);                             ///< Updates the modified flag.

    textgrid::TextGrid m_textGrid;  ///< Underlying TextGrid data.
    QString m_filePath;             ///< Path of the loaded file.
    bool m_modified = false;        ///< Whether unsaved changes exist.
    int m_activeTierIndex = 0;      ///< Currently active tier index.
};

} // namespace phonemelabeler
} // namespace dstools
