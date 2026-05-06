#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/Log.h>

#include <QComboBox>
#include <QPushButton>
#include <QSet>
#include <QWidget>

namespace dsfw::widgets {
class LogViewer;
}

namespace dstools {

class LogPage : public QWidget, public labeler::IPageActions, public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit LogPage(QWidget *parent = nullptr);
    ~LogPage() override = default;

    QMenuBar *createMenuBar(QWidget *parent) override;
    QString windowTitle() const override;

    void onActivated() override;
    void onDeactivated() override;

private:
    void appendEntry(const dstools::LogEntry &entry);
    void rebuildCategoryFilter();
    void applyCategoryFilter();
    void exportToFile();
    void clear();

    dsfw::widgets::LogViewer *m_viewer = nullptr;
    QComboBox *m_categoryFilter = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_exportButton = nullptr;
    QSet<QString> m_knownCategories;
    QSet<QString> m_activeCategories;
};

} // namespace dstools
