#pragma once

#include "../ApuMixer.h"

#include <QtGlobal>

#include <functional>

class DpcmChannel final {
public:
    using MemoryReader = std::function<quint8(quint16)>;
    using IrqCallback = std::function<void(bool)>;

    DpcmChannel() = default;

    void reset();
    void clockCpuCycle();
    bool writeRegister(quint16 offset, quint8 value);
    void contributeAudio(AudioMixContext &context) const;
    void setEnabled(bool enabled);
    [[nodiscard]] bool isEnabled() const;
    [[nodiscard]] bool isActive() const;
    [[nodiscard]] float sample() const;
    [[nodiscard]] bool irqPending() const;
    [[nodiscard]] quint16 currentAddress() const;
    [[nodiscard]] quint16 bytesRemaining() const;
    [[nodiscard]] quint16 sampleLength() const;
    [[nodiscard]] quint16 timerPeriod() const;
    [[nodiscard]] quint8 outputLevel() const;

    void setMemoryReader(MemoryReader reader);
    void setIrqCallback(IrqCallback callback);
    void clearIrq();

private:
    static constexpr quint16 RateTable[16] = {
        428, 380, 340, 320, 286, 254, 226, 214,
        190, 160, 142, 128, 106, 84, 72, 54,
    };

    quint8 control = 0;
    quint8 output = 0;
    quint8 addressRegister = 0;
    quint8 lengthRegister = 0;
    quint16 timer = 0;
    quint16 currentAddressValue = 0;
    quint16 bytesRemainingValue = 0;
    quint16 sampleLengthValue = 0;
    quint8 sampleBuffer = 0;
    quint8 bitsRemaining = 0;
    bool sampleBufferEmpty = true;
    bool enabled = false;
    bool irq = false;
    MemoryReader memoryReader;
    IrqCallback irqCallback;

    void fetchSampleByte();
    void finishSample();
    void setIrqPending(bool pending);
};
