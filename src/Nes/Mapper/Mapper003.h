#pragma once

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