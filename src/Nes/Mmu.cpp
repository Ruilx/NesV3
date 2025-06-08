#include "Mmu.h"

void Mmu::setPRomBank(quint8 page, Ram *bank, Mmu::BankType type) {
    if (page >= CpuBankSize) {
        throw ValueError(QString("Invalid page number: %1").arg(page));
    }
    this->cpuBank[page].bank = bank;
    this->cpuBank[page].base = 0;
    this->cpuBank[page].type = type;
    this->cpuBank[page].page = 0;
}

void Mmu::setPRom8kBank(quint8 page, quint16 bankIndex) {
    bankIndex %= this->pRom8kSize;
    if (page >= CpuBankSize) {
        throw ValueError("Invalid page number");
    }
    this->cpuBank[page].bank = this->pRom;
    this->cpuBank[page].base = bankIndex * 0x2000;
    this->cpuBank[page].type = BankTypeRom;
    this->cpuBank[page].page = bankIndex;
}

void Mmu::setPRom16kBank(quint8 page, quint16 bankIndex) {
    this->setPRom8kBank(page, bankIndex << 1);
    this->setPRom8kBank(page + 1, bankIndex << 1 | 1);
}

void Mmu::setPRom32kBank(quint16 bankIndex) {
    this->setPRom16kBank(4, bankIndex << 2);
    this->setPRom16kBank(5, bankIndex << 2 | 1);
    this->setPRom16kBank(6, bankIndex << 2 | 2);
    this->setPRom16kBank(7, bankIndex << 2 | 3);
}

void Mmu::setPRom32kBank(quint16 bankIndex0, quint16 bankIndex1, quint16 bankIndex2, quint16 bankIndex3) {
    this->setPRom16kBank(4, bankIndex0);
    this->setPRom16kBank(5, bankIndex1);
    this->setPRom16kBank(6, bankIndex2);
    this->setPRom16kBank(7, bankIndex3);
}

void Mmu::setVRomBank(quint8 page, Ram *ram, Mmu::BankType type) {
    if (page >= PpuBankSize) {
        throw ValueError(QString("Invalid page number: %1").arg(page));
    }
    this->ppuBank[page].bank = ram;
    this->ppuBank[page].type = type;
    this->ppuBank[page].page = 0;
}

void Mmu::setVRom1kBank(quint8 page, quint16 bankIndex) {
    bankIndex %= this->vRom1kSize;
    this->ppuBank[page].bank = this->vRom;
    this->ppuBank[page].base = 0x0400 * bankIndex;
    this->ppuBank[page].type = BankTypeVRom;
    this->ppuBank[page].page = bankIndex;
}

void Mmu::setVRom2kBank(quint8 page, quint16 bankIndex) {
    this->setVRom1kBank(page, bankIndex << 1);
    this->setVRom1kBank(page + 1, bankIndex << 1 | 1);
}

void Mmu::setVRom4kBank(quint8 page, quint16 bankIndex) {
    this->setVRom1kBank(page, bankIndex << 2);
    this->setVRom1kBank(page + 1, bankIndex << 2 | 1);
    this->setVRom1kBank(page + 2, bankIndex << 2 | 2);
    this->setVRom1kBank(page + 3, bankIndex << 2 | 3);
}

void Mmu::setVRom8kBank(quint16 bankIndex) {
    this->setVRom4kBank(0, bankIndex << 3);
    this->setVRom4kBank(1, bankIndex << 3 | 1);
    this->setVRom4kBank(2, bankIndex << 3 | 2);
    this->setVRom4kBank(3, bankIndex << 3 | 3);
    this->setVRom4kBank(4, bankIndex << 3 | 4);
    this->setVRom4kBank(5, bankIndex << 3 | 5);
    this->setVRom4kBank(6, bankIndex << 3 | 6);
    this->setVRom4kBank(7, bankIndex << 3 | 7);
}

