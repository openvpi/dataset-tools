//
// Created by fluty on 2024/1/23.
//

#ifndef PIANOROLLGRAPHICSVIEW_H
#define PIANOROLLGRAPHICSVIEW_H

#include "Controls/Base/CommonGraphicsView.h"
#include "Model/DsModel.h"
#include "NoteGraphicsItem.h"
#include "PianoRollGraphicsScene.h"

class PianoRollGraphicsView final : public CommonGraphicsView {
    Q_OBJECT

public:
    explicit PianoRollGraphicsView();

public slots:
    void updateView(const DsModel &model);
    void onSelectedClipChanged(const DsModel &model, int trackIndex, int clipIndex);

private:
    void paintEvent(QPaintEvent *event) override;
    PianoRollGraphicsScene *m_pianoRollScene;
    QVector<NoteGraphicsItem *> m_noteItems;
    bool m_oneSingingClipSelected = false;

    void reset();
    void insertNote(const DsNote &dsNote, int index);
};

#endif // PIANOROLLGRAPHICSVIEW_H
