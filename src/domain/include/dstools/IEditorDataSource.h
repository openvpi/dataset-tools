#pragma once

#include <QString>
#include <QStringList>
#include <dsfw/ISliceDataSource.h>
#include <dsfw/PipelineContext.h>
#include <dstools/DsTextTypes.h>
#include <dstools/Result.h>

namespace dstools {

    class IEditorDataSource : public dsfw::ISliceDataSource {
        Q_OBJECT

    public:
        static constexpr int kInterfaceVersion = 1;

        using dsfw::ISliceDataSource::ISliceDataSource;
        ~IEditorDataSource() noexcept override = default;

        [[nodiscard]] virtual QStringList sliceIds() const = 0;

        [[nodiscard]] virtual PipelineContext *context(const QString &sliceId) {
            Q_UNUSED(sliceId)
            return nullptr;
        }

        [[nodiscard]] virtual Result<DsTextDocument> loadSlice(const QString &sliceId) = 0;

        [[nodiscard]] virtual Result<void> saveSlice(const QString &sliceId, const DsTextDocument &doc) = 0;

        virtual void setLayerManuallyEdited(const QString &sliceId, const QString &layer, bool edited) {
            Q_UNUSED(sliceId)
            Q_UNUSED(layer)
            Q_UNUSED(edited)
        }

        [[nodiscard]] virtual bool isLayerManuallyEdited(const QString &sliceId, const QString &layer) const {
            Q_UNUSED(sliceId)
            Q_UNUSED(layer)
            return false;
        }

        virtual void addEditedStep(const QString &sliceId, const QString &step) {
            Q_UNUSED(sliceId)
            Q_UNUSED(step)
        }
    };

} // namespace dstools