void
Mmu::setVRom8kBank(quint16 bankIndex0, quint16 bankIndex1, quint16 bankIndex2, quint16 bankIndex3, quint16 bankIndex4,
                   quint16 bankIndex5, quint16 bankIndex6, quint16 bankIndex7) {
    this->setVRom4kBank(0, bankIndex0);
    this->setVRom4kBank(1, bankIndex1);
    this->setVRom4kBank(2, bankIndex2);
    this->setVRom4kBank(3, bankIndex3);
    this->setVRom4kBank(4, bankIndex4);
    this->setVRom4kBank(5, bankIndex5);
    this->setVRom4kBank(6, bankIndex6);
    this->setVRom4kBank(7, bankIndex7);
}

void Mmu::setCRam1kBank(quint8 page, quint16 bankIndex) {
    bankIndex &= 0x1F;
    this->ppuBank[page].bank = this->cRam;
    this->ppuBank[page].base = 0x0400 * bankIndex;
    this->ppuBank[page].type = BankTypeCRam;
    this->ppuBank[page].page = bankIndex;
    this->cRamUsed[bankIndex >> 2] = true;
}

void Mmu::setCRam2kBank(quint8 page, quint16 bankIndex) {
    this->setCRam1kBank(page, bankIndex << 1);
    this->setCRam1kBank(page + 1, bankIndex << 1 | 1);
}

void Mmu::setCRam4kBank(quint8 page, quint16 bankIndex) {
    this->setCRam1kBank(page, bankIndex << 2);
    this->setCRam1kBank(page + 1, bankIndex << 2 | 1);
    this->setCRam1kBank(page + 2, bankIndex << 2 | 2);
    this->setCRam1kBank(page + 3, bankIndex << 2 | 3);
}

void Mmu::setCRam8kBank(quint16 bankIndex) {
    this->setCRam1kBank(0, bankIndex << 3);
    this->setCRam1kBank(1, bankIndex << 3 | 1);
    this->setCRam1kBank(2, bankIndex << 3 | 2);
    this->setCRam1kBank(3, bankIndex << 3 | 3);
    this->setCRam1kBank(4, bankIndex << 3 | 4);
    this->setCRam1kBank(5, bankIndex << 3 | 5);
    this->setCRam1kBank(6, bankIndex << 3 | 6);
    this->setCRam1kBank(7, bankIndex << 3 | 7);
}

void Mmu::setVRam1kBank(quint8 page, quint16 bankIndex) {
    bankIndex &= 0x03;
    this->ppuBank[page].bank = this->vRam;
    this->ppuBank[page].base = 0x0400 * bankIndex;
    this->ppuBank[page].type = BankTypeVRam;
    this->ppuBank[page].page = bankIndex;
}

void Mmu::setVRamBank(quint16 bankIndex0, quint16 bankIndex1, quint16 bankIndex2, quint16 bankIndex3) {
    this->setVRam1kBank(8, bankIndex0);
    this->setVRam1kBank(9, bankIndex1);
    this->setVRam1kBank(10, bankIndex2);
    this->setVRam1kBank(11, bankIndex3);
}

void Mmu::setVRamMirror(Mmu::VRamMirror type) {
    switch (type) {
        case VRamHMirror:
            this->setVRamBank(0, 0, 1, 1);
            break;
        case VRamVMirror:
            this->setVRamBank(0, 1, 0, 1);
            break;
        case VRamMirror4L:
            this->setVRamBank(0, 0, 0, 0);
            break;
        case VRamMirror4H:
            this->setVRamBank(1, 1, 1, 1);
            break;
        case VRamMirror4:
            this->setVRamBank(0, 1, 2, 3);
            break;
    }
}

void Mmu::setVRamMirror(quint16 bankIndex0, quint16 bankIndex1, quint16 bankIndex2, quint16 bankIndex3) {
    this->setVRam1kBank(8, bankIndex0);
    this->setVRam1kBank(9, bankIndex1);
    this->setVRam1kBank(10, bankIndex2);
    this->setVRam1kBank(11, bankIndex3);
}
