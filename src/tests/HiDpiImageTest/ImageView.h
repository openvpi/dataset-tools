//
// Created by fluty on 2023/11/27.
//

#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QFrame>

class ImageView : public QFrame {
public:
    enum ScaleType { CenterInside, CenterCrop, Stretch };

    explicit ImageView(QWidget *parent = nullptr);
    ~ImageView();

    void setImage(const QPixmap &pixmap);
    void setScaleType(const ScaleType &scaleType);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

    QPixmap m_pixmap;
    ScaleType m_scaleType = CenterInside;
};



#endif // IMAGEVIEW_H
