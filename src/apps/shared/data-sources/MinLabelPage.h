#pragma once

#include "EditorPageBase.h"

#include <MinLabelEditor.h>

namespace LyricFA {
class Asr;
}

namespace dstools {

class IModelManager;

class MinLabelPage : public EditorPageBase {
    Q_OBJECT

public:
    explicit MinLabelPage(QWidget *parent = nullptr);
    ~MinLabelPage() override;

    // IPageActions
    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;
    bool supportsDragDrop() const override;
    void handleDragEnter(QDragEnterEvent *event) override;
    void handleDrop(QDropEvent *event) override;

protected:
    // EditorPageBase hooks
    QString windowTitlePrefix() const override;
    bool isDirty() const override;
    bool saveCurrentSlice() override;
    void onSliceSelectedImpl(const QString &sliceId) override;
    void onAutoInfer() override;
    void onDeactivatedImpl() override;

private:
    LyricFA::Asr *m_asr = nullptr;
    IModelManager *m_modelManager = nullptr;

    Minlabel::MinLabelEditor *m_editor = nullptr;
    bool m_dirty = false;
    bool m_asrRunning = false;

    QAction *m_playAction = nullptr;
    QAction *m_asrAction = nullptr;

    void onRunAsr();
    void onBatchAsr();
    void runAsrForSlice(const QString &sliceId);
    void ensureAsrEngine();
    void onModelInvalidated(const QString &taskKey);
    void setAsrResult(const QString &sliceId, const QString &text);
    void updateProgress();
};

} // namespace dstools
