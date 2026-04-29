#pragma once

#include <QListWidget>

namespace dstools::labeler {

class StepNavigator : public QListWidget {
    Q_OBJECT

public:
    enum StepStatus {
        NotStarted, // ○
        InProgress, // ◉
        Completed,  // ✓
        HasError    // ✕
    };

    explicit StepNavigator(QWidget *parent = nullptr);

    void setStepStatus(int step, StepStatus status);
    StepStatus stepStatus(int step) const;

signals:
    void stepSelected(int step);

private:
    void buildItems();
    void updateItemIcon(int step);

    static constexpr int StepCount = 9;
    StepStatus m_statuses[StepCount] = {};
};

} // namespace dstools::labeler
