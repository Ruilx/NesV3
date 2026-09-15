#pragma once

#include "Bus.h"

#include <QVector>

class ChrTileDecoder final {
public:
    enum class PatternTable : quint16 {
        Lower = 0x0000,
        Upper = 0x1000,
    };

    enum class Flip : quint8 {
        None,
        Horizontal,
        Vertical,
        HorizontalAndVertical,
    };

    static constexpr quint16 TileCount = 256;
    static constexpr quint16 TileSize = 8;
    static constexpr quint16 TileBytes = 16;

    [[nodiscard]] static bool decodeTile(
        Bus &chrBus,
        quint16 tileIndex,
        PatternTable patternTable,
        QVector<quint8> &pixels,
        Flip flip = Flip::None);
};