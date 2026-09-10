#pragma once

#include <QtGlobal>

#include <functional>

class NesClock {
public:
    static constexpr quint32 PpuTicksPerCpuCycle = 3;
    using TickCallback = std::function<void()>;

    NesClock(TickCallback cpuTick, TickCallback ppuTick);

    void tick();
    void runPpuTicks(quint64 ticks);
    void reset();

    [[nodiscard]] quint64 ppuTicks() const;
    [[nodiscard]] quint64 cpuCycles() const;

private:
    TickCallback cpuTickCallback;
    TickCallback ppuTickCallback;
    quint32 cpuPhase = 0;
    quint64 ppuTicksValue = 0;
    quint64 cpuCyclesValue = 0;
};