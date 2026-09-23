#include "NesScene.h"

#include "NesNametableItem.h"
#include "NesPalette.h"
#include "NesFrameItem.h"
#include "NesSpriteItem.h"
#include "Ppu.h"

#include <QGraphicsRectItem>
#include <QPen>
#include <QElapsedTimer>

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
    while (position < -viewportSize) {
        position += period;
    }
    while (position > viewportSize) {
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
    this->tileStates.resize(NametableColumns * NametableRows * TilesWide * TilesHigh);

    createDemoNametables();
    createBackdropItem();
    createFrameItem();
    createSpriteItems();
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

    TileState &state = this->tileStates[tileCacheIndex(nametable, tileX, tileY)];
    state.pixels = pixels;
    state.subpalette = subpalette;
    state.valid = true;
}

void NesScene::updateFromPpu(Ppu &ppu) {
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    if (this->backdropItem != nullptr) {
        this->backdropItem->setBrush(
            this->nesPalette.colorAt(ppu.universalBackgroundColor()));
    }
    const QVector<Ppu::DirtyTile> dirtyTiles = ppu.takeDirtyTiles();
    const Ppu::ScrollSnapshot scrollSnapshot = ppu.scrollSnapshot();
    const QVector<Ppu::ScrollSnapshot> rasterScroll = ppu.rasterScroll();
    QVector<QPointF> scrollByScanline;
    scrollByScanline.reserve(rasterScroll.size());
    for (const Ppu::ScrollSnapshot &snapshot : rasterScroll) {
        scrollByScanline.append(QPointF(snapshot.x, snapshot.y));
    }
    if (!rasterScroll.isEmpty()) {
        layoutNametableCopies(rasterScroll.constFirst().x, rasterScroll.constFirst().y);
    } else {
        layoutNametableCopies(scrollSnapshot.x, scrollSnapshot.y);
    }
    for (const NametablePlacement &placement : this->placements) {
        placement.item->setRasterScroll(
            scrollByScanline,
            placement.basePosition,
            placement.projectedPosition);
    }
    updateSpritesFromPpu(ppu);
    this->updateStats.dirtyTiles += static_cast<quint64>(dirtyTiles.size());
    bool fullUpdate = false;
    for (const TileState &state : this->tileStates) {
        if (!state.valid) {
            fullUpdate = true;
            break;
        }
    }

    auto updateOneTile = [this, &ppu](int nametable, int tileX, int tileY) {
        QVector<quint8> pixels;
        QVector<quint8> subpalette;
        if (!ppu.renderNametableTile(
                nametable, tileX, tileY, pixels, subpalette)) {
            return;
        }
        ++this->updateStats.decodedTiles;

        TileState &state = this->tileStates[
            this->tileCacheIndex(nametable, tileX, tileY)];
        if (!state.valid || state.pixels != pixels
                || state.subpalette != subpalette) {
            this->updateTile(nametable, tileX, tileY, pixels, subpalette);
            ++this->updateStats.updatedTiles;
        }
    };

    if (fullUpdate) {
        for (int nametable = 0;
            nametable < NametableColumns * NametableRows; ++nametable) {
            for (int tileY = 0; tileY < TilesHigh; ++tileY) {
                for (int tileX = 0; tileX < TilesWide; ++tileX) {
                    updateOneTile(nametable, tileX, tileY);
                }
            }
        }
        this->updateStats.elapsedNanoseconds += static_cast<quint64>(
            elapsedTimer.nsecsElapsed());
        return;
    }

    for (const Ppu::DirtyTile &tile : dirtyTiles) {
        updateOneTile(tile.nametable, tile.tileX, tile.tileY);
    }
    this->updateStats.elapsedNanoseconds += static_cast<quint64>(
        elapsedTimer.nsecsElapsed());
}

NesScene::UpdateStats NesScene::takeUpdateStats() {
    const UpdateStats result = this->updateStats;
    this->updateStats = UpdateStats();
    return result;
}

void NesScene::invalidateTileCache() {
    for (TileState &state : this->tileStates) {
        state.valid = false;
        state.pixels.clear();
        state.subpalette.clear();
    }
}

void NesScene::setPalette(const NesPalette &palette) {
    this->nesPalette = palette;
    if (this->backdropItem != nullptr) {
        this->backdropItem->setBrush(this->nesPalette.colorAt(0));
    }
    this->frameItem->setPalette(palette);
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
        nametableItem->setZValue(100.0);
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

void NesScene::createBackdropItem() {
    this->backdropItem = addRect(
        nesViewportRect(), QPen(Qt::NoPen), QBrush(this->nesPalette.colorAt(0)));
    this->backdropItem->setZValue(0.0);
}

void NesScene::createFrameItem() {
    this->frameItem = new NesFrameItem;
    this->frameItem->setZValue(100.0);
    this->frameItem->setVisible(false);
    addItem(this->frameItem);
}

void NesScene::layoutNametableCopies() {
    layoutNametableCopies(this->scroll.x(), this->scroll.y());
}

void NesScene::layoutNametableCopies(qreal scrollX, qreal scrollY) {
    for (NametablePlacement &placement : this->placements) {
        const qreal positionX = wrappedItemPosition(
            placement.basePosition.x(), scrollX,
            NametableWidth * NametableColumns, NametableWidth);
        const qreal positionY = wrappedItemPosition(
            placement.basePosition.y(), scrollY,
            NametableHeight * NametableRows, NametableHeight);
        const QPointF projectedPosition(positionX, positionY);
        if (!placement.hasProjectedPosition
                || placement.projectedPosition != projectedPosition) {
            placement.item->setPos(projectedPosition);
            placement.projectedPosition = projectedPosition;
            placement.hasProjectedPosition = true;
        }
    }
}

int NesScene::tileCacheIndex(int nametable, int tileX, int tileY) const {
    return (nametable * TilesHigh + tileY) * TilesWide + tileX;
}

void NesScene::createViewportFrame() {
    auto *frame = addRect(nesViewportRect(), QPen(QColor(255, 255, 255, 220), 1), Qt::NoBrush);
    frame->setZValue(1000.0);
    frame->setFlag(QGraphicsItem::ItemIgnoresTransformations, false);
}

void NesScene::createSpriteItems() {
    this->spriteItems.reserve(64);
    for (int index = 0; index < 64; ++index) {
        auto *spriteItem = new NesSpriteItem;
        spriteItem->setZValue(110.0 - index * 0.01);
        addItem(spriteItem);
        this->spriteItems.append(spriteItem);
    }
}

void NesScene::updateSpritesFromPpu(Ppu &ppu) {
    const QVector<Ppu::SpriteOutput> sprites = ppu.renderSpritesForFrame();
    for (int index = 0; index < this->spriteItems.size(); ++index) {
        NesSpriteItem *item = this->spriteItems[index];
        if (index >= sprites.size()) {
            item->setVisibleSprite(false);
            continue;
        }

        const Ppu::SpriteOutput &sprite = sprites[index];
        const bool behindBackground = (sprite.entry.attributes & 0x20) != 0;
        const qreal baseZ = behindBackground ? 90.0 : 110.0;
        const qreal zValue = baseZ - index * 0.01;
        item->setZValue(zValue);
        item->setPos(sprite.screenX, sprite.screenY);
        item->setSprite(
            sprite.render.width,
            sprite.render.height,
            sprite.render.pixels,
            sprite.render.paletteIndices,
            this->nesPalette);
    }
}
