#include "NesSpriteItem.h"

#include <QPainter>

NesSpriteItem::NesSpriteItem(QGraphicsItem *parent)
    : QGraphicsItem(parent) {
    setVisible(false);
}

QRectF NesSpriteItem::boundingRect() const {
    return QRectF(0, 0, this->width, this->height);
}

void NesSpriteItem::paint(QPainter *painter,
    const QStyleOptionGraphicsItem *, QWidget *) {
    if (this->visibleSprite && !this->image.isNull()) {
        painter->drawImage(QPointF(0, 0), this->image);
    }
}

void NesSpriteItem::setSprite(
    int width,
    int height,
    const QVector<quint8> &pixels,
    const QVector<quint8> &paletteIndices,
    const NesPalette &palette) {
    prepareGeometryChange();
    this->width = width;
    this->height = height;
    this->image = QImage(width, height, QImage::Format_ARGB32);
    this->image.fill(Qt::transparent);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int index = y * width + x;
            if (pixels.value(index, 0) == 0) {
                continue;
            }
            this->image.setPixelColor(
                x, y, palette.colorAt(paletteIndices.value(index, 0)));
        }
    }
    this->visibleSprite = true;
    setVisible(true);
    update();
}

void NesSpriteItem::setVisibleSprite(bool visible) {
    if (this->visibleSprite == visible) {
        return;
    }
    this->visibleSprite = visible;
    setVisible(visible);
    update();
}