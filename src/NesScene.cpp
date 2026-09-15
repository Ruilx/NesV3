#include "NesScene.h"

#include "NesNametableItem.h"
#include "NesPalette.h"
#include "Ppu.h"

#include <QGraphicsRectItem>
#include <QPen>

#include <cmath>

namespace {
quint64 tileKey(int nametable, int tileX, int tileY) {
    return (static_cast<quint64>(nametable) << 32)
           | (static_cast<quint64>(tileY) << 16)
           | static_cast<quint64>(tileX);
}

qreal positiveModulo(qreal value, qreal period) {
    const qreal result = std::fmod(value, period);
    return result < 0 ? result + period : result;
}

qreal wrappedItemPosition(qreal basePosition, qreal scrollOffset, qreal period, qreal viewportSize) {
    qreal position = basePosition - scrollOffset;
    if (position < -viewportSize) {
        position += period;
    } else if (position > viewportSize) {
        position -= period;
    }
    return position;
}

QVector<quint8> demoTile(int nametable, int tileX, int tileY) {
    QVector<quint8> pixels(NesScene::TileSize * NesScene::TileSize, 0);
    const int pattern = (tileX / 4 + tileY / 3 + nametable) % 4;

    for (int y = 0; y < NesScene::TileSize; ++y) {
        for (int x = 0; x < NesScene::TileSize; ++x) {
            const bool border = x == 0 || y == 0 || x == 7 || y == 7;
            const bool diagonal = ((x + y + tileX + tileY) % 7) == 0;
            const bool nametableMarker = tileX < 2 && tileY < 2;
            pixels[y * NesScene::TileSize + x] = nametableMarker
                ? static_cast<quint8>(1 + ((nametable >> (tileY * 2 + tileX)) & 1))
                : (border ? 3 : (diagonal ? 2 : pattern));
        }
    }

    return pixels;
}
}

NesScene::NesScene(QObject *parent) : QGraphicsScene(parent) {
    setSceneRect(0, 0, NametableWidth, NametableHeight);

    createDemoNametables();
    createViewportFrame();
}

void NesScene::updateTile(int nametable, int tileX, int tileY, const QVector<quint8> &pixels) {
	updateTile(nametable, tileX, tileY, pixels, QVector<quint8>());
}

void NesScene::updateTile(
    int nametable,
    int tileX,
    int tileY,
    const QVector<quint8> &pixels,
    const QVector<quint8> &subpalette) {
    if (nametable < 0 || nametable >= NametableColumns * NametableRows
        || tileX < 0 || tileX >= TilesWide || tileY < 0 || tileY >= TilesHigh) {
        return;
    }

    const auto iterator = this->tiles.constFind(tileKey(nametable, tileX, tileY));
    if (iterator == this->tiles.constEnd()) {
        return;
    }

    for (NesNametableItem *nametableItem : iterator.value()) {
        if (subpalette.size() == 4) {
            nametableItem->setTilePixels(tileX, tileY, pixels, subpalette);
        } else {
            nametableItem->setTilePixels(tileX, tileY, pixels);
        }
    }
}

void NesScene::updateFromPpu(Ppu &ppu) {
    for (int nametable = 0; nametable < NametableColumns * NametableRows; ++nametable) {
        for (int tileY = 0; tileY < TilesHigh; ++tileY) {
            for (int tileX = 0; tileX < TilesWide; ++tileX) {
                QVector<quint8> pixels;
                QVector<quint8> subpalette;
                if (ppu.renderNametableTile(
                            nametable, tileX, tileY, pixels, subpalette)) {
                    updateTile(nametable, tileX, tileY, pixels, subpalette);
                }
            }
        }
    }
}

void NesScene::setPalette(const NesPalette &palette) {
    this->nesPalette = palette;
    for (const NametablePlacement &placement : this->placements) {
        placement.item->setPalette(this->nesPalette);
    }
}

const NesPalette &NesScene::palette() const {
    return this->nesPalette;
}

void NesScene::scrollBy(qreal deltaX, qreal deltaY) {
    setScrollOffset(this->scroll.x() + deltaX, this->scroll.y() + deltaY);
}

void NesScene::setScrollOffset(qreal x, qreal y) {
    const QPointF nextScroll(
        positiveModulo(x, NametableWidth * NametableColumns),
        positiveModulo(y, NametableHeight * NametableRows));

    if (qFuzzyCompare(nextScroll.x(), this->scroll.x())
        && qFuzzyCompare(nextScroll.y(), this->scroll.y())) {
        return;
    }

    this->scroll = nextScroll;
    layoutNametableCopies();
}

QPointF NesScene::scrollOffset() const {
    return this->scroll;
}

QRectF NesScene::nesViewportRect() const {
    return QRectF(0, 0, NametableWidth, NametableHeight);
}

void NesScene::createDemoNametables() {
    for (int nametable = 0; nametable < NametableColumns * NametableRows; ++nametable) {
        const int originX = (nametable % NametableColumns) * NametableWidth;
        const int originY = (nametable / NametableColumns) * NametableHeight;

        auto *nametableItem = new NesNametableItem;
        nametableItem->setPos(originX, originY);
        nametableItem->setPalette(this->nesPalette);
        addItem(nametableItem);

        for (int tileY = 0; tileY < TilesHigh; ++tileY) {
            for (int tileX = 0; tileX < TilesWide; ++tileX) {
                const quint64 key = tileKey(nametable, tileX, tileY);
                this->tiles[key].append(nametableItem);
                nametableItem->setTilePixels(tileX, tileY, demoTile(nametable, tileX, tileY));
            }
        }

        this->placements.append({nametableItem, nametable, QPointF(originX, originY)});
    }

    layoutNametableCopies();
}

void NesScene::layoutNametableCopies() {
    for (const NametablePlacement &placement : this->placements) {
        const qreal x = wrappedItemPosition(
            placement.basePosition.x(), this->scroll.x(),
            NametableWidth * NametableColumns, NametableWidth);
        const qreal y = wrappedItemPosition(
            placement.basePosition.y(), this->scroll.y(),
            NametableHeight * NametableRows, NametableHeight);
        placement.item->setPos(x, y);
    }
}

void NesScene::createViewportFrame() {
    auto *frame = addRect(nesViewportRect(), QPen(QColor(255, 255, 255, 220), 1), Qt::NoBrush);
    frame->setZValue(1000.0);
    frame->setFlag(QGraphicsItem::ItemIgnoresTransformations, false);
}
