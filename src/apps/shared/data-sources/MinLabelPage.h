#pragma once

#include "EditorPageBase.h"

#include <atomic>
#include <functional>
#include <memory>

#include <MinLabelEditor.h>

namespace LyricFA {
class Asr;
class MatchLyric;
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
    std::shared_ptr<std::atomic<bool>> m_asrAlive;
    IModelManager *m_modelManager = nullptr;

    Minlabel::MinLabelEditor *m_editor = nullptr;
    bool m_dirty = false;
    bool m_asrRunning = false;

    QAction *m_playAction = nullptr;
    QAction *m_asrAction = nullptr;
    QAction *m_lyricFaAction = nullptr;

    std::unique_ptr<LyricFA::MatchLyric> m_matchLyric;
    std::shared_ptr<std::atomic<bool>> m_matchLyricAlive;

    void onRunAsr();
    void onBatchAsr();
    void runAsrForSlice(const QString &sliceId);
    void ensureAsrEngine();
    void ensureAsrEngineAsync(std::function<void()> onReady = {});
    void onModelInvalidated(const QString &taskKey);
    void setAsrResult(const QString &sliceId, const QString &text);
    void updateProgress();
    void onRunLyricFa();
    void onBatchLyricFa();
    void ensureLyricLibrary();
    void applyLyricFaResult(const QString &sliceId, const QString &matchedText);
};

} // namespace dstools
