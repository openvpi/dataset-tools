#pragma once

#include "TierLabelArea.h"

#include <QList>

namespace dstools {

namespace phonemelabeler {
class IBoundaryModel;
}

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
    void mousePressEvent(QMouseEvent *event) override;

private:
    void updateHeight();

    static constexpr int kTierRowHeight = 24;
    static constexpr int kLabelWidth = 70;
    bool m_alignmentRunning = false;
};

} // namespace dstools
