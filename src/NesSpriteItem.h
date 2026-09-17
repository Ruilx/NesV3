#pragma once

#include <QGraphicsItem>
#include <QImage>
#include <QVector>

#include "NesPalette.h"

class NesSpriteItem final : public QGraphicsItem {
public:
    explicit NesSpriteItem(QGraphicsItem *parent = nullptr);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
        QWidget *widget) override;

    void setSprite(
        int width,
        int height,
        const QVector<quint8> &pixels,
        const QVector<quint8> &paletteIndices,
        const NesPalette &palette);
    void setVisibleSprite(bool visible);

private:
    int width = 0;
    int height = 0;
    QImage image;
    bool visibleSprite = false;
};