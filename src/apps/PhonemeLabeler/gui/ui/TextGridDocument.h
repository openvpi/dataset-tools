#pragma once

#include <QObject>
#include <QString>
#include <vector>
#include <memory>

// textgrid.hpp is in src/libs/textgrid/
#include <textgrid.hpp>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument : public QObject {
    Q_OBJECT

public:
    explicit TextGridDocument(QObject *parent = nullptr);

    // I/O
    bool load(const QString &path);
    bool save(const QString &path);
    [[nodiscard]] bool isModified() const { return m_modified; }
    [[nodiscard]] QString filePath() const { return m_filePath; }

    // Read-only access
    [[nodiscard]] int tierCount() const;
    [[nodiscard]] const textgrid::IntervalTier *intervalTier(int index) const;
    [[nodiscard]] const textgrid::PointTier *pointTier(int index) const;
    [[nodiscard]] double totalDuration() const;

    // Tier meta info
    [[nodiscard]] QString tierName(int index) const;
    [[nodiscard]] bool isIntervalTier(int index) const;

    // Mutation (called by UndoCommands)
    void moveBoundary(int tierIndex, int boundaryIndex, double newTime);
    void setIntervalText(int tierIndex, int intervalIndex, const QString &text);
    void insertBoundary(int tierIndex, double time);
    void removeBoundary(int tierIndex, int boundaryIndex);

    // Boundary query
    [[nodiscard]] double boundaryTime(int tierIndex, int boundaryIndex) const;
    [[nodiscard]] int boundaryCount(int tierIndex) const;
    [[nodiscard]] int intervalCount(int tierIndex) const;

    // Interval query
    [[nodiscard]] QString intervalText(int tierIndex, int intervalIndex) const;
    [[nodiscard]] double intervalStart(int tierIndex, int intervalIndex) const;
    [[nodiscard]] double intervalEnd(int tierIndex, int intervalIndex) const;

    // Active tier (whose boundaries are shown across all charts)
    [[nodiscard]] int activeTierIndex() const { return m_activeTierIndex; }
    void setActiveTierIndex(int index);

signals:
    void documentChanged();
    void boundaryMoved(int tierIndex, int boundaryIndex, double newTime);
    void intervalTextChanged(int tierIndex, int intervalIndex);
    void boundaryInserted(int tierIndex, int boundaryIndex);
    void boundaryRemoved(int tierIndex, int boundaryIndex);
    void modifiedChanged(bool modified);
    void activeTierChanged(int tierIndex);

private:
    void setModified(bool modified);

    textgrid::TextGrid m_textGrid;
    QString m_filePath;
    bool m_modified = false;
    int m_activeTierIndex = 0;
};

} // namespace phonemelabeler
} // namespace dstools
