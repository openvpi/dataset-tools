#pragma once

#include "EditorPageBase.h"

#include <atomic>
#include <functional>
#include <memory>

#include <PhonemeEditor.h>

namespace HFA {
class HFA;
}

namespace dstools {

class IModelManager;

class PhonemeLabelerPage : public EditorPageBase {
    Q_OBJECT

public:
    explicit PhonemeLabelerPage(QWidget *parent = nullptr);
    ~PhonemeLabelerPage() override;

    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;

    [[nodiscard]] QToolBar *toolbar() const { return m_editor->toolbar(); }

protected:
    QString windowTitlePrefix() const override;
    bool isDirty() const override;
    bool saveCurrentSlice() override;
    void onSliceSelectedImpl(const QString &sliceId) override;
    void onAutoInfer() override;
    void restoreExtraSplitters() override;
    void saveExtraSplitters() override;
    void onDeactivatedImpl() override;

private:
    phonemelabeler::PhonemeEditor *m_editor = nullptr;
    IModelManager *m_modelManager = nullptr;

    QAction *m_faAction = nullptr;

    HFA::HFA *m_hfa = nullptr;
    bool m_faRunning = false;
    std::shared_ptr<std::atomic<bool>> m_hfaAlive;

    void onRunFA();
    void onBatchFA();
    void ensureHfaEngine();
    void ensureHfaEngineAsync(std::function<void()> onReady = {});
    void onModelInvalidated(const QString &taskKey);
    void runFaForSlice(const QString &sliceId);
    void applyFaResult(const QString &sliceId, const QList<IntervalLayer> &layers,
                       const std::vector<BindingGroup> &groups = {},
                       const std::vector<LayerDependency> &dependencies = {});
    void updateProgress();
};

} // namespace dstools
