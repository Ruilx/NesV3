#pragma once

#include "ApuChannel.h"

class NoiseChannel final : public ApuChannel {
public:
    NoiseChannel() = default;

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
    [[nodiscard]] quint8 envelopeVolume() const;
    [[nodiscard]] quint8 envelopeDivider() const;
    [[nodiscard]] quint16 shiftRegister() const;
    [[nodiscard]] bool mode() const;

private:
    static constexpr quint8 LengthTable[32] = {
        10, 254, 20, 2, 40, 4, 80, 6,
        160, 8, 60, 10, 14, 12, 26, 14,
        12, 16, 24, 18, 48, 20, 96, 22,
        192, 24, 72, 26, 16, 28, 32, 30,
    };
    static constexpr quint16 PeriodTable[16] = {
        4, 8, 16, 32, 64, 96, 128, 160,
        202, 254, 380, 508, 762, 1016, 2034, 4068,
    };

    quint8 control = 0;
    quint8 periodControl = 0;
    quint8 periodIndex = 0;
    quint8 lengthIndex = 0;
    quint16 timer = 0;
    quint8 length = 0;
    quint16 shift = 1;
    quint8 envelope = 0;
    quint8 envelopeDividerValue = 0;
    bool envelopeStart = false;
    bool enabled = false;

    void reloadLengthCounter();
    void clockEnvelope();
    void clockLengthCounter();
};
