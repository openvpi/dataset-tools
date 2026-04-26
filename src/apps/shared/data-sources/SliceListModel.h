#pragma once

#include <QAbstractListModel>
#include <QIcon>
#include <QStringList>
#include <vector>

namespace dsfw::widgets {
    class FileProgressTracker;
}

namespace dstools {

    class IEditorDataSource;
    class AppSettings;

    class SliceListModel : public QAbstractListModel {
        Q_OBJECT

    public:
        enum CustomRoles {
            SliceIdRole = Qt::UserRole,
            DurationRole = Qt::UserRole + 1,
            BaseTextRole = Qt::UserRole + 2,
            DirtyRole = Qt::UserRole + 3,
            AudioExistsRole = Qt::UserRole + 4,
            DirtyLayersRole = Qt::UserRole + 5,
            DiscardedRole = Qt::UserRole + 6,
            StartTimeRole = Qt::UserRole + 10,
            EndTimeRole = Qt::UserRole + 11,
        };

        explicit SliceListModel(QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        void setDataSource(IEditorDataSource *source);
        void refresh();
        void setCurrentSlice(const QString &sliceId);
        [[nodiscard]] QString currentSliceId() const;
        void selectNext();
        void selectPrev();
        [[nodiscard]] int sliceCount() const;

        void setSliceDirty(const QString &sliceId, bool dirty);
        void setSliceDirtyLayers(const QString &sliceId, const QStringList &dirtyLayers);

        QString ensureSelection(AppSettings &settings);
        void saveSelection(AppSettings &settings) const;

        void setSlicerMode(bool enabled);
        [[nodiscard]] bool isSlicerMode() const { return m_slicerMode; }
        void setSliceData(const std::vector<double> &slicePoints, double totalDuration,
                          const QString &prefix = QStringLiteral("slice"));
        void setDiscarded(int index, bool discarded);
        [[nodiscard]] std::vector<int> discardedIndices() const;
        [[nodiscard]] double sliceStart(int index) const;
        [[nodiscard]] double sliceEnd(int index) const;

    signals:
        void currentSliceChanged(const QString &sliceId);

    private:
        struct Item {
            QString id;
            double duration = 0.0;
            double startTime = 0.0;
            double endTime = 0.0;
            bool dirty = false;
            bool audioExists = true;
            bool discarded = false;
            QString baseText;
            QStringList dirtyLayers;
        };

        void rebuildEditorItems();
        void rebuildItemText(int index);
        [[nodiscard]] QString buildDisplayText(int index) const;
        [[nodiscard]] QString buildEditorText(const Item &item) const;

        std::vector<Item> m_items;
        IEditorDataSource *m_source = nullptr;
        bool m_slicerMode = false;
        int m_currentIndex = -1;
        std::vector<int> m_discardedInSlicer;

        static constexpr double kMinDuration = 0.5;
        static constexpr double kMaxDuration = 30.0;
    };

} // namespace dstools