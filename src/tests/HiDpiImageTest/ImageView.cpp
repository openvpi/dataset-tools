//
// Created by fluty on 2023/11/27.
//

#include <QDebug>
#include <QPainter>
#include <QTouchEvent>

#include "ImageView.h"

// #ifdef Q_OS_WIN
// #    include <Windows.h>
// #endif

ImageView::ImageView(QWidget *parent) {
    setAttribute(Qt::WA_AcceptTouchEvents);
    installEventFilter(this);
}
ImageView::~ImageView() {
}
void ImageView::setImage(const QPixmap &pixmap) {
    m_pixmap = pixmap;
    update();
}

void ImageView::setScaleType(const ScaleType &scaleType) {
    m_scaleType = scaleType;
    update();
}

void ImageView::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const auto pixelRatio = painter.device()->devicePixelRatioF();
    const auto rectW = rect().width();
    const auto rectH = rect().height();
    const auto scaledRectW = qRound(rectW * pixelRatio);
    const auto scaledRectH = qRound(rectH * pixelRatio);

    QRect drawRect;
    QRect cropRect;
    QPixmap drawPixmap;
    int scaledW, scaledH;
    switch (m_scaleType) {
        case CenterCrop:
            scaledH = rectH;
            scaledW = m_pixmap.scaledToHeight(scaledH).width();
            if (rectW < scaledW) {
                auto cropLeft = (scaledW - rectW) / 2;
                cropRect =
                    QRect(qRound(cropLeft * pixelRatio), 0, scaledRectW, scaledRectH);
            } else {
                scaledW = rectW;
                scaledH = m_pixmap.scaledToWidth(scaledW).height();
                auto cropTop = (scaledH - rectH) / 2;
                cropRect = QRect(0, qRound(cropTop * pixelRatio), scaledRectW, scaledRectH);
            }
            drawRect = rect();
            drawPixmap = m_pixmap.scaled(qRound(scaledW * pixelRatio), qRound(scaledH * pixelRatio),
                                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            painter.drawPixmap(drawRect, drawPixmap, cropRect);
            break;

        case CenterInside:
            scaledH = rectH;
            scaledW = m_pixmap.scaledToHeight(scaledH).width();
            if (rectW >= scaledW) {
                auto paddingLeft = (rectW - scaledW) / 2;
                drawRect = QRect(paddingLeft, 0, scaledW, scaledH);
            } else {
                scaledW = rectW;
                scaledH = m_pixmap.scaledToWidth(scaledW).height();
                auto paddingTop = (rectH - scaledH) / 2;
                drawRect = QRect(0, paddingTop, scaledW, scaledH);
            }
            drawPixmap = m_pixmap.scaled(qRound(scaledW * pixelRatio), qRound(scaledH * pixelRatio),
                                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            painter.drawPixmap(drawRect, drawPixmap);
            break;

        case Stretch:
            drawRect = rect();
            scaledW = rectW;
            scaledH = rectH;
            drawPixmap = m_pixmap.scaled(qRound(scaledW * pixelRatio), qRound(scaledH * pixelRatio),
                                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            painter.drawPixmap(drawRect, drawPixmap);
            break;
    }

    QFrame::paintEvent(event);
}

bool ImageView::eventFilter(QObject *object, QEvent *event) {
    // qDebug() << event;
    if (event->type() == QEvent::TouchBegin || event->type() == QEvent::TouchUpdate ||
        event->type() == QEvent::TouchEnd) {
        const auto e = dynamic_cast<QTouchEvent *>(event);
        qDebug() << e->type() << e->touchPoints();
        // event->ignore();
    } else if (event->type() == QEvent::MouseMove) {
        const auto e = dynamic_cast<QMouseEvent *>(event);
        if (e->source() == Qt::MouseEventSynthesizedBySystem)
            qDebug() << "MouseMove" << e->pos() << "MouseEventSynthesizedBySystem";
    }

    return QFrame::eventFilter(object, event);
}