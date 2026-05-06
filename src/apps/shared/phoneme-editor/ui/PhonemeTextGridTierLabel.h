#pragma once

#include "TierLabelArea.h"

#include <QList>

namespace dstools {

namespace phonemelabeler {
class IBoundaryModel;
}

/// @brief Tier label area for PhonemeEditor.
/// Draws boundary lines for all tiers (active = solid, non-active = dashed).
/// Tier selection is handled by the toolbar combo box (not part of this widget).
class PhonemeTextGridTierLabel : public TierLabelArea {
    Q_OBJECT

public:
    explicit PhonemeTextGridTierLabel(QWidget *parent = nullptr);
    ~PhonemeTextGridTierLabel() override = default;

    void setBoundaryModel(IBoundaryModel *model);

    int activeTierIndex() const override;
    void setActiveTierIndex(int index) override;

    QList<double> activeBoundaries() const override;
    QList<QList<double>> allTierBoundaries() const;

    int tierCount() const;
    int tierRowHeight() const;

    void onModelDataChanged() override;

    void setAlignmentRunning(bool running);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    static constexpr int kTierRowHeight = 24;
    bool m_alignmentRunning = false;
};

} // namespace dstools
