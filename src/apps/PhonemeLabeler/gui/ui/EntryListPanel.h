#pragma once

#include <QWidget>
#include <QListWidget>

#include <dstools/ViewportController.h>
#include <dstools/TimePos.h>

namespace dstools {
namespace phonemelabeler {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

class TextGridDocument;

/// Panel showing phoneme entries from the active tier.
/// Scroll wheel switches entries; selected entry centers in the viewport.
class EntryListPanel : public QWidget {
    Q_OBJECT

public:
    explicit EntryListPanel(QWidget *parent = nullptr);
    ~EntryListPanel() override = default;

    void setDocument(TextGridDocument *doc);
    void setViewportController(ViewportController *viewport);

    /// Rebuild the list from the active tier's intervals.
    void rebuildEntries();

    /// Select a specific entry by index (0-based interval index in the active tier).
    void selectEntry(int index);

    /// Return the currently selected entry index (-1 if none).
    [[nodiscard]] int currentEntryIndex() const;

signals:
    /// Emitted when the user selects an entry (by click or scroll).
    void entrySelected(int tierIndex, int intervalIndex, TimePos startTime, TimePos endTime);

protected:
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void onCurrentRowChanged(int row);

private:
    void centerEntryInViewport(int intervalIndex);

    TextGridDocument *m_document = nullptr;
    ViewportController *m_viewport = nullptr;
    QListWidget *m_listWidget = nullptr;

    bool m_updatingSelection = false; // guard against re-entrant signals
};

} // namespace phonemelabeler
} // namespace dstools
