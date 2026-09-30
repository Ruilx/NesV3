#pragma once

#include "ApuChannel.h"

class PulseChannel final : public ApuChannel {
public:
    explicit PulseChannel(bool channel1 = false);

    void reset() override;
    void clockCpuCycle() override;
    void onQuarterFrame() override;
    void onHalfFrame() override;
    bool writeRegister(quint16 offset, quint8 value) override;
    void contributeAudio(AudioMixContext &context) const override;
    [[nodiscard]] float sample() const override;
    void setEnabled(bool enabled) override;
    [[nodiscard]] bool isEnabled() const override;
    [[nodiscard]] bool isActive() const override;

    [[nodiscard]] quint8 duty() const;
    [[nodiscard]] quint16 timerPeriod() const;
    [[nodiscard]] quint8 lengthCounter() const;
    [[nodiscard]] quint8 envelopeVolume() const;
    [[nodiscard]] quint16 sweepTargetPeriod() const;
    [[nodiscard]] quint8 sweepDivider() const;
    [[nodiscard]] bool sweepMutes() const;

private:
    static constexpr quint8 LengthTable[32] = {
        10, 254, 20,  2, 40,  4, 80,  6,
        160, 8, 60, 10, 14, 12, 26, 14,
        12, 16, 24, 18, 48, 20, 96, 22,
        192, 24, 72, 26, 16, 28, 32, 30,
    };
    static constexpr quint8 DutySequences[4][8] = {
        {0, 1, 0, 0, 0, 0, 0, 0},
        {0, 1, 1, 0, 0, 0, 0, 0},
        {0, 1, 1, 1, 1, 0, 0, 0},
        {1, 0, 0, 1, 1, 1, 1, 1},
    };

    quint8 control = 0;
    quint8 sweep = 0;
    quint8 timerLow = 0;
    quint8 timerHigh = 0;
    quint8 lengthIndex = 0;
    quint8 sweepDividerValue = 0;
    quint16 timer = 0;
    quint8 sequenceIndex = 0;
    quint8 length = 0;
    quint8 envelopeDivider = 0;
    quint8 envelopeDecay = 0;
    bool envelopeStart = false;
    bool sweepReload = false;
    bool timerPhase = false;
    bool enabled = false;
    bool channel1 = false;

    void reloadEnvelope();
    void reloadLengthCounter();
    void clockEnvelope();
    void clockLengthCounter();
    void clockSweep();
};
