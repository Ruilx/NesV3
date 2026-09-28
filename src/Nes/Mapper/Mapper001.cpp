#include "Mapper001.h"

#include "../Ram.h"

Mapper001::Mapper001(Ram &prgRom, Ram &prgRam, Ram &chrRom, bool chrRam)
    : prgRom(prgRom),
      prgRam(prgRam),
      chrRom(chrRom),
      chrRamEnabled(chrRam) {
}

bool Mapper001::chrRam() const {
    return this->chrRamEnabled;
}

Mapper::Mirroring Mapper001::mirroring() const {
    switch (this->controlRegister & 0x03) {
    case 0:
        return Mirroring::SingleScreenA;
    case 1:
        return Mirroring::SingleScreenB;
    case 2:
        return Mirroring::Vertical;
    default:
        return Mirroring::Horizontal;
    }
}

bool Mapper001::readCpu(quint16 address, quint8 &value) {
    const qsizetype offset = this->prgOffset(address);
    if (offset < 0 || offset >= static_cast<qsizetype>(this->prgRom.getSize())) {
        return false;
    }
    value = this->prgRom.getU8(offset);
    return true;
}

bool Mapper001::writeCpu(quint16 address, quint8 value) {
    ++this->stats.serialWrites;
    if ((value & 0x80) != 0) {
        ++this->stats.resetWrites;
        this->shiftRegister = 0x10;
        this->controlRegister |= 0x0C;
        this->notifyMirroringChanged();
        return true;
    }

    const bool commit = (this->shiftRegister & 0x01) != 0;
    this->shiftRegister = static_cast<quint8>(
        (this->shiftRegister >> 1) | ((value & 0x01) << 4));
    if (commit) {
        this->commitRegister(address, this->shiftRegister & 0x1F);
        this->shiftRegister = 0x10;
    }
    return true;
}

Mapper001::Stats Mapper001::takeStats() {
    const Stats result = this->stats;
    this->stats = {};
    return result;
}

bool Mapper001::readPpu(quint16 address, quint8 &value) {
    const qsizetype offset = this->chrOffset(address);
    if (offset < 0 || offset >= static_cast<qsizetype>(this->chrRom.getSize())) {
        return false;
    }
    value = this->chrRom.getU8(offset);
    return true;
}

bool Mapper001::writePpu(quint16 address, quint8 value) {
    const qsizetype offset = this->chrOffset(address);
    if (!this->chrRamEnabled || offset < 0
        || offset >= static_cast<qsizetype>(this->chrRom.getSize())) {
        return false;
    }
    this->chrRom.setU8(offset, value);
    this->notifyMappingChanged();
    return true;
}

void Mapper001::commitRegister(quint16 address, quint8 value) {
    switch ((address >> 13) & 0x03) {
    case 0:
        this->controlRegister = value;
        this->notifyMirroringChanged();
        break;
    case 1:
		++this->stats.chrBankCommits;
        if (this->chrBank0 != value) {
            this->chrBank0 = value;
			++this->stats.effectiveChrChanges;
            this->notifyMappingChanged();
        }
        break;
    case 2:
		++this->stats.chrBankCommits;
        if (this->chrBank1 != value) {
            this->chrBank1 = value;
			++this->stats.effectiveChrChanges;
            this->notifyMappingChanged();
        }
        break;
    default:
        this->prgBank = value;
        break;
    }
}

qsizetype Mapper001::prgOffset(quint16 address) const {
    if (this->prgRom.getSize() == 0) {
        return -1;
    }
    const qsizetype size = static_cast<qsizetype>(this->prgRom.getSize());
    const qsizetype bank16Count = size / 0x4000;
    const qsizetype bank32Count = size / 0x8000;
    const quint8 mode = static_cast<quint8>((this->controlRegister >> 2) & 0x03);
    qsizetype offset = address;
    if (mode <= 1) {
        const qsizetype bank = bank32Count == 0
            ? 0
            : ((this->prgBank & 0x0E) >> 1) % bank32Count;
        offset = bank * 0x8000 + address;
    } else if (mode == 2) {
        offset = address < 0x4000
            ? address
            : static_cast<qsizetype>(this->prgBank % bank16Count) * 0x4000
                + (address - 0x4000);
    } else {
        offset = address < 0x4000
            ? static_cast<qsizetype>(this->prgBank % bank16Count) * 0x4000
                + address
            : (bank16Count - 1) * 0x4000 + (address - 0x4000);
    }
    return offset % size;
}

qsizetype Mapper001::chrOffset(quint16 address) const {
    if (address >= 0x2000 || this->chrRom.getSize() == 0) {
        return -1;
    }
    const qsizetype size = static_cast<qsizetype>(this->chrRom.getSize());
    const bool mode4k = (this->controlRegister & 0x10) != 0;
    const qsizetype bankSize = mode4k ? 0x1000 : 0x2000;
    const qsizetype bankCount = size / bankSize;
    if (bankCount == 0) {
        return -1;
    }
    const quint8 selectedBank = mode4k
        ? (address < 0x1000 ? this->chrBank0 : this->chrBank1)
        : static_cast<quint8>((this->chrBank0 & 0x1E) >> 1);
    const qsizetype bank = static_cast<qsizetype>(selectedBank) % bankCount;
    return (bank * bankSize + (address % bankSize)) % size;
}