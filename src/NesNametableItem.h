#pragma once

#include <QColor>
#include <QGraphicsItem>
#include <QVector>
#include <QPointF>

#include "NesPalette.h"

class NesNametableItem : public QGraphicsItem {
public:
    struct PositionStats {
        quint64 rasterPositionChecks = 0;
        quint64 zeroDeltaPositionChecks = 0;
        quint64 smallDeltaPositionChecks = 0;
        quint64 tileSetPosCalls = 0;
        quint64 tilePositionChanges = 0;
    };
    static constexpr int Width = 256;
    static constexpr int Height = 240;

    explicit NesNametableItem(QGraphicsItem *parent = nullptr);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setPalette(const NesPalette &palette);
    void setSubpalette(const QVector<quint8> &subpalette);
    void setRasterScroll(
        const QVector<QPointF> &scrollByScanline,
        const QPointF &basePosition,
        const QPointF &projectedPosition);
    void setTilePixels(int tileX, int tileY, const QVector<quint8> &pixels);
    static void resetPositionStats();
    [[nodiscard]] static PositionStats positionStats();
        void setTilePixels(
            int tileX,
            int tileY,
            const QVector<quint8> &pixels,
            const QVector<quint8> &subpalette);

private:
    static constexpr int TileCount = (Width / 8) * (Height / 8);

    QVector<class NesTileItem *> tiles;
    QVector<QPointF> projectedTilePositions;
    NesPalette palette;
    QVector<quint8> subpalette = {0x0F, 0x01, 0x21, 0x31};
    static PositionStats statistics;
};
