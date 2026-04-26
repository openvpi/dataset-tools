/// @file NoteBoundaryModel.h
/// @brief Boundary model adapter for DsPitchDocument notes (MIDI pitch boundaries).

#pragma once

#include <IBoundaryModel.h>
#include <QObject>
#include <memory>
#include <dstools/DsPitchDocument.h>

namespace dstools {
    namespace pitchlabeler {


        namespace ui {

            /// @brief Boundary model adapter for DsPitchDocument notes (MIDI pitch boundaries).
            ///
            /// Maps DsPitchDocument notes to IBoundaryModel interface for use with AudioVisualizerContainer.
            /// Each note has two boundaries (start and end), allowing boundary drag interaction.
            class NoteBoundaryModel : public QObject, public chart::IBoundaryModel {
                Q_OBJECT

            public:
                explicit NoteBoundaryModel(QObject *parent = nullptr);
                ~NoteBoundaryModel() override = default;

                /// @brief Set the DsPitchDocument containing notes.
                void setDsPitchDocument(std::shared_ptr<DsPitchDocument> file);

                /// @brief Get the current DsPitchDocument.
                [[nodiscard]] std::shared_ptr<DsPitchDocument> dsFile() const {
                    return m_file;
                }

                // IBoundaryModel interface
                [[nodiscard]] int tierCount() const override {
                    return 1;
                }
                [[nodiscard]] QString tierName(int /*tierIndex*/) const override {
                    return "notes";
                }
                [[nodiscard]] int activeTierIndex() const override {
                    return 0;
                }
                [[nodiscard]] int boundaryCount(int tierIndex) const override;
                [[nodiscard]] TimePos boundaryTime(int tierIndex, int boundaryIndex) const override;
                void moveBoundary(int tierIndex, int boundaryIndex, TimePos newTime) override;
                [[nodiscard]] TimePos totalDuration() const override;
                [[nodiscard]] bool supportsBinding() const override {
                    return false;
                }
                [[nodiscard]] bool supportsInsert() const override {
                    return true;
                }

                [[nodiscard]] TimePos clampBoundaryTime(int tierIndex, int boundaryIndex,
                                                        TimePos proposedTime) const override;
                [[nodiscard]] TimePos snapToNearestBoundary(int tierIndex, TimePos proposedTime,
                                                            TimePos snapThreshold) const override;

            signals:
                /// @brief Emitted when a note boundary is changed (during drag preview).
                /// @param noteIndex The index of the note whose boundary changed.
                /// @param isStart True if the start boundary changed, false if end boundary.
                void noteBoundaryChanged(int noteIndex, bool isStart);

                /// @brief Emitted when the model data is invalidated (e.g., after drag completion).
                void modelInvalidated();

            private:
                std::shared_ptr<DsPitchDocument> m_file;
            };

        } // namespace ui
    } // namespace pitchlabeler
} // namespace dstools