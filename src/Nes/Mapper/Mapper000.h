#pragma once

#include "Mapper.h"

class Ram;

class Mapper000 final : public Mapper {
public:
    Mapper000(Ram &prgRom, Ram &chrRom);

    bool readCpu(quint16 address, quint8 &value) override;
    bool writeCpu(quint16 address, quint8 value) override;
    bool readPpu(quint16 address, quint8 &value) override;
    bool writePpu(quint16 address, quint8 value) override;

private:
    Ram &prgRom;
    Ram &chrRom;
};
