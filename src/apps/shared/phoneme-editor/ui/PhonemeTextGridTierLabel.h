#pragma once

#include "TierLabelArea.h"

#include <QList>
#include <QRadioButton>
#include <QButtonGroup>
#include <QVBoxLayout>

namespace dstools {

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

    /// Show/hide alignment-in-progress indicator.
    void setAlignmentRunning(bool running);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuildRadioButtons();

    QButtonGroup *m_buttonGroup = nullptr;
    QVBoxLayout *m_radioLayout = nullptr;
    QWidget *m_radioPanel = nullptr;
    QList<QRadioButton *> m_radioButtons;

    static constexpr int kTierRowHeight = 24;
    static constexpr int kRadioPanelWidth = 28;

    bool m_alignmentRunning = false;
    int m_leftMargin = kRadioPanelWidth;
};

} // namespace dstools
