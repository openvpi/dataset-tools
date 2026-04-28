#pragma once

#include <QObject>
#include <QUndoCommand>
#include <vector>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

struct AlignedBoundary {
    int tierIndex;
    int boundaryIndex;
    double time;
};

class BoundaryBindingManager : public QObject {
    Q_OBJECT

public:
    explicit BoundaryBindingManager(TextGridDocument *doc, QObject *parent = nullptr);

    // Tolerance: boundaries within this many ms are considered aligned
    void setTolerance(double toleranceMs = 1.0);
    [[nodiscard]] double tolerance() const { return m_tolerance; }

    // Enable/disable binding
    void setEnabled(bool enabled);
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    // Find all boundaries in other tiers that are aligned with the source boundary
    [[nodiscard]] std::vector<AlignedBoundary> findAlignedBoundaries(
        int sourceTierIndex, int sourceBoundaryIndex) const;

    // Create a linked move command for an aligned group
    [[nodiscard]] QUndoCommand *createLinkedMoveCommand(
        int sourceTierIndex, int sourceBoundaryIndex,
        double newTime, TextGridDocument *doc);

signals:
    void bindingChanged();

private:
    TextGridDocument *m_document = nullptr;
    double m_tolerance = 1.0; // ms
    bool m_enabled = true;
};

} // namespace phonemelabeler
} // namespace dstools
