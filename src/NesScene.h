#pragma once

#include <QGraphicsScene>
#include <QColor>
#include <QHash>
#include <QPointF>
#include <QVector>

#include "NesPalette.h"

class NesNametableItem;
class NesSpriteItem;
class NesFrameItem;
class Ppu;

class NesScene : public QGraphicsScene {
    Q_OBJECT

public:
    struct UpdateStats {
        quint64 dirtyTiles = 0;
        quint64 decodedTiles = 0;
        quint64 updatedTiles = 0;
        quint64 elapsedNanoseconds = 0;
    };
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
            void invalidateTileCache();
    void setPalette(const NesPalette &palette);
    [[nodiscard]] const NesPalette &palette() const;
    void scrollBy(qreal deltaX, qreal deltaY);
    void setScrollOffset(qreal x, qreal y);
    [[nodiscard]] QPointF scrollOffset() const;
    [[nodiscard]] QRectF nesViewportRect() const;
    [[nodiscard]] UpdateStats takeUpdateStats();

private:
    struct NametablePlacement {
        NesNametableItem *item = nullptr;
        int logicalNametable = 0;
        QPointF basePosition;
    };

    void createDemoNametables();
    void createFrameItem();
    void createViewportFrame();
    void createSpriteItems();
    void updateSpritesFromPpu(Ppu &ppu);
    void layoutNametableCopies();
    void layoutNametableCopies(qreal x, qreal y);
    [[nodiscard]] int tileCacheIndex(int nametable, int tileX, int tileY) const;

    struct TileState {
        QVector<quint8> pixels;
        QVector<quint8> subpalette;
        bool valid = false;
    };

    NesPalette nesPalette;
    QHash<quint64, QVector<NesNametableItem *>> tiles;
    QVector<TileState> tileStates;
    QVector<NametablePlacement> placements;
    NesFrameItem *frameItem = nullptr;
    QVector<NesSpriteItem *> spriteItems;
    QPointF scroll = QPointF(0, 0);
    UpdateStats updateStats;
};
