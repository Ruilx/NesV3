#include "NesClock.h"

#include <chrono>

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
    ++this->cpuPhase;
    if (this->cpuPhase == this->timingProfile.ppuTicksPerCpuCycle) {
        this->cpuPhase = 0;
        if (this->cpuTickCallback) {
            ++this->cpuCallbackCount;
            const bool sample = (this->cpuCallbackCount & 0xFF) == 0;
            const auto start = sample
                    ? std::chrono::steady_clock::now()
                    : std::chrono::steady_clock::time_point();
            this->cpuTickCallback();
            if (sample) {
                this->timingStats.cpuNanoseconds += static_cast<quint64>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::steady_clock::now() - start).count());
                ++this->timingStats.cpuSamples;
            }
        }
        ++this->cpuCyclesValue;
    }

    if (this->ppuTickCallback) {
        ++this->ppuCallbackCount;
        const bool sample = (this->ppuCallbackCount & 0xFF) == 0;
        const auto start = sample
            ? std::chrono::steady_clock::now()
            : std::chrono::steady_clock::time_point();
        this->ppuTickCallback();
        if (sample) {
            this->timingStats.ppuNanoseconds += static_cast<quint64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - start).count());
            ++this->timingStats.ppuSamples;
        }
    }
    ++this->ppuTicksValue;
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
    this->cpuCallbackCount = 0;
    this->ppuCallbackCount = 0;
    this->timingStats = TimingStats();
}

quint64 NesClock::ppuTicks() const {
    return this->ppuTicksValue;
}

quint64 NesClock::cpuCycles() const {
    return this->cpuCyclesValue;
}

NesClock::TimingStats NesClock::takeTimingStats() {
    const TimingStats result = this->timingStats;
    this->timingStats = TimingStats();
    return result;
}

void NesClock::runFrame() {
    this->runMasterTicks(this->timingProfile.ppuTicksPerFrame());
}

const NesClock::TimingProfile &NesClock::timing() const {
    return this->timingProfile;
}