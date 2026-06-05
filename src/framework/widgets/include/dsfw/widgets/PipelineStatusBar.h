#pragma once

/// @file PipelineStatusBar.h
/// @brief Horizontal pipeline step indicator bar for EditorPageBase.
///
/// Displays pipeline steps as clickable status indicators:
/// - ✓ completed (green)
/// - ⏳ in progress (blue)
/// - ○ pending (gray)
/// - ✕ failed (red)
/// - Manual step: orange border

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QEvent>
#include <QStringList>
#include <QVector>
#include <map>

#include <dsfw/TaskTypes.h>
#include <dsfw/widgets/WidgetsGlobal.h>

namespace dsfw {
    struct PipelineContext;
    struct StepRecord;
}

namespace dsfw::widgets {

class DSFW_WIDGETS_API PipelineStatusBar : public QWidget {
    Q_OBJECT

public:
    struct StepInfo {
        QString taskName;
        QString displayName;
        bool isManual = false;
    };

    explicit PipelineStatusBar(QWidget *parent = nullptr);

    void setSteps(const QVector<StepInfo> &steps);
    void setContext(const dsfw::PipelineContext *ctx);
    void clearContext();

signals:
    void stepClicked(const QString &taskName);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void rebuild();
    static QString statusChar(const StepInfo &step, const dsfw::PipelineContext *ctx);

    QHBoxLayout *m_layout = nullptr;
    QVector<StepInfo> m_steps;
    const dsfw::PipelineContext *m_ctx = nullptr;
    QVector<QLabel *> m_labels;
};

} // namespace dsfw::widgets