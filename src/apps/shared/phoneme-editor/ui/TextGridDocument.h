#pragma once

#include <QObject>
#include <QString>
#include <vector>
#include <memory>

#include <textgrid.hpp>
#include <dstools/TimePos.h>
#include <dstools/DsTextTypes.h>

#include "IBoundaryModel.h"

namespace dstools {
namespace phonemelabeler {

class TextGridDocument : public QObject, public IBoundaryModel {
    Q_OBJECT

public:
    explicit TextGridDocument(QObject *parent = nullptr);

    bool load(const QString &path);
    bool save(const QString &path);

    /// Build an in-memory TextGrid from dstext IntervalLayers.
    /// @param layers  IntervalLayer list from DsTextDocument.
    /// @param duration Total audio duration in microseconds (sets maxTime).
    void loadFromDsText(const QList<IntervalLayer> &layers, TimePos duration);

    /// Export all tiers as dstext IntervalLayers.
    QList<IntervalLayer> toDsText() const;

    [[nodiscard]] bool isModified() const { return m_modified; }
    [[nodiscard]] QString filePath() const { return m_filePath; }

    [[nodiscard]] int tierCount() const override;

    [[nodiscard]] const textgrid::IntervalTier *intervalTier(int index) const;
    [[nodiscard]] const textgrid::PointTier *pointTier(int index) const;

    [[nodiscard]] TimePos totalDuration() const override;

    [[nodiscard]] QString tierName(int index) const override;
    [[nodiscard]] bool isIntervalTier(int index) const;

    void moveBoundary(int tierIndex, int boundaryIndex, TimePos newTime) override;
    void setIntervalText(int tierIndex, int intervalIndex, const QString &text);
    void insertBoundary(int tierIndex, TimePos time);
    void removeBoundary(int tierIndex, int boundaryIndex);

    [[nodiscard]] TimePos boundaryTime(int tierIndex, int boundaryIndex) const override;
    [[nodiscard]] int boundaryCount(int tierIndex) const override;
    [[nodiscard]] int intervalCount(int tierIndex) const;

    [[nodiscard]] QString intervalText(int tierIndex, int intervalIndex) const;
    [[nodiscard]] TimePos intervalStart(int tierIndex, int intervalIndex) const;
    [[nodiscard]] TimePos intervalEnd(int tierIndex, int intervalIndex) const;

    [[nodiscard]] int activeTierIndex() const override { return m_activeTierIndex; }
    void setActiveTierIndex(int index);

    // IBoundaryModel
    [[nodiscard]] bool supportsBinding() const override { return true; }
    [[nodiscard]] bool supportsInsert() const override { return true; }

    [[nodiscard]] TimePos clampBoundaryTime(int tierIndex, int boundaryIndex, TimePos proposedTime) const override;
    [[nodiscard]] TimePos snapToLowerTier(int tierIndex, TimePos proposedTime, TimePos snapThreshold) const override;

signals:
    void documentChanged();
    void boundaryMoved(int tierIndex, int boundaryIndex, TimePos newTime);
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
