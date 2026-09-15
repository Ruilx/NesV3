#include "Mapper000.h"

#include "../Ram.h"

Mapper000::Mapper000(Ram &prgRom, Ram &chrRom, bool chrRam)
    : prgRom(prgRom),
      chrRom(chrRom),
    chrRamEnabled(chrRam) {
}

void Mapper000::setChrRam(bool enabled) {
    this->chrRamEnabled = enabled;
}

bool Mapper000::chrRam() const {
    return this->chrRamEnabled;
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

bool Mapper000::writePpu(quint16 address, quint8 value) {
    if (!this->chrRamEnabled || address >= this->chrRom.getSize()) {
        return false;
    }
    this->chrRom.setU8(address, value);
    return true;
}
