#include "ChrTileDecoder.h"

bool ChrTileDecoder::decodeTile(
        Bus &chrBus,
        quint16 tileIndex,
        PatternTable patternTable,
        QVector<quint8> &pixels,
        Flip flip) {
    if (tileIndex >= TileCount) {
        pixels.clear();
        return false;
    }

    const quint16 tileAddress = static_cast<quint16>(
            static_cast<quint16>(patternTable) + tileIndex * TileBytes);
    quint8 planeBytes[TileBytes] = {};
    for (quint16 byteIndex = 0; byteIndex < TileBytes; ++byteIndex) {
        quint8 value = chrBus.openBusValue();
        if (chrBus.read(static_cast<quint16>(tileAddress + byteIndex), value)
                != Bus::AccessResult::Handled) {
            pixels.clear();
            return false;
        }
        planeBytes[byteIndex] = value;
    }

    pixels.resize(TileSize * TileSize);
    const bool flipHorizontal = flip == Flip::Horizontal
            || flip == Flip::HorizontalAndVertical;
    const bool flipVertical = flip == Flip::Vertical
            || flip == Flip::HorizontalAndVertical;
    for (quint16 y = 0; y < TileSize; ++y) {
        const quint16 sourceY = flipVertical ? TileSize - 1 - y : y;
        const quint8 lowPlane = planeBytes[sourceY];
        const quint8 highPlane = planeBytes[sourceY + TileSize];
        for (quint16 x = 0; x < TileSize; ++x) {
            const quint16 sourceX = flipHorizontal ? TileSize - 1 - x : x;
            const quint8 bit = static_cast<quint8>(7 - sourceX);
            pixels[y * TileSize + x] = static_cast<quint8>(
                    ((highPlane >> bit) & 0x01) << 1
                    | ((lowPlane >> bit) & 0x01));
        }
    }
    return true;
}