#pragma once

#include <QGraphicsScene>
#include <QColor>
#include <QHash>
#include <QPointF>
#include <QVector>

#include "NesPalette.h"

class NesNametableItem;
class Ppu;

class NesScene : public QGraphicsScene {
    Q_OBJECT

public:
    static constexpr int TileSize = 8;
    static constexpr int TilesWide = 32;
    static constexpr int TilesHigh = 30;
    static constexpr int NametableWidth = TileSize * TilesWide;
    static constexpr int NametableHeight = TileSize * TilesHigh;
    static constexpr int NametableColumns = 2;
    static constexpr int NametableRows = 2;

    explicit NesScene(QObject *parent = nullptr);

    void updateTile(int nametable, int tileX, int tileY, const QVector<quint8> &pixels);
        void updateTile(
            int nametable,
            int tileX,
            int tileY,
            const QVector<quint8> &pixels,
            const QVector<quint8> &subpalette);
        void updateFromPpu(Ppu &ppu);
    void setPalette(const NesPalette &palette);
    [[nodiscard]] const NesPalette &palette() const;
    void scrollBy(qreal deltaX, qreal deltaY);
    void setScrollOffset(qreal x, qreal y);
    [[nodiscard]] QPointF scrollOffset() const;
    [[nodiscard]] QRectF nesViewportRect() const;

private:
    struct NametablePlacement {
        NesNametableItem *item = nullptr;
        int logicalNametable = 0;
        QPointF basePosition;
    };

    void createDemoNametables();
    void createViewportFrame();
    void layoutNametableCopies();

    NesPalette nesPalette;
    QHash<quint64, QVector<NesNametableItem *>> tiles;
    QVector<NametablePlacement> placements;
    QPointF scroll = QPointF(0, 0);
};
