#include "Mapper000.h"

#include "../Ram.h"

Mapper000::Mapper000(Ram &prgRom, Ram &chrRom)
    : prgRom(prgRom),
      chrRom(chrRom) {
}

bool Mapper000::readCpu(quint16 address, quint8 &value) {
    if (this->prgRom.getSize() == 0) {
        return false;
    }

    const qsizetype offset = static_cast<qsizetype>(address) % static_cast<qsizetype>(this->prgRom.getSize());
    value = this->prgRom.getU8(offset);
    return true;
}

bool Mapper000::writeCpu(quint16, quint8) {
    return false;
}

bool Mapper000::readPpu(quint16 address, quint8 &value) {
    if (address >= this->chrRom.getSize()) {
        return false;
    }

    value = this->chrRom.getU8(address);
    return true;
}

bool Mapper000::writePpu(quint16, quint8) {
    return false;
}
