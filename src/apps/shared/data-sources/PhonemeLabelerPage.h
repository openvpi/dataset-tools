#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/AppSettings.h>
#include <dstools/ShortcutManager.h>

#include <QWidget>
#include <QSplitter>

#include <PhonemeEditor.h>

namespace HFA {
class HFA;
}

namespace dstools {

class IEditorDataSource;
class ISettingsBackend;
class SliceListPanel;
class IModelManager;

class PhonemeLabelerPage : public QWidget,
                           public labeler::IPageActions,
                           public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit PhonemeLabelerPage(QWidget *parent = nullptr);
    ~PhonemeLabelerPage() override;

    void setDataSource(IEditorDataSource *source, ISettingsBackend *settingsBackend);

    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;
    QString windowTitle() const override;
    bool hasUnsavedChanges() const override;

    void onActivated() override;
    bool onDeactivating() override;
    void onDeactivated() override;
    void onShutdown() override;

    [[nodiscard]] QToolBar *toolbar() const { return m_editor->toolbar(); }
    dstools::widgets::ShortcutManager *shortcutManager() const;

signals:
    void sliceChanged(const QString &sliceId);

private:
    phonemelabeler::PhonemeEditor *m_editor = nullptr;
    SliceListPanel *m_sliceList = nullptr;
    QSplitter *m_splitter = nullptr;
    IEditorDataSource *m_source = nullptr;
    ISettingsBackend *m_settingsBackend = nullptr;
    IModelManager *m_modelManager = nullptr;
    QString m_currentSliceId;

    dstools::AppSettings m_settings;
    dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;

    QAction *m_prevAction = nullptr;
    QAction *m_nextAction = nullptr;

    HFA::HFA *m_hfa = nullptr;
    bool m_faRunning = false;

    void onSliceSelected(const QString &sliceId);
    bool saveCurrentSlice();
    bool maybeSave();

    void onRunFA();
    void onBatchFA();
    void ensureHfaEngine();
    void onModelInvalidated(const QString &taskKey);
    void runFaForSlice(const QString &sliceId);
    void applyFaResult(const QString &sliceId, const QList<IntervalLayer> &layers);
};

} // namespace dstools
