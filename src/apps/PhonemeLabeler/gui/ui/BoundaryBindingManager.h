#pragma once

#include <QObject>
#include <QUndoCommand>
#include <vector>

#include <dstools/TimePos.h>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

struct AlignedBoundary {
    int tierIndex;
    int boundaryIndex;
    TimePos time;
};

class BoundaryBindingManager : public QObject {
    Q_OBJECT

public:
    explicit BoundaryBindingManager(TextGridDocument *doc, QObject *parent = nullptr);

    void setToleranceMs(double toleranceMs = 1.0);
    [[nodiscard]] double toleranceMs() const { return usToMs(m_toleranceUs); }

    void setEnabled(bool enabled);
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    [[nodiscard]] std::vector<AlignedBoundary> findAlignedBoundaries(
        int sourceTierIndex, int sourceBoundaryIndex) const;

    [[nodiscard]] QUndoCommand *createLinkedMoveCommand(
        int sourceTierIndex, int sourceBoundaryIndex,
        TimePos newTime, TextGridDocument *doc);

signals:
    void bindingChanged();

private:
    TextGridDocument *m_document = nullptr;
    TimePos m_toleranceUs = 1000; // default 1ms = 1000us
    bool m_enabled = true;
};

} // namespace phonemelabeler
} // namespace dstools
