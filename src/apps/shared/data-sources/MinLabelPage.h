#pragma once

#include "EditorPageBase.h"

#include <MinLabelEditor.h>
#include <atomic>
#include <functional>
#include <memory>

namespace LyricFA {
    class Asr;
    class MatchLyric;
}

namespace dstools {

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
        void onAutoInferProcessDirty(const QStringList &dirty) override;
        void onDeactivatedImpl() override;

        // Batch processing overrides
        bool hasExistingResult(const QString &sliceId) const override;
        bool prepareSliceInput(const QString &sliceId, QString &skipReason) override;
        BatchSliceResult processSlice(const QString &sliceId) override;
        void applyBatchResult(const QString &sliceId, const BatchSliceResult &result) override;

    private:
        LyricFA::Asr *m_asr = nullptr;

        Minlabel::MinLabelEditor *m_editor = nullptr;
        bool m_dirty = false;

        QAction *m_undoAction = nullptr;
        QAction *m_redoAction = nullptr;

        QAction *m_playAction = nullptr;
        QAction *m_asrAction = nullptr;
        QAction *m_lyricFaAction = nullptr;

        QString m_pendingAsrText;
        QString m_pendingLyricFaInput;
        QString m_pendingLyricFaText;
        bool m_batchAutoG2P = false;
        bool m_batchIsLyricFA = false;

        std::unique_ptr<LyricFA::MatchLyric> m_matchLyric;

        void onRunAsr();
        void onBatchAsr();
        void runAsrForSlice(const QString &sliceId);
        void onEngineInvalidated(const QString &taskKey) override;
        void setAsrResult(const QString &sliceId, const QString &text);
        void batchG2P(const QString &sliceId);
        void updateProgress();
        void onRunLyricFa();
        void onBatchLyricFa();
        void ensureLyricLibrary();
        void applyLyricFaResult(const QString &sliceId, const QString &matchedText);
        void updateUndoRedoState();
    };

} // namespace dstools
