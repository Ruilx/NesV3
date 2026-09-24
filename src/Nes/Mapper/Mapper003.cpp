#include "Mapper003.h"

#include "../Ram.h"

Mapper003::Mapper003(Ram &prgRom, Ram &chrRom, bool chrRam)
    : prgRom(prgRom),
      chrRom(chrRom),
      chrRamEnabled(chrRam) {
}

bool Mapper003::chrRam() const {
    return this->chrRamEnabled;
}

bool Mapper003::readCpu(quint16 address, quint8 &value) {
    if (this->prgRom.getSize() == 0) {
        return false;
    }

    const qsizetype offset = static_cast<qsizetype>(address)
        % static_cast<qsizetype>(this->prgRom.getSize());
    value = this->prgRom.getU8(offset);
    return true;
}

bool Mapper003::writeCpu(quint16, quint8 value) {
    const size_t chrBankCount = this->chrRom.getSize() / 0x2000;
    if (chrBankCount == 0) {
        return false;
    }

    this->chrBank = static_cast<quint8>(value % chrBankCount);
    return true;
}

bool Mapper003::readPpu(quint16 address, quint8 &value) {
    const qsizetype offset = static_cast<qsizetype>(this->chrBank) * 0x2000
        + address;
    if (address >= 0x2000
        || offset >= static_cast<qsizetype>(this->chrRom.getSize())) {
        return false;
    }

    value = this->chrRom.getU8(offset);
    return true;
}

bool Mapper003::writePpu(quint16 address, quint8 value) {
    const qsizetype offset = static_cast<qsizetype>(this->chrBank) * 0x2000
        + address;
    if (!this->chrRamEnabled || address >= 0x2000
        || offset >= static_cast<qsizetype>(this->chrRom.getSize())) {
        return false;
    }

    this->chrRom.setU8(offset, value);
    return true;
}