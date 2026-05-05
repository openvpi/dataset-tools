#pragma once

#include "TierLabelArea.h"

namespace dstools {

class SliceTierLabel : public TierLabelArea {
    Q_OBJECT

public:
    explicit SliceTierLabel(QWidget *parent = nullptr);
    ~SliceTierLabel() override = default;

    int activeTierIndex() const override;
    void setActiveTierIndex(int index) override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

} // namespace dstools
