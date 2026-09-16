#pragma once

#include <QGraphicsItem>
#include <QColor>
#include <QImage>
#include <QVector>

#include "NesPalette.h"

class NesTileItem : public QGraphicsItem {
public:
    struct PaintStats {
        quint64 paintCalls = 0;
        quint64 imageRebuilds = 0;
        quint64 updateRequests = 0;
    };
    static constexpr int TileSize = 8;

    explicit NesTileItem(QGraphicsItem *parent = nullptr);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setPixels(const QVector<quint8> &pixels);
        void setPaletteIndices(
            const QVector<quint8> &pixels,
            const QVector<quint8> &subpalette,
            const NesPalette &palette);
    void setPalette(const QVector<QColor> &palette);
    void setGridVisible(bool visible);
    static void resetPaintStats();
    [[nodiscard]] static PaintStats paintStats();

private:
    QVector<quint8> pixels = QVector<quint8>(TileSize * TileSize, 0);
    QVector<quint8> colorIndices = QVector<quint8>(TileSize * TileSize, 0);
    QVector<QColor> palette;
    QImage image;
    bool gridVisible = false;

    static PaintStats statistics;

    void rebuildImage();
};
