#pragma once

#include "Bus.h"

#include <functional>

class Apu final : public BusDevice {
public:
    using IrqCallback = std::function<void(bool)>;

    struct Status {
        quint8 channelEnable = 0;
        bool frameIrq = false;
        bool dpcmIrq = false;
    };

    struct Stats {
        quint64 clockCalls = 0;
        quint64 reads = 0;
        quint64 writes = 0;
        quint64 irqLineChanges = 0;
    };

    Apu();

    bool read(quint16 address, quint8 &value) override;
    bool write(quint16 address, quint8 value) override;

    void clock();
    void reset();
    void setIrqCallback(IrqCallback callback);
    [[nodiscard]] Status status() const;
    [[nodiscard]] quint8 registerValue(quint8 index) const;
    [[nodiscard]] Stats takeStats();

private:
    static constexpr quint64 FrameCounterPeriod = 14915;

    quint8 registers[0x16] = {};
    quint8 channelEnable = 0;
    bool frameIrq = false;
    bool dpcmIrq = false;
    bool frameCounterMode5 = false;
    bool frameIrqInhibit = false;
    quint64 frameCounterCycles = 0;
    IrqCallback irqCallback;
    Stats statistics;
    bool irqLinePending = false;

    void writeFrameCounter(quint8 value);
    void updateIrqLine();
};
