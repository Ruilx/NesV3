#include "NesTileItem.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

NesTileItem::NesTileItem(QGraphicsItem *parent) : QGraphicsItem(parent) {
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
}

QRectF NesTileItem::boundingRect() const {
    return QRectF(0, 0, TileSize, TileSize);
}

void NesTileItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    if (this->palette.isEmpty()) {
        painter->fillRect(boundingRect(), Qt::black);
        return;
    }

    for (int y = 0; y < TileSize; ++y) {
        for (int x = 0; x < TileSize; ++x) {
            const int pixelIndex = y * TileSize + x;
            const quint8 colorIndex = this->pixels.value(pixelIndex, 0);
            const QColor color = this->palette.value(colorIndex, Qt::black);
            painter->fillRect(x, y, 1, 1, color);
        }
    }

    if (this->gridVisible) {
        painter->setPen(QPen(QColor(255, 255, 255, 80), 0));
        painter->drawRect(boundingRect());
    }
}

void NesTileItem::setPixels(const QVector<quint8> &pixels) {
    this->pixels = pixels;
    this->pixels.resize(TileSize * TileSize);
    update();
}

void NesTileItem::setPalette(const QVector<QColor> &palette) {
    this->palette = palette;
    update();
}

void NesTileItem::setGridVisible(bool visible) {
    if (this->gridVisible == visible) {
        return;
    }

    this->gridVisible = visible;
    update();
}
