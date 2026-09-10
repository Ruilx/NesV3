#include "NesClock.h"

NesClock::NesClock(TickCallback cpuTick, TickCallback ppuTick)
        : cpuTickCallback(std::move(cpuTick)),
            ppuTickCallback(std::move(ppuTick)) {
}

void NesClock::tick() {
        if (this->ppuTickCallback) {
                this->ppuTickCallback();
        }
    ++this->ppuTicksValue;

    ++this->cpuPhase;
    if (this->cpuPhase == PpuTicksPerCpuCycle) {
        this->cpuPhase = 0;
        if (this->cpuTickCallback) {
            this->cpuTickCallback();
        }
        ++this->cpuCyclesValue;
    }
}

void NesClock::runPpuTicks(quint64 ticks) {
    while (ticks > 0) {
        tick();
        --ticks;
    }
}

void NesClock::reset() {
    this->cpuPhase = 0;
    this->ppuTicksValue = 0;
    this->cpuCyclesValue = 0;
}

quint64 NesClock::ppuTicks() const {
    return this->ppuTicksValue;
}

quint64 NesClock::cpuCycles() const {
    return this->cpuCyclesValue;
}