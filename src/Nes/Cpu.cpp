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

quint16 Cpu::readRam16(quint16 addr) {
    if (addr < 0x2000) {
        // ram (Mirror $0800, $1000, $1800)
        return mmu.getIRam()->get16(addr & 0x07FF);
    } else if (addr < 0x8000) {
        // others
        //return this->nes->read(addr);
    } else {
        // quick bank read
        return mmu.getCpuBank(addr >> 13).bank->get16(addr & 0x1FFF);
    }

}

void Cpu::writeRam(quint16 addr, quint8 value) {
    if (addr < 0x2000) {
        // ram (mirror $0800, $1000, $1800)
        mmu.getIRam()->setU8(addr & 0x07FF, value);
    } else {
        // others
        // nes->write(addr, data);
    }
}

void Cpu::reset() {
    this->reg.a = 0x00;
    this->reg.x = 0x00;
    this->reg.y = 0x00;
    this->reg.s = 0xFF;
    this->reg.p = ZFlag | RFlag;
    this->reg.pc = this->readRam16(Cpu::ResVector);
    this->reg.intPending = 0;

    this->totalCycles = 0;
    this->dmaCycles = 0;

    // stack quick access
    this->stack = mmu.getIRam();
    this->stack_offset = 0x0100;

    // zero/negative flag
    this->znTable[0] = ZFlag;
    for (quint16 i = 1; i < 256; i++) {
        this->znTable[i] = (i & 0x80) ? NFlag : 0;
    }
}

quint8 Cpu::op8(quint16 addr) {
    return mmu.getCpuBank(addr >> 13).bank->getU8(addr & 0x1FFF);
}

quint16 Cpu::op16(quint16 addr) {
    return mmu.getCpuBank(addr >> 13).bank->getU16(addr & 0x1FFF);
}

quint8 Cpu::zeroPageRead(quint8 addr) const {
    return mmu.get
}


