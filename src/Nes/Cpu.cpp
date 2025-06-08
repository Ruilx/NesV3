#include "Cpu.h"

#include "Mmu.h"

quint8 Cpu::readRam8(quint16 addr) {
    if (addr < 0x2000) {
        // RAM (mirror $0800, $1000, $1800)
        return mmu.getIRam()->getU8(addr & 0x07FF);
    } else if (addr < 0x8000) {
        // others
        //return this->nes->read(addr);
    } else {
        // dummy access
        //this->mapper->read(addr, mmu.getCpuBank(addr >> 13).bank->getU8(addr & 0x1FFF);
    }
    return mmu.getCpuBank(addr >> 13).bank->getU8(addr & 0x1FFF);
}


