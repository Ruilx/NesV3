#pragma once

#include <qglobal.h>

#include "Ram.h"
#include "../Excption/ValueError.h"

#define NesInternalRamSize (8 * 1024)
#define NesWorkRamSize (128 * 1024)
#define NesDiskSysRamSize (40 * 1024)
#define NesDummyRamSize (8 * 1024)
#define NesExternalDevRamSize (32 * 1024)
#define NesCharPatternRamSize (32 * 1024)
#define NesNamePatternRamSize (4 * 1024)
#define NesSplitRamSize (0x100)
#define NesBGPaletteRamSize (0x10)
#define NesSPPaletteRamSize (0x10)

#define NesCpuRegSize (0x18)
#define NesPpuRegSize (0x04)

#define CpuBankSize (8)
#define PpuBankSize (12)

class Mmu {

public:
    static Mmu &instance() {
        static Mmu instance;
        return instance;
    }

    // memory type
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

    // mirror type
    enum VRamMirror {
        VRamHMirror = 0x00, // horizontal
        VRamVMirror = 0x01, // vertical
        VRamMirror4 = 0x02, // all screen
        VRamMirror4L = 0x03, // PA10 L fixed mirror of $2000-$23FF
        VRamMirror4H = 0x04, // PA10 H fixed mirror of $2400-$27FF
    };

    // CPU memory bank
    typedef struct CpuBank_t {
        Ram *bank;     // Ram对象
        quint32 base;      // 偏移量
        BankType type;
        quint8 page;
    } CpuBank;

    // PPU memory bank
    typedef struct PpuBank_t {
        Ram *bank;
        quint32 base;
        BankType type;
        quint8 page;
    } PpuBank;

protected:
    CpuBank cpuBank[CpuBankSize]{};
    PpuBank ppuBank[PpuBankSize]{};

    bool cRamUsed[16] = {false}; // state save

    Ram *iRam = new Ram(NesInternalRamSize);    // Nes internal ram
    Ram *wRam = new Ram(NesWorkRamSize);        // Work ram
    Ram *dRam = new Ram(NesDiskSysRamSize);     // Disk system ram
    Ram *xRam = new Ram(NesDummyRamSize);       // Dummy ram
    Ram *eRam = new Ram(NesExternalDevRamSize); // External device ram
    Ram *cRam = new Ram(NesCharPatternRamSize); // Character / pattern ram
    Ram *vRam = new Ram(NesNamePatternRamSize); // Name pattern ram
    Ram *spRam = new Ram(NesSplitRamSize);      // Split ram

    Ram *bgPal = new Ram(NesBGPaletteRamSize);  // Background palette ram
    Ram *spPal = new Ram(NesSPPaletteRamSize);  // Sprite palette ram

    Ram *cpuReg = new Ram(NesCpuRegSize);      // Nes $4000-$4017
    Ram *ppuReg = new Ram(NesPpuRegSize);      // Nes $2000-$2003

    quint8 frameIrq = 0xC0;

    // ROM data pointer
    Ram *pRom = nullptr;
    Ram *vRom = nullptr;

    // ROM bank size
    // assign 1 to avoid division by zero
    // TODO: get the actual size from the ROM file
    size_t pRom8kSize = 1;
    size_t pRom16kSize = 1;
    size_t pRom32kSize = 1;
    size_t vRom1kSize = 1;
    size_t vRom2kSize = 1;
    size_t vRom4kSize = 1;
    size_t vRom8kSize = 1;

    void init_ram() {
        // default bank setting
        for (CpuBank &i: cpuBank) {
            i.bank = nullptr;
            i.base = 0;
            i.type = BankTypeRom;
            i.page = 0;
        }

        // internal RAM / WRAM
        this->setPRomBank(0, this->iRam, BankTypeRam);
        this->setPRomBank(3, this->wRam, BankTypeRam);
        // dummy
        this->setPRomBank(1, this->xRam, BankTypeRom);
        this->setPRomBank(2, this->xRam, BankTypeRom);

        for (quint8 i = 0; i < 8; i++) {
            this->cRamUsed[i] = false;
        }
    }

    explicit Mmu() {
        this->init_ram();
    }

public:


    void setPRomBank(quint8 page, Ram *bank, BankType type);

    void setPRom8kBank(quint8 page, quint16 bankIndex);

    void setPRom16kBank(quint8 page, quint16 bankIndex);

    void setPRom32kBank(quint16 bankIndex);

    void setPRom32kBank(quint16 bankIndex0, quint16 bankIndex1, quint16 bankIndex2, quint16 bankIndex3);

    void setVRomBank(quint8 page, Ram *ram, BankType type);

    void setVRom1kBank(quint8 page, quint16 bankIndex);

    void setVRom2kBank(quint8 page, quint16 bankIndex);

    void setVRom4kBank(quint8 page, quint16 bankIndex);

    void setVRom8kBank(quint16 bankIndex);

    void setVRom8kBank(quint16 bankIndex0, quint16 bankIndex1, quint16 bankIndex2, quint16 bankIndex3, quint16 bankIndex4,
                       quint16 bankIndex5, quint16 bankIndex6, quint16 bankIndex7);

    void setCRam1kBank(quint8 page, quint16 bankIndex);

    void setCRam2kBank(quint8 page, quint16 bankIndex);

    void setCRam4kBank(quint8 page, quint16 bankIndex);

    void setCRam8kBank(quint16 bankIndex);

    void setVRam1kBank(quint8 page, quint16 bankIndex);

    void setVRamBank(quint16 bankIndex0, quint16 bankIndex1, quint16 bankIndex2, quint16 bankIndex3);

    void setVRamMirror(VRamMirror type);

    void setVRamMirror(quint16 bankIndex0, quint16 bankIndex1, quint16 bankIndex2, quint16 bankIndex3);

public:
    [[nodiscard]] inline CpuBank getCpuBank(quint16 bankIndex) const {
        if (bankIndex >= CpuBankSize) {
            throw ValueError(QString::asprintf("Invalid bank index: %d", bankIndex));
        }
        return this->cpuBank[bankIndex];
    }

    [[nodiscard]] inline PpuBank getPpuBank(quint16 bankIndex) const {
        if (bankIndex >= PpuBankSize) {
            throw ValueError(QString::asprintf("Invalid bank index: %d", bankIndex));
        }
        return this->ppuBank[bankIndex];
    }

    [[nodiscard]] inline Ram *getIRam() const {
        return this->iRam;
    }

    [[nodiscard]] inline Ram *getWRam() const {
        return this->wRam;
    }

    [[nodiscard]] inline Ram *getDRam() const {
        return this->dRam;
    }

    [[nodiscard]] inline Ram *getXRam() const {
        return this->xRam;
    }

    [[nodiscard]] inline Ram *getERam() const {
        return this->eRam;
    }

    [[nodiscard]] inline Ram *getCRam() const {
        return this->cRam;
    }

    [[nodiscard]] inline Ram *getVRam() const {
        return this->vRam;
    }
};

#ifndef mmu
#   define mmu (Mmu::instance())
#endif