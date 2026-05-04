#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/AppSettings.h>
#include <dstools/ShortcutManager.h>

#include <QWidget>

#include <MinLabelEditor.h>

namespace LyricFA {
class Asr;
}

namespace dstools {

class IEditorDataSource;
class ISettingsBackend;
class SliceListPanel;
class IModelManager;

class MinLabelPage : public QWidget,
                     public labeler::IPageActions,
                     public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit MinLabelPage(QWidget *parent = nullptr);
    ~MinLabelPage() override;

    void setDataSource(IEditorDataSource *source, ISettingsBackend *settingsBackend);

    // IPageActions
    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;
    QString windowTitle() const override;
    bool hasUnsavedChanges() const override;
    bool supportsDragDrop() const override;
    void handleDragEnter(QDragEnterEvent *event) override;
    void handleDrop(QDropEvent *event) override;

    // IPageLifecycle
    void onActivated() override;
    bool onDeactivating() override;
    void onDeactivated() override;
    void onShutdown() override;

    dstools::widgets::ShortcutManager *shortcutManager() const;

signals:
    void sliceChanged(const QString &sliceId);

private:
    LyricFA::Asr *m_asr = nullptr;
    IModelManager *m_modelManager = nullptr;

    Minlabel::MinLabelEditor *m_editor = nullptr;
    SliceListPanel *m_sliceList = nullptr;
    IEditorDataSource *m_source = nullptr;
    ISettingsBackend *m_settingsBackend = nullptr;
    QString m_currentSliceId;
    bool m_dirty = false;
    bool m_asrRunning = false;

    dstools::AppSettings m_settings;
    dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;

    QAction *m_prevAction = nullptr;
    QAction *m_nextAction = nullptr;
    QAction *m_playAction = nullptr;

    void onSliceSelected(const QString &sliceId);
    bool saveCurrentSlice();
    bool maybeSave();

    void onRunAsr();
    void onBatchAsr();
    void runAsrForSlice(const QString &sliceId);
    void ensureAsrEngine();
    void onModelInvalidated(const QString &taskKey);
    void setAsrResult(const QString &sliceId, const QString &text);
    void updateProgress();
};

} // namespace dstools
