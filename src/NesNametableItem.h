#pragma once

#include <QColor>
#include <QGraphicsItem>
#include <QImage>
#include <QVector>

#include "NesPalette.h"

class NesNametableItem : public QGraphicsItem {
public:
    static constexpr int Width = 256;
    static constexpr int Height = 240;

    explicit NesNametableItem(QGraphicsItem *parent = nullptr);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setPalette(const NesPalette &palette);
    void setSubpalette(const QVector<quint8> &subpalette);
    void setTilePixels(int tileX, int tileY, const QVector<quint8> &pixels);
        void setTilePixels(
            int tileX,
            int tileY,
            const QVector<quint8> &pixels,
            const QVector<quint8> &subpalette);

private:
    void rebuildImage();

    QVector<quint8> pixels = QVector<quint8>(Width * Height, 0);
    QVector<quint8> colorIndices = QVector<quint8>(Width * Height, 0);
    NesPalette palette;
    QVector<quint8> subpalette = {0x0F, 0x01, 0x21, 0x31};
    QImage image;
};
