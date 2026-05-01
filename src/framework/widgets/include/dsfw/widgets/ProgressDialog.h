#pragma once

#include <dsfw/widgets/WidgetsGlobal.h>

#include <QDialog>

class QProgressBar;
class QLabel;
class QPushButton;

namespace dsfw::widgets {

class DSFW_WIDGETS_API ProgressDialog : public QDialog {
    Q_OBJECT
public:
    explicit ProgressDialog(const QString &title = {}, QWidget *parent = nullptr);

    void setRange(int minimum, int maximum);
    void setValue(int value);
    int value() const;

    void setLabelText(const QString &text);
    QString labelText() const;

    void setCancelButtonEnabled(bool enabled);

signals:
    void canceled();

private:
    QProgressBar *m_progressBar;
    QLabel *m_label;
    QPushButton *m_cancelBtn;
    int m_value = 0;
};

} // namespace dsfw::widgets
