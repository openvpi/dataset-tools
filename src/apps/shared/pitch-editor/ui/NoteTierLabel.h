/// @file NoteTierLabel.h
/// @brief Tier label area for displaying note numbers and pitch names.

#pragma once

#include "../../phoneme-editor/ui/TierLabelArea.h"

#include <memory>
#include <dstools/DsPitchDocument.h>

namespace dstools {
    namespace pitchlabeler {


        namespace ui {

            /// @brief Tier label area for displaying note numbers and pitch names.
            ///
            /// Displays note information in the format: "1 C4 | 2 D4 | 3 E4"
            /// Used in PitchEditor's AudioVisualizerContainer.
            class NoteTierLabel : public TierLabelArea {
                Q_OBJECT

            public:
                explicit NoteTierLabel(QWidget *parent = nullptr);
                ~NoteTierLabel() override = default;

                /// @brief Set the DsPitchDocument containing notes.
                void setDsPitchDocument(std::shared_ptr<DsPitchDocument> file);

                /// @brief Get the current DsPitchDocument.
                [[nodiscard]] std::shared_ptr<DsPitchDocument> dsFile() const {
                    return m_file;
                }

            protected:
                void paintEvent(QPaintEvent *event) override;

            private:
                std::shared_ptr<DsPitchDocument> m_file;
            };

        } // namespace ui
    } // namespace pitchlabeler
} // namespace dstools