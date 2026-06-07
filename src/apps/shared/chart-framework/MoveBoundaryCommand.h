#pragma once

#include <QUndoCommand>
#include <dsfw/TimePos.h>

namespace dstools {

        class IBoundaryModel;

        class MoveBoundaryCommand : public QUndoCommand {
        public:
            MoveBoundaryCommand(IBoundaryModel *model, int tierIndex, int boundaryIndex, dsfw::TimePos oldTime,
                                dsfw::TimePos newTime, QUndoCommand *parent = nullptr);

            void redo() override;
            void undo() override;

            int id() const override {
                return 1;
            }
            bool mergeWith(const QUndoCommand *other) override;

        private:
            IBoundaryModel *m_model;
            int m_tierIndex;
            int m_boundaryIndex;
            dsfw::TimePos m_oldTime;
            dsfw::TimePos m_newTime;
        };

    } // namespace dstools