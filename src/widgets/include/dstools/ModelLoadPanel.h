#pragma once
#include <dstools/WidgetsGlobal.h>
#include <QWidget>

class QLabel;
class QPushButton;

namespace dstools::widgets {

class PathSelector;

/// Composite widget: PathSelector (OpenFile) + "Load" button + status label for model loading.
class DSTOOLS_WIDGETS_API ModelLoadPanel : public QWidget {
    Q_OBJECT
public:
    explicit ModelLoadPanel(const QString &label, const QString &filter,
                            QWidget *parent = nullptr);

    QString path() const;
    PathSelector *pathSelector() const;

public slots:
    void setStatus(const QString &text, bool success = true);
    void setLoading(bool loading);

signals:
    void loadRequested(const QString &path);
    void pathChanged(const QString &path);

private:
    PathSelector *m_pathSelector;
    QPushButton *m_loadBtn;
    QLabel *m_statusLabel;
};

} // namespace dstools::widgets
