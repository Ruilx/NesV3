#include "DpcmChannel.h"

constexpr quint16 DpcmChannel::RateTable[16];

void DpcmChannel::reset() {
    this->control = 0;
    this->output = 0;
    this->addressRegister = 0;
    this->lengthRegister = 0;
    this->timer = 0;
    this->currentAddressValue = 0;
    this->bytesRemainingValue = 0;
    this->sampleLengthValue = 0;
    this->sampleBuffer = 0;
    this->bitsRemaining = 0;
    this->sampleBufferEmpty = true;
    this->enabled = false;
    this->setIrqPending(false);
}

void DpcmChannel::clockCpuCycle() {
    if (!this->enabled) {
        return;
    }
    if (this->timer != 0) {
        --this->timer;
        return;
    }

    this->timer = this->timerPeriod();
    if (this->bitsRemaining == 0) {
        if (this->sampleBufferEmpty) {
            this->fetchSampleByte();
        }
        if (this->bitsRemaining == 0) {
            return;
        }
    }

    if ((this->sampleBuffer & 0x01) != 0) {
        if (this->output <= 125) {
            this->output = static_cast<quint8>(this->output + 2);
        }
    } else if (this->output >= 2) {
        this->output = static_cast<quint8>(this->output - 2);
    }
    this->sampleBuffer >>= 1;
    --this->bitsRemaining;
    if (this->bitsRemaining == 0) {
        this->sampleBufferEmpty = true;
        if (this->bytesRemainingValue == 0) {
            this->finishSample();
        }
    }
}

bool DpcmChannel::writeRegister(quint16 offset, quint8 value) {
    switch (offset) {
    case 0:
        this->control = value;
        if ((value & 0x80) == 0) {
            this->setIrqPending(false);
        }
        return true;
    case 1:
        this->output = static_cast<quint8>(value & 0x7F);
        return true;
    case 2:
        this->addressRegister = value;
        return true;
    case 3:
        this->lengthRegister = value;
        return true;
    default:
        return false;
    }
}

void DpcmChannel::contributeAudio(AudioMixContext &context) const {
    context.add(AudioMixBus::NativeDpcm, this->sample());
}

void DpcmChannel::setEnabled(bool value) {
    if (this->enabled == value) {
        return;
    }
    this->enabled = value;
    if (!value) {
        this->bytesRemainingValue = 0;
        this->bitsRemaining = 0;
        this->sampleBufferEmpty = true;
        return;
    }
    if (this->bytesRemainingValue == 0) {
        this->currentAddressValue = static_cast<quint16>(
            0xC000 | (static_cast<quint16>(this->addressRegister) << 6));
        this->sampleLengthValue = static_cast<quint16>(
            (static_cast<quint16>(this->lengthRegister) << 4) | 1);
        this->bytesRemainingValue = this->sampleLengthValue;
        this->bitsRemaining = 0;
        this->sampleBufferEmpty = true;
    }
}

bool DpcmChannel::isEnabled() const {
    return this->enabled;
}

bool DpcmChannel::isActive() const {
    return this->enabled && this->bytesRemainingValue != 0;
}

float DpcmChannel::sample() const {
    return static_cast<float>(this->output) / 127.0F;
}

bool DpcmChannel::irqPending() const {
    return this->irq;
}

quint16 DpcmChannel::currentAddress() const {
    return this->currentAddressValue;
}

quint16 DpcmChannel::bytesRemaining() const {
    return this->bytesRemainingValue;
}

quint16 DpcmChannel::sampleLength() const {
    return this->sampleLengthValue;
}

quint16 DpcmChannel::timerPeriod() const {
    return RateTable[this->control & 0x0F];
}

quint8 DpcmChannel::outputLevel() const {
    return this->output;
}

void DpcmChannel::setMemoryReader(MemoryReader reader) {
    this->memoryReader = std::move(reader);
}

void DpcmChannel::setIrqCallback(IrqCallback callback) {
    this->irqCallback = std::move(callback);
    if (this->irqCallback) {
        this->irqCallback(this->irq);
    }
}

void DpcmChannel::clearIrq() {
    this->setIrqPending(false);
}

void DpcmChannel::fetchSampleByte() {
    if (this->bytesRemainingValue == 0) {
        return;
    }
    this->sampleBuffer = this->memoryReader
        ? this->memoryReader(this->currentAddressValue)
        : 0;
    this->sampleBufferEmpty = false;
    this->bitsRemaining = 8;
    ++this->currentAddressValue;
    if (this->currentAddressValue == 0) {
        this->currentAddressValue = 0x8000;
    }
    --this->bytesRemainingValue;
}

void DpcmChannel::finishSample() {
    if ((this->control & 0x40) != 0) {
        this->currentAddressValue = static_cast<quint16>(
            0xC000 | (static_cast<quint16>(this->addressRegister) << 6));
        this->bytesRemainingValue = this->sampleLengthValue;
        return;
    }
    if ((this->control & 0x80) != 0) {
        this->setIrqPending(true);
    }
}

void DpcmChannel::setIrqPending(bool pending) {
    if (this->irq == pending) {
        return;
    }
    this->irq = pending;
    if (this->irqCallback) {
        this->irqCallback(this->irq);
    }
}
