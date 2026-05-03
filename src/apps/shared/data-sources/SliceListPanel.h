#pragma once

#include <QListWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace dsfw::widgets {
class FileProgressTracker;
}

namespace dstools {

class IEditorDataSource;

class SliceListPanel : public QWidget {
    Q_OBJECT

public:
    explicit SliceListPanel(QWidget *parent = nullptr);
    ~SliceListPanel() override = default;

    void setDataSource(IEditorDataSource *source);
    void refresh();
    void setCurrentSlice(const QString &sliceId);
    [[nodiscard]] QString currentSliceId() const;
    void selectNext();
    void selectPrev();
    [[nodiscard]] int sliceCount() const;
    void setProgress(int completed, int total);
    [[nodiscard]] dsfw::widgets::FileProgressTracker *progressTracker() const;

signals:
    void sliceSelected(const QString &sliceId);

private:
    QListWidget *m_listWidget = nullptr;
    IEditorDataSource *m_source = nullptr;
    dsfw::widgets::FileProgressTracker *m_progressTracker = nullptr;

    void onCurrentRowChanged(int row);
};

} // namespace dstools
