#include "Apu.h"

#include "Diagnostics.h"

Apu::Apu() {
    this->reset();
}

bool Apu::read(quint16 address, quint8 &value) {
    if constexpr (NesDiagnostics::EnableApuStats) {
        ++this->statistics.reads;
    }
    if (address == 0x15) {
        value = static_cast<quint8>(this->channelEnable & 0x1F);
        if (this->frameIrq) {
            value |= 0x40;
        }
        if (this->dpcmIrq) {
            value |= 0x80;
        }
        this->frameIrq = false;
        this->updateIrqLine();
        return true;
    }

    return false;
}

bool Apu::write(quint16 address, quint8 value) {
    if constexpr (NesDiagnostics::EnableApuStats) {
        ++this->statistics.writes;
    }
    if (address <= 0x13) {
        this->registers[address] = value;
        return true;
    }
    if (address == 0x15) {
        this->registers[address] = value;
        this->channelEnable = static_cast<quint8>(value & 0x1F);
        this->dpcmIrq = false;
        this->updateIrqLine();
        return true;
    }
    if (address == 0x17) {
        this->writeFrameCounter(value);
        return true;
    }

    return false;
}

void Apu::clock() {
    if constexpr (NesDiagnostics::EnableApuStats) {
        ++this->statistics.clockCalls;
    }
    ++this->frameCounterCycles;
    if (this->frameCounterCycles < FrameCounterPeriod) {
        return;
    }

    this->frameCounterCycles = 0;
    if (!this->frameCounterMode5 && !this->frameIrqInhibit) {
        this->frameIrq = true;
        this->updateIrqLine();
    }
}

void Apu::reset() {
    for (quint8 &value : this->registers) {
        value = 0;
    }
    this->channelEnable = 0;
    this->frameIrq = false;
    this->dpcmIrq = false;
    this->frameCounterMode5 = false;
    this->frameIrqInhibit = false;
    this->frameCounterCycles = 0;
    this->statistics = Stats();
    this->irqLinePending = false;
    this->updateIrqLine();
}

void Apu::setIrqCallback(IrqCallback callback) {
    this->irqCallback = std::move(callback);
    this->updateIrqLine();
}

Apu::Status Apu::status() const {
    return Status{
        .channelEnable = this->channelEnable,
        .frameIrq = this->frameIrq,
        .dpcmIrq = this->dpcmIrq,
    };
}

quint8 Apu::registerValue(quint8 index) const {
    return index < 0x16 ? this->registers[index] : 0;
}

Apu::Stats Apu::takeStats() {
    const Stats result = this->statistics;
    this->statistics = Stats();
    return result;
}

void Apu::writeFrameCounter(quint8 value) {
    this->registers[0x15] = value;
    this->frameCounterMode5 = (value & 0x80) != 0;
    this->frameIrqInhibit = (value & 0x40) != 0;
    if (this->frameIrqInhibit) {
        this->frameIrq = false;
    }
    this->frameCounterCycles = 0;
    this->updateIrqLine();
}

void Apu::updateIrqLine() {
    const bool pending = this->frameIrq || this->dpcmIrq;
    if (pending != this->irqLinePending) {
        this->irqLinePending = pending;
        if constexpr (NesDiagnostics::EnableApuStats) {
            ++this->statistics.irqLineChanges;
        }
    }
    if (this->irqCallback) {
        this->irqCallback(pending);
    }
}
