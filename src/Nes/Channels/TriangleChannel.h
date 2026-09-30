#pragma once

#include "ApuChannel.h"

class TriangleChannel final : public ApuChannel {
public:
    TriangleChannel() = default;

    void reset() override;
    void clockCpuCycle() override;
    void onQuarterFrame() override;
    void onHalfFrame() override;
    bool writeRegister(quint16 offset, quint8 value) override;
    [[nodiscard]] float sample() const override;
    void setEnabled(bool enabled) override;
    [[nodiscard]] bool isEnabled() const override;
    [[nodiscard]] bool isActive() const override;

    [[nodiscard]] quint16 timerPeriod() const;
    [[nodiscard]] quint8 lengthCounter() const;
    [[nodiscard]] quint8 linearCounter() const;
    [[nodiscard]] quint8 sequenceIndex() const;

private:
    static constexpr quint8 LengthTable[32] = {
        10, 254, 20, 2, 40, 4, 80, 6,
        160, 8, 60, 10, 14, 12, 26, 14,
        12, 16, 24, 18, 48, 20, 96, 22,
        192, 24, 72, 26, 16, 28, 32, 30,
    };
    static constexpr quint8 Sequence[32] = {
        15, 14, 13, 12, 11, 10, 9, 8,
        7, 6, 5, 4, 3, 2, 1, 0,
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
    };

    quint8 control = 0;
    quint8 timerLow = 0;
    quint8 timerHigh = 0;
    quint8 lengthIndex = 0;
    quint16 timer = 0;
    quint8 sequencePosition = 0;
    quint8 length = 0;
    quint8 linear = 0;
    quint8 linearReloadValue = 0;
    bool linearReload = false;
    bool enabled = false;

    void reloadLengthCounter();
    void clockLinearCounter();
    void clockLengthCounter();
};
