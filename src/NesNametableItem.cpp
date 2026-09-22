#include "NesNametableItem.h"

#include "NesTileItem.h"

#include "NesPalette.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <cmath>

namespace {
qreal wrappedCoordinate(qreal basePosition, qreal scrollOffset, qreal period, qreal viewportSize) {
    qreal position = basePosition - scrollOffset;
    while (position < -viewportSize) {
        position += period;
    }
    while (position > viewportSize) {
        position -= period;
    }
    return position;
}
}

NesNametableItem::NesNametableItem(QGraphicsItem *parent)
    : QGraphicsItem(parent) {
    this->tiles.reserve(TileCount);
    this->projectedTilePositions.reserve(TileCount);
    for (int tileY = 0; tileY < Height / 8; ++tileY) {
        for (int tileX = 0; tileX < Width / 8; ++tileX) {
            auto *tile = new NesTileItem(this);
            const QPointF position(tileX * 8, tileY * 8);
            tile->setPos(position);
            this->tiles.append(tile);
            this->projectedTilePositions.append(position);
        }
    }
}

QRectF NesNametableItem::boundingRect() const {
    return QRectF(0, 0, Width, Height);
}

void NesNametableItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    Q_UNUSED(painter);
}

void NesNametableItem::setPalette(const NesPalette &palette) {
    this->palette = palette;
    for (NesTileItem *tile : this->tiles) {
        tile->setPalette(this->palette.colors());
    }
}

void NesNametableItem::setSubpalette(const QVector<quint8> &subpalette) {
    if (subpalette.size() != 4) {
        return;
    }

    this->subpalette = subpalette;
}

void NesNametableItem::setRasterScroll(
        const QVector<QPointF> &scrollByScanline,
        const QPointF &basePosition,
        const QPointF &parentPosition) {
    if (scrollByScanline.isEmpty()) {
        return;
    }

    for (int tileY = 0; tileY < Height / 8; ++tileY) {
        const int scanline = qMin(tileY * 8, scrollByScanline.size() - 1);
        const QPointF rowScroll = scrollByScanline.at(scanline);
        const QPointF rowNametablePosition(
            wrappedCoordinate(basePosition.x(), rowScroll.x(), Width * 2, Width),
            wrappedCoordinate(basePosition.y(), rowScroll.y(), Height * 2, Height));
        for (int tileX = 0; tileX < Width / 8; ++tileX) {
            const int tileIndex = tileY * (Width / 8) + tileX;
            const QPointF tilePosition(
                tileX * 8 + rowNametablePosition.x() - parentPosition.x(),
                tileY * 8 + rowNametablePosition.y() - parentPosition.y());
            if (this->projectedTilePositions.at(tileIndex) != tilePosition) {
                this->tiles.value(tileIndex)->setPos(tilePosition);
                this->projectedTilePositions[tileIndex] = tilePosition;
            }
        }
    }
}

void NesNametableItem::setTilePixels(int tileX, int tileY, const QVector<quint8> &tilePixels) {
	setTilePixels(tileX, tileY, tilePixels, this->subpalette);
}

void NesNametableItem::setTilePixels(
        int tileX,
        int tileY,
        const QVector<quint8> &tilePixels,
        const QVector<quint8> &tileSubpalette) {
    if (tileX < 0 || tileX >= Width / 8 || tileY < 0 || tileY >= Height / 8) {
        return;
    }
	if (tileSubpalette.size() != 4) {
		return;
	}

    const int tileIndex = tileY * (Width / 8) + tileX;
    this->tiles.value(tileIndex)->setPaletteIndices(
            tilePixels, tileSubpalette, this->palette);
}
