#pragma once

#include <QGraphicsItem>
#include <QImage>
#include <QVector>

#include "NesPalette.h"

class NesFrameItem final : public QGraphicsItem {
public:
    explicit NesFrameItem(QGraphicsItem *parent = nullptr);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
        QWidget *widget) override;

    void setPaletteIndices(const QVector<quint8> &paletteIndices,
        const NesPalette &palette);
    void setPalette(const NesPalette &palette);

private:
    static constexpr int Width = 256;
    static constexpr int Height = 240;

    QImage image;
    QVector<quint8> paletteIndices;
    NesPalette palette;
};
