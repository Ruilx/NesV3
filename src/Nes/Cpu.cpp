#include "Cpu.h"

Cpu::Cpu(Nes *nes, QObject *parent)
    : QObject(parent),
      cpuBus(0x10000),
    internalRam(0x0800),
    internalRamDevice(this->internalRam),
      nes(nes) {
    const Bus::Mapping internalRamMapping{
        .start = 0x0000,
        .end = 0x1FFF,
        .priority = 0,
        .flags = AccessFlags::Readable | AccessFlags::Writable,
        .name = QStringLiteral("Internal RAM"),
        .device = &this->internalRamDevice,
        .translate = [](quint16 address) {
            return static_cast<quint16>(address & 0x07FF);
        },
    };
    this->cpuBus.registerMapping(internalRamMapping);
}

quint8 Cpu::readRam8(quint16 addr) {
    if (addr < 0x2000) {
        quint8 value = this->cpuBus.openBusValue();
        this->cpuBus.read(addr, value);
        return value;
    } else if (addr < 0x8000) {
        // others
        //return this->nes->read(addr);
    } else {
        quint8 value = this->cpuBus.openBusValue();
        this->cpuBus.read(addr, value);
        return value;
    }
    return 0;
}

quint16 Cpu::readRam16(quint16 addr) {
    if (addr < 0x2000) {
        quint8 low = this->cpuBus.openBusValue();
        quint8 high = this->cpuBus.openBusValue();
        this->cpuBus.read(addr, low);
        this->cpuBus.read(static_cast<quint16>(addr + 1), high);
        return static_cast<quint16>(low | (high << 8));
    } else if (addr < 0x8000) {
        // others
        //return this->nes->read(addr);
        return 0;
    } else {
        quint8 low = this->cpuBus.openBusValue();
        quint8 high = this->cpuBus.openBusValue();
        this->cpuBus.read(addr, low);
        this->cpuBus.read(static_cast<quint16>(addr + 1), high);
        return static_cast<quint16>(low | (high << 8));
    }

}

void Cpu::writeRam(quint16 addr, quint8 value) {
    if (addr < 0x2000) {
        this->cpuBus.write(addr, value);
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
    this->reg.intPending = CpuInterrupt::None;

    this->totalCycles = 0;
    this->dmaCycles = 0;

    // stack quick access
    this->stack = &this->internalRam;
    this->stack_offset = 0x0100;

    // zero/negative flag
    this->znTable[0] = ZFlag;
    for (quint16 i = 1; i < 256; i++) {
        this->znTable[i] = (i & 0x80) ? NFlag : 0;
    }
}

quint8 Cpu::op8(quint16 addr) {
    quint8 value = this->cpuBus.openBusValue();
    this->cpuBus.read(addr, value);
    return value;
}

quint16 Cpu::op16(quint16 addr) {
    quint8 low = this->cpuBus.openBusValue();
    quint8 high = this->cpuBus.openBusValue();
    this->cpuBus.read(addr, low);
    this->cpuBus.read(static_cast<quint16>(addr + 1), high);
    return static_cast<quint16>(low | (high << 8));
}

quint8 Cpu::zeroPageRead(quint8 addr) {
    quint8 value = this->cpuBus.openBusValue();
    this->cpuBus.read(addr, value);
    return value;
}


