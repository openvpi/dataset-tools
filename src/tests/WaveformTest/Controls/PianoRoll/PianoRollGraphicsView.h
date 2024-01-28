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
    // void selectedClipChanged();

private:
    PianoRollGraphicsScene *m_pianoRollScene;
    int m_noteId = 0;
    QVector<NoteGraphicsItem *> m_noteItems;

    void reset();
};

#endif // PIANOROLLGRAPHICSVIEW_H
