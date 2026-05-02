#pragma once

#include <QUndoCommand>
#include <QString>
#include <cstdint>
#include <vector>
#include <memory>

namespace dstools {
namespace pitchlabeler {

class DSFile;

namespace ui {

/// Undo command for shifting note pitch by delta cents.
class PitchMoveCommand : public QUndoCommand {
public:
    PitchMoveCommand(std::shared_ptr<DSFile> dsFile, std::vector<int> indices, int deltaCents,
                     QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1001; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    std::shared_ptr<DSFile> m_dsFile;
    std::vector<int> m_indices;
    int m_deltaCents;
};

/// Undo command for modulation/drift F0 adjustment.
class ModulationDriftCommand : public QUndoCommand {
public:
    ModulationDriftCommand(std::shared_ptr<DSFile> dsFile,
                           std::vector<int32_t> oldF0Values,
                           std::vector<int32_t> newF0Values,
                           QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    std::shared_ptr<DSFile> m_dsFile;
    std::vector<int32_t> m_oldF0;
    std::vector<int32_t> m_newF0;
};

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
