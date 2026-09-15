#pragma once

#include <QtGlobal>

#include <functional>

class NesClock {
public:
    enum class Region : quint8 {
        Ntsc,
    };

    struct TimingProfile {
        Region region = Region::Ntsc;
        quint32 ppuTicksPerCpuCycle = 3;
        quint32 dotsPerScanline = 341;
        quint32 scanlinesPerFrame = 262;

        [[nodiscard]] constexpr quint64 ppuTicksPerFrame() const {
            return static_cast<quint64>(dotsPerScanline) * scanlinesPerFrame;
        }
    };

    using TickCallback = std::function<void()>;

    NesClock(TickCallback cpuTick, TickCallback ppuTick);
    NesClock(TickCallback cpuTick, TickCallback ppuTick, TimingProfile timing);

    void tick();
    void runMasterTicks(quint64 ticks);
    void runPpuTicks(quint64 ticks);
    void runFrame();
    void reset();

    [[nodiscard]] const TimingProfile &timing() const;
    [[nodiscard]] quint64 ppuTicks() const;
    [[nodiscard]] quint64 cpuCycles() const;

private:
    TickCallback cpuTickCallback;
    TickCallback ppuTickCallback;
    TimingProfile timingProfile;
    quint32 cpuPhase = 0;
    quint64 ppuTicksValue = 0;
    quint64 cpuCyclesValue = 0;
};