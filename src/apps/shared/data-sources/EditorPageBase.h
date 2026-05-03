#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/AppSettings.h>
#include <dstools/ShortcutManager.h>
#include <dstools/IEditorDataSource.h>

#include <QWidget>
#include <QString>

#include <memory>

class QMenuBar;

namespace dstools {

class ISettingsBackend;
class SliceListPanel;

class EditorPageBase : public QWidget,
                       public labeler::IPageActions,
                       public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit EditorPageBase(QWidget *parent = nullptr);
    ~EditorPageBase() override;

    void setDataSource(IEditorDataSource *source, ISettingsBackend *settingsBackend);

    IEditorDataSource *source() const { return m_source; }
    ISettingsBackend *settingsBackend() const { return m_settingsBackend; }
    const QString &currentSliceId() const { return m_currentSliceId; }
    bool isDirty() const { return m_dirty; }

    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;
    bool hasUnsavedChanges() const override;
    bool supportsDragDrop() const override { return true; }
    void handleDragEnter(QDragEnterEvent *event) override;
    void handleDrop(QDropEvent *event) override;

    void onActivated() override;
    bool onDeactivating() override;
    void onShutdown() override;

    dstools::widgets::ShortcutManager *shortcutManager() const;

signals:
    void sliceChanged(const QString &sliceId);

protected:
    SliceListPanel *sliceListPanel() const { return m_sliceList; }
    void setCurrentSliceId(const QString &id) { m_currentSliceId = id; }
    void setDirty(bool dirty) { m_dirty = dirty; }

    virtual void onSliceSelected(const QString &sliceId) = 0;
    virtual bool saveCurrentSlice() = 0;
    virtual bool maybeSave();

    dstools::AppSettings *m_settings = nullptr;

    QAction *prevAction() const { return m_prevAction; }
    QAction *nextAction() const { return m_nextAction; }
    QAction *playAction() const { return m_playAction; }

private:
    SliceListPanel *m_sliceList = nullptr;
    IEditorDataSource *m_source = nullptr;
    ISettingsBackend *m_settingsBackend = nullptr;
    QString m_currentSliceId;
    bool m_dirty = false;

    dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;

    QAction *m_prevAction = nullptr;
    QAction *m_nextAction = nullptr;
    QAction *m_playAction = nullptr;
};

} // namespace dstools
