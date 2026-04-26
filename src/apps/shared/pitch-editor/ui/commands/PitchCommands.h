#pragma once

#include <QString>
#include <QUndoCommand>
#include <cstdint>
#include <memory>
#include <vector>
#include <dstools/DsPitchDocument.h>

namespace dstools {
    namespace pitchlabeler {


        namespace ui {

            /// Undo command for shifting note pitch by delta cents.
            class PitchMoveCommand : public QUndoCommand {
            public:
                PitchMoveCommand(std::shared_ptr<DsPitchDocument> dsFile, std::vector<int> indices, int deltaCents,
                                 QUndoCommand *parent = nullptr);

                void undo() override;
                void redo() override;
                int id() const override {
                    return 1001;
                }
                bool mergeWith(const QUndoCommand *other) override;

            private:
                std::shared_ptr<DsPitchDocument> m_dsFile;
                std::vector<int> m_indices;
                int m_deltaCents;
            };

            /// Undo command for modulation/drift F0 adjustment.
            class ModulationDriftCommand : public QUndoCommand {
            public:
                ModulationDriftCommand(std::shared_ptr<DsPitchDocument> dsFile, std::vector<float> oldF0Values,
                                       std::vector<float> newF0Values, QUndoCommand *parent = nullptr);

                void undo() override;
                void redo() override;

            private:
                std::shared_ptr<DsPitchDocument> m_dsFile;
                std::vector<float> m_oldF0;
                std::vector<float> m_newF0;
            };

        } // namespace ui
    } // namespace pitchlabeler
} // namespace dstools
