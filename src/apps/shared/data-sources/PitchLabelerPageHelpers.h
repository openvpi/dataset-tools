#pragma once

#include <QString>
#include <memory>
#include <vector>
#include <dstools/DsPitchDocument.h>

namespace Game {
    struct GameNote;
}

namespace dstools {

    class IEditorDataSource;
    class PitchLabelerPage;
    class PhNumCalculator;

    namespace pitchlabeler {
        class PitchEditor;

    }

    void pitchLabelerApplyPitchResult(IEditorDataSource *source, std::shared_ptr<pitchlabeler::DsPitchDocument> &currentFile,
                                      pitchlabeler::PitchEditor *editor, const QString &sliceId,
                                      const std::vector<float> &f0, float timestep);

    void pitchLabelerApplyMidiResult(IEditorDataSource *source, std::shared_ptr<pitchlabeler::DsPitchDocument> &currentFile,
                                     pitchlabeler::PitchEditor *editor, const QString &sliceId,
                                     const std::vector<Game::GameNote> &notes);

    void pitchLabelerRunBatchExtract(PitchLabelerPage *page);

} // namespace dstools
