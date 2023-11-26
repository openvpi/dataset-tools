//
// Created by fluty on 2023/11/27.
//

#include <QPainter>
#include <QDebug>

#include "ImageView.h"

void ImageView::setImage(const QPixmap &pixmap) {
    m_pixmap = pixmap;
    update();
}
void ImageView::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const auto pixelRatio = painter.device()->devicePixelRatioF();
    auto scaledPixmap = m_pixmap.scaled(rect().width() * pixelRatio, rect().height() * pixelRatio,
        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    qDebug() << "Screen Scale" << pixelRatio;
    painter.drawPixmap(rect(), scaledPixmap);

    QFrame::paintEvent(event);
}