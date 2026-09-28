#pragma once

/**
 * Mapper003 (CNROM)
 * CNROM is a discrete-logic circult board providing up to four 8KB banks of CHR-ROM.
 * Two iNES mapper numbers denote its two configs:
 * * iNES Mapper3 decotes the common usage mounting 16-32 KiB of CHR-ROM.
 *   * submapper 0: Bus conflict behavior unknown
 *   * submapper 1: No bus conflicts
 *   * submapper 2: AND-type bus conflicts
 * * iNES Mapper 185 denotes a special usage mounting only 8KiB of CHR-ROM.
 * CHR-ROM is disabled unless the correct bank number has been selected.
 *   * submapper 0: CHR-ROM-enabling CS1/CS2 values unknown
 *   * submapper 4: CS1/CS2 = 0 enables CHR-ROM, or disabled
 *   * submapper 5: CS1/CS2 = 1 enables CHR-ROM, or disabled
 *   * submapper 6: CS1/CS2 = 2 enables CHR-ROM, or disabled
 *   * submapper 7: CS1/CS2 = 3 enables CHR-ROM, or disabled
 * 
 * Banks:
 * CPU $6000-$7FFF: PRG-ROM 2 KiB, mirrored three times (Hayauchi Super Igo only)
 * CPU $8000-$FFFF: 32KB unbanked PRG-ROM
 * PPU $0000-$1FFF: 8KB switchable window into 32KiB CHR-ROM
 * Nametable mirroring is cartridge metadata handled by the PPU nametable mapping.
 */

#include "Mapper.h"

class Ram;

class Mapper003 final : public Mapper {
public:
    Mapper003(Ram &prgRom, Ram &chrRom, bool chrRam = false);

    [[nodiscard]] bool chrRam() const override;

    bool readCpu(quint16 address, quint8 &value) override;
    bool writeCpu(quint16 address, quint8 value) override;
    bool readPpu(quint16 address, quint8 &value) override;
    bool writePpu(quint16 address, quint8 value) override;

private:
    Ram &prgRom;
    Ram &chrRom;
    bool chrRamEnabled = false;
    quint8 chrBank = 0;
};