#include "NesTileItem.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

NesTileItem::PaintStats NesTileItem::statistics;

NesTileItem::NesTileItem(QGraphicsItem *parent)
    : QGraphicsItem(parent), image(TileSize, TileSize, QImage::Format_ARGB32) {
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
    this->image.fill(Qt::black);
}

QRectF NesTileItem::boundingRect() const {
    return QRectF(0, 0, TileSize, TileSize);
}

void NesTileItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    ++NesTileItem::statistics.paintCalls;
    painter->drawImage(QPointF(0, 0), this->image);

    if (this->gridVisible) {
        painter->setPen(QPen(QColor(255, 255, 255, 80), 0));
        painter->drawRect(boundingRect());
    }
}

void NesTileItem::setPixels(const QVector<quint8> &pixels) {
    this->pixels = pixels;
    this->pixels.resize(TileSize * TileSize);
    this->colorIndices = this->pixels;
    this->colorIndices.resize(TileSize * TileSize);
    rebuildImage();
    ++NesTileItem::statistics.updateRequests;
    update();
}

void NesTileItem::setPaletteIndices(
        const QVector<quint8> &pixels,
        const QVector<quint8> &subpalette,
        const NesPalette &palette) {
    if (subpalette.size() != 4) {
        return;
    }

    this->pixels = pixels;
    this->pixels.resize(TileSize * TileSize);
    this->palette = palette.colors();
    for (int y = 0; y < TileSize; ++y) {
        for (int x = 0; x < TileSize; ++x) {
            const int index = y * TileSize + x;
            this->colorIndices[index] = subpalette.value(
                    this->pixels.value(index, 0), subpalette.value(0));
            this->image.setPixelColor(
                    x,
                    y,
                    palette.colorAt(this->colorIndices[index]));
        }
    }
    ++NesTileItem::statistics.updateRequests;
    update();
}

void NesTileItem::setPalette(const QVector<QColor> &palette) {
    this->palette = palette;
    rebuildImage();
    ++NesTileItem::statistics.updateRequests;
    update();
}

void NesTileItem::setGridVisible(bool visible) {
    if (this->gridVisible == visible) {
        return;
    }

    this->gridVisible = visible;
    ++NesTileItem::statistics.updateRequests;
    update();
}

void NesTileItem::rebuildImage() {
    ++NesTileItem::statistics.imageRebuilds;
    for (int y = 0; y < TileSize; ++y) {
        for (int x = 0; x < TileSize; ++x) {
            const int pixelIndex = y * TileSize + x;
            this->image.setPixelColor(
                    x,
                    y,
                    this->palette.value(this->colorIndices.value(pixelIndex, 0), Qt::black));
        }
    }
}

void NesTileItem::resetPaintStats() {
    NesTileItem::statistics = PaintStats();
}

NesTileItem::PaintStats NesTileItem::paintStats() {
    return NesTileItem::statistics;
}
