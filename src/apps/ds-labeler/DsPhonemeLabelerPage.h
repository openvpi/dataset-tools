/// @file DsPhonemeLabelerPage.h
/// @brief DsLabeler PhonemeLabeler page with auto FA, batch processing, and preload.

#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

#include <QWidget>

#include <PhonemeEditor.h>

namespace dstools {

class ProjectDataSource;
class SliceListPanel;

/// @brief DsLabeler PhonemeLabeler page — project-backed phoneme editing.
///
/// Composes PhonemeEditor with ProjectDataSource. Adds automatic FA execution
/// when opening a slice without phoneme data, batch FA, and preloading.
class DsPhonemeLabelerPage : public QWidget,
                             public labeler::IPageActions,
                             public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit DsPhonemeLabelerPage(QWidget *parent = nullptr);
    ~DsPhonemeLabelerPage() override = default;

    void setDataSource(ProjectDataSource *source);

    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;
    QString windowTitle() const override;
    bool hasUnsavedChanges() const override;

    void onActivated() override;
    bool onDeactivating() override;

    [[nodiscard]] QToolBar *toolbar() const { return m_editor->toolbar(); }

signals:
    void sliceChanged(const QString &sliceId);

private:
    phonemelabeler::PhonemeEditor *m_editor = nullptr;
    SliceListPanel *m_sliceList = nullptr;
    ProjectDataSource *m_source = nullptr;
    QString m_currentSliceId;

    void onSliceSelected(const QString &sliceId);
    bool saveCurrentSlice();
    bool maybeSave();

    void onRunFA();
    void onBatchFA();
};

} // namespace dstools
