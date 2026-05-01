#pragma once
#include <dsfw/widgets/WidgetsGlobal.h>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

namespace dsfw::widgets {

class DSFW_WIDGETS_API PathSelector : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
public:
    enum Mode { OpenFile, SaveFile, Directory };

    explicit PathSelector(Mode mode, const QString &label,
                          const QString &filter = {}, QWidget *parent = nullptr);

    QString path() const;
    void setPath(const QString &path);
    void setPlaceholder(const QString &text);
    Mode mode() const;
    QLineEdit *lineEdit() const;

signals:
    void pathChanged(const QString &path);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void onBrowseClicked();

    Mode m_mode;
    QString m_filter;
    QLabel *m_label;
    QLineEdit *m_lineEdit;
    QPushButton *m_browseBtn;
};

} // namespace dsfw::widgets
