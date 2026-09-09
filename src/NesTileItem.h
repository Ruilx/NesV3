#pragma once

#include <QGraphicsItem>
#include <QColor>
#include <QVector>

class NesTileItem : public QGraphicsItem {
public:
    static constexpr int TileSize = 8;

    explicit NesTileItem(QGraphicsItem *parent = nullptr);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setPixels(const QVector<quint8> &pixels);
    void setPalette(const QVector<QColor> &palette);
    void setGridVisible(bool visible);

private:
    QVector<quint8> pixels = QVector<quint8>(TileSize * TileSize, 0);
    QVector<QColor> palette;
    bool gridVisible = false;
};
