#pragma once

#include "SliceListModel.h"

#include <QListView>
#include <QVBoxLayout>
#include <QWidget>
#include <dsfw/AppSettings.h>
#include <vector>

namespace dsfw::widgets {
    class FileProgressTracker;
}

namespace dstools {

    class IEditorDataSource;

    class SliceListPanel : public QWidget {
        Q_OBJECT

    public:
        explicit SliceListPanel(QWidget *parent = nullptr);
        ~SliceListPanel() override = default;

        void setDataSource(IEditorDataSource *source);
        void refresh();
        void setCurrentSlice(const QString &sliceId);
        [[nodiscard]] QString currentSliceId() const;
        void selectNext();
        void selectPrev();
        [[nodiscard]] int sliceCount() const;
        void setProgress(int completed, int total);
        [[nodiscard]] dsfw::widgets::FileProgressTracker *progressTracker() const;
        void setSliceDirty(const QString &sliceId, bool dirty);
        void setSliceDirtyLayers(const QString &sliceId, const QStringList &dirtyLayers);
        void setSliceLoadError(const QString &sliceId, const QString &error);

        QString ensureSelection(AppSettings &settings);
        void saveSelection(AppSettings &settings) const;

        void setSlicerMode(bool enabled);
        void setSliceData(const std::vector<double> &slicePoints, double totalDuration,
                          const QString &prefix = QStringLiteral("slice"));
        void setDiscarded(int index, bool discarded);
        [[nodiscard]] std::vector<int> discardedIndices() const {
            return m_model ? m_model->discardedIndices() : std::vector<int>{};
        }

    signals:
        void sliceSelected(const QString &sliceId);

        void sliceDoubleClicked(int index, double startSec, double endSec);
        void discardToggled(int index, bool discarded);
        void addSlicePointRequested(double timeSec);
        void removeSlicePointRequested(int pointIndex);

    private:
        QListView *m_listView = nullptr;
        SliceListModel *m_model = nullptr;
        IEditorDataSource *m_source = nullptr;
        dsfw::widgets::FileProgressTracker *m_progressTracker = nullptr;

        bool m_slicerMode = false;
        std::vector<double> m_slicePoints;
        double m_totalDuration = 0.0;

        static constexpr double kMinDuration = 0.5;
        static constexpr double kMaxDuration = 30.0;

        void onCurrentRowChanged(const QModelIndex &current, const QModelIndex &previous);
        void onContextMenu(const QPoint &pos);
    };

} // namespace dstools