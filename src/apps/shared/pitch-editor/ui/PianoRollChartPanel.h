#pragma once

#include "PianoRollView.h"

#include <ChartPanelBase.h>
#include <QPainter>
#include <QWidget>
#include <dsfw/widgets/PlayWidget.h>
#include <dstools/TimePos.h>
#include <dsfw/widgets/ViewportController.h>
#include <dstools/DsPitchDocument.h>

namespace dstools {
    namespace pitchlabeler {


        namespace ui {

            /// Piano roll chart panel that wraps PianoRollView in ChartPanelBase architecture
            class PianoRollChartPanel : public chart::ChartPanelBase {
                Q_OBJECT

            public:
                explicit PianoRollChartPanel(dsfw::widgets::ViewportController *vc, QWidget *parent = nullptr);

                static void registerChartConfig();

                void setDsPitchDocument(std::shared_ptr<DsPitchDocument> file);
                void setUndoStack(QUndoStack *stack);
                void setBoundaryModel(chart::IBoundaryModel *model);

                PianoRollView *pianoRollView() const {
                    return m_pianoRoll;
                }

                void onViewportUpdate(const chart::ChartCoordinate &conv, int pixelWidth) override;
                void onBoundaryModelInvalidated() override;
                void paintYAxisContent(QPainter &painter, const QRect &rect) override;

                // F-01: 新增纯虚方法实现
                void renderFullData(QImage &image) override;
                double dataDurationSec() const override;

            signals:
                void noteSelected(int noteIndex);
                void toolModeChanged(int mode);
                void fileEdited();
                void positionClicked(double time, double midi);
                void rulerClicked(double timeSec);
                void noteGlideChanged(int noteIndex, const QString &glide);
                void noteSlurToggled(int noteIndex);
                void noteRestToggled(int noteIndex);
                void noteMergeLeft(int noteIndex);
                void noteDeleteRequested(const std::vector<int> &indices);

            private:
                void loadConfigParams();

                PianoRollView *m_pianoRoll = nullptr;

                // Config values from ChartConfigRegistry
                int m_configPianoWidth = 52;
                int m_configScrollBarSize = 14;
                int m_configMinMidi = 24;
                int m_configMaxMidi = 96;
                double m_configModSensitivity = 80.0;
            };

        } // namespace ui
    } // namespace pitchlabeler
} // namespace dstools