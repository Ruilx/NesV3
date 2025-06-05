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
    static Mmu *_instance;

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

    // CPU memory bank
    typedef struct CpuBank_t {
        Ram *bank;
        BankType type;
        quint8 page;
    } CpuBank;

    // PPU memory bank
    typedef struct PpuBank_t {
        Ram *bank;
        BankType type;
        quint8 page;
    } PpuBank;

    CpuBank cpuBank[CpuBankSize];
    PpuBank ppuBank[PpuBankSize];

    quint8 cRamUsed[16]; // state save

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
    quint8 *pRom = nullptr;
    quint8 *vRom = nullptr;

    // ROM bank size
    size_t pRom8kSize = 1;
    size_t pRom16kSize = 1;
    size_t pRom32kSize = 1;
    size_t vRom1kSize = 1;
    size_t vRom2kSize = 1;
    size_t vRom4kSize = 1;
    size_t vRom8kSize = 1;

    void init_ram(){
        // default bank setting
        for(CpuBank &i : cpuBank){
            i.bank = nullptr;
            i.type = BankTypeRom;
            i.page = 0;
        }

        // internal RAM / WRAM

    }

public:
    explicit Mmu(){

    }

    void setPRomBank(quint8 page, Ram *bank, BankType type){
        if(page >= CpuBankSize){
            throw ValueError("Invalid page number");
        }
        this->cpuBank[page].bank = bank;
        this->cpuBank[page].type = type;
        this->cpuBank[page].page = 0;
    }

};