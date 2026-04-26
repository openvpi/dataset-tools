#pragma once

#include <QFile>
#include <QObject>
#include <QString>
#include <QStringList>

namespace dsfw {

    class ISliceDataSource : public QObject {
        Q_OBJECT

    public:
        using QObject::QObject;
        ~ISliceDataSource() noexcept override = default;

        [[nodiscard]] virtual int getSliceCount() const = 0;

        [[nodiscard]] virtual QStringList sliceIds() const = 0;

        [[nodiscard]] virtual QString sliceIdAt(int index) const {
            const auto ids = sliceIds();
            if (index >= 0 && index < ids.size())
                return ids.at(index);
            return {};
        }

        [[nodiscard]] virtual QString audioPath(const QString &sliceId) const = 0;

        [[nodiscard]] virtual bool audioExists(const QString &sliceId) const {
            return QFile::exists(audioPath(sliceId));
        }

        [[nodiscard]] QString validatedAudioPath(const QString &sliceId) const {
            const QString path = audioPath(sliceId);
            if (!path.isEmpty() && audioExists(sliceId))
                return path;
            return {};
        }

        [[nodiscard]] QStringList missingAudioSlices() const {
            QStringList missing;
            for (const auto &id : sliceIds()) {
                if (!audioExists(id))
                    missing.append(id);
            }
            return missing;
        }

        [[nodiscard]] virtual double sliceDuration(const QString &sliceId) const {
            Q_UNUSED(sliceId)
            return -1.0;
        }

        [[nodiscard]] virtual QStringList dirtyLayers(const QString &sliceId) const {
            Q_UNUSED(sliceId)
            return {};
        }

        virtual void clearDirtyLayers(const QString &sliceId, const QStringList &layers) {
            Q_UNUSED(sliceId)
            Q_UNUSED(layers)
        }

        virtual void addDirtyLayers(const QString &sliceId, const QStringList &modifiedLayers) {
            Q_UNUSED(sliceId)
            Q_UNUSED(modifiedLayers)
        }

    signals:
        void sliceListChanged();
        void sliceSaved(const QString &sliceId);
        void audioAvailabilityChanged();
    };

} // namespace dsfw