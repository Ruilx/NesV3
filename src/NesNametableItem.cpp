#include "NesNametableItem.h"

#include "NesTileItem.h"

#include "NesPalette.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

NesNametableItem::NesNametableItem(QGraphicsItem *parent)
    : QGraphicsItem(parent) {
    this->tiles.reserve(TileCount);
    for (int tileY = 0; tileY < Height / 8; ++tileY) {
        for (int tileX = 0; tileX < Width / 8; ++tileX) {
            auto *tile = new NesTileItem(this);
            tile->setPos(tileX * 8, tileY * 8);
            this->tiles.append(tile);
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
