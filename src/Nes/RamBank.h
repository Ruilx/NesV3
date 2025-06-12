#pragma once

#include <qglobal.h>

class Ram;
class RamBank {

public:
    enum BankType {
        // for PROM (CPU)
        BankTypeRom = 0x00,
        BankTypeRam = 0xFF,
        BankTypeDRam = 0x01,
        BankTypeMapper = 0x80,
        // for VROM/VRAM/CRAM (PPU)
        BankTypeVRom = 0x00,
        BankTypeCRam = 0x01,
        BankTypeVRam = 0x80,
    };

protected:
    Ram *ram = nullptr;
    BankType type = BankTypeRom;
    quint8 page;
};

