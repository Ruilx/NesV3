#include "NesFrameItem.h"

#include <QPainter>

NesFrameItem::NesFrameItem(QGraphicsItem *parent)
    : QGraphicsItem(parent),
      image(Width, Height, QImage::Format_ARGB32) {
    this->image.fill(Qt::transparent);
}

QRectF NesFrameItem::boundingRect() const {
    return QRectF(0, 0, Width, Height);
}

void NesFrameItem::paint(QPainter *painter,
    const QStyleOptionGraphicsItem *, QWidget *) {
    painter->drawImage(QPointF(0, 0), this->image);
}

void NesFrameItem::setPaletteIndices(
    const QVector<quint8> &indices, const NesPalette &nextPalette) {
    if (indices.size() != Width * Height) {
        return;
    }

    this->paletteIndices = indices;
    this->palette = nextPalette;
    for (int y = 0; y < Height; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(this->image.scanLine(y));
        for (int x = 0; x < Width; ++x) {
            line[x] = this->palette.colorAt(
                this->paletteIndices[y * Width + x]).rgba();
        }
    }
    update();
}

void NesFrameItem::setPalette(const NesPalette &nextPalette) {
    this->palette = nextPalette;
    if (this->paletteIndices.size() != Width * Height) {
        return;
    }
    setPaletteIndices(this->paletteIndices, this->palette);
}
