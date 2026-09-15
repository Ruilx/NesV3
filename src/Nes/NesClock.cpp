#include "NesClock.h"

NesClock::NesClock(TickCallback cpuTick, TickCallback ppuTick)
    : NesClock(std::move(cpuTick), std::move(ppuTick), TimingProfile()) {
}

NesClock::NesClock(
        TickCallback cpuTick,
        TickCallback ppuTick,
        TimingProfile timing)
        : cpuTickCallback(std::move(cpuTick)),
            ppuTickCallback(std::move(ppuTick)),
            timingProfile(timing) {
}

void NesClock::tick() {
        if (this->ppuTickCallback) {
                this->ppuTickCallback();
        }
    ++this->ppuTicksValue;

    ++this->cpuPhase;
    if (this->cpuPhase == this->timingProfile.ppuTicksPerCpuCycle) {
        this->cpuPhase = 0;
        if (this->cpuTickCallback) {
            this->cpuTickCallback();
        }
        ++this->cpuCyclesValue;
    }
}

void NesClock::runPpuTicks(quint64 ticks) {
    this->runMasterTicks(ticks);
}

void NesClock::runMasterTicks(quint64 ticks) {
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

void NesClock::runFrame() {
    this->runMasterTicks(this->timingProfile.ppuTicksPerFrame());
}

const NesClock::TimingProfile &NesClock::timing() const {
    return this->timingProfile;
}