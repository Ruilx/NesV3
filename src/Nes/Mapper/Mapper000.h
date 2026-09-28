#pragma once

/**
 * Mapper000 (NROM)
 * The generic designation NROM refers to the Nintendo cartidge boards
 * NES-NROM-128, NES-NROM-256, their HVC counterparts, and clone boards.
 * The iNES format assigns mapper0 to NROM.
 * 
 * Banks:
 * CPU $6000-$7FFF: Unbanked PRG-RAM, mirrored as necessary to fill entire 8KiB Window.
 * CPU $8000-$BFFF: PRG-ROM first 16KiB data.
 * CPU $C000-$FFFF: PRG-ROM NROM-256: Last 16 KiB data; NROM-128 mirror of $8000-$BFFF.
 * PPU $0000-$1FFF: CHR-ROM 8KiB
 */

#include "Mapper.h"

class Ram;

class Mapper000 final : public Mapper {
public:
    Mapper000(Ram &prgRom, Ram &chrRom, bool chrRam = false);

    void setChrRam(bool enabled);
    [[nodiscard]] bool chrRam() const override;

    bool readCpu(quint16 address, quint8 &value) override;
    bool writeCpu(quint16 address, quint8 value) override;
    bool readPpu(quint16 address, quint8 &value) override;
    bool writePpu(quint16 address, quint8 value) override;

private:
    Ram &prgRom;
    Ram &chrRom;
    bool chrRamEnabled = false;
};
