#include "NesNametableItem.h"

#include "NesPalette.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

NesNametableItem::NesNametableItem(QGraphicsItem *parent)
    : QGraphicsItem(parent), image(Width, Height, QImage::Format_ARGB32) {
    image.fill(Qt::black);
}

QRectF NesNametableItem::boundingRect() const {
    return QRectF(0, 0, Width, Height);
}

void NesNametableItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->drawImage(QPointF(0, 0), this->image);
}

void NesNametableItem::setPalette(const NesPalette &palette) {
    this->palette = palette;
    rebuildImage();
    update();
}

void NesNametableItem::setSubpalette(const QVector<quint8> &subpalette) {
    if (subpalette.size() != 4) {
        return;
    }

    this->subpalette = subpalette;
    rebuildImage();
    update();
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

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const int sourceIndex = y * 8 + x;
            const int destinationIndex = (tileY * 8 + y) * Width + tileX * 8 + x;
            this->pixels[destinationIndex] = tilePixels.value(sourceIndex, 0);
			this->colorIndices[destinationIndex] = tileSubpalette.value(
					this->pixels[destinationIndex], tileSubpalette.value(0));
        }
    }

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const int index = (tileY * 8 + y) * Width + tileX * 8 + x;
            this->image.setPixelColor(tileX * 8 + x, tileY * 8 + y,
							this->palette.colorAt(this->colorIndices[index]));
        }
    }

    update(QRectF(tileX * 8, tileY * 8, 8, 8));
}

void NesNametableItem::rebuildImage() {
    for (int y = 0; y < Height; ++y) {
        for (int x = 0; x < Width; ++x) {
            const int index = y * Width + x;
            this->image.setPixelColor(x, y,
							this->palette.colorAt(this->colorIndices[index]));
        }
    }
}
