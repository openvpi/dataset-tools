//
// Created by fluty on 2023/11/27.
//

#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QFrame>

class ImageView : public QFrame {
public:
    void setImage(const QPixmap &pixmap);

protected:
    void paintEvent(QPaintEvent *event) override;

    QPixmap m_pixmap;
};



#endif //IMAGEVIEW_H
