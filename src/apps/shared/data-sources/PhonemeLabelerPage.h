#pragma once

#include "EditorPageBase.h"

#include <PhonemeEditor.h>
#include <atomic>
#include <functional>
#include <memory>

namespace HFA {
    class HFA;
}

namespace dstools {

    class MoeCurveProcessor;

    class PhonemeLabelerPage : public EditorPageBase {
        Q_OBJECT

    public:
        explicit PhonemeLabelerPage(QWidget *parent = nullptr);
        ~PhonemeLabelerPage() override;

        QMenuBar *createMenuBar(QWidget *parent) override;
        QWidget *createStatusBarContent(QWidget *parent) override;

        [[nodiscard]] QToolBar *toolbar() const {
            return m_editor->toolbar();
        }

    protected:
        QString windowTitlePrefix() const override;
        bool isDirty() const override;
        bool saveCurrentSlice() override;
        void onSliceSelectedImpl(const QString &sliceId) override;
        void onAutoInferPreloadEngines() override;
        void onAutoInferProcessDirty(const QStringList &dirty) override;
        void restoreExtraSplitters() override;
        void saveExtraSplitters() override;
        void onDeactivatedImpl() override;
        BatchSliceResult processSlice(const QString &sliceId) override;

    private:
        phonemelabeler::PhonemeEditor *m_editor = nullptr;

        QAction *m_faAction = nullptr;

        QAction *m_actExtractF0 = nullptr;
        QAction *m_actExtractMidi = nullptr;

        HFA::HFA *m_hfa = nullptr;

        MoeCurveProcessor *m_moeProcessor = nullptr;

        void onRunFA();
        void onBatchFA();
        void onEngineInvalidated(const QString &taskKey) override;
        void runFaForSlice(const QString &sliceId);
        void applyFaResult(const QString &sliceId, const QList<IntervalLayer> &layers,
                           const std::vector<BindingGroup> &groups = {},
                           const std::vector<LayerDependency> &dependencies = {});
        void updateProgress();
        void loadFaResultIntoEditor(const DsTextDocument &doc);
        void onExtractF0();
        void onExtractMidi();
    };

} // namespace dstools
