#include "NoiseChannel.h"

constexpr quint8 NoiseChannel::LengthTable[32];
constexpr quint16 NoiseChannel::PeriodTable[16];

void NoiseChannel::reset() {
    this->control = 0;
    this->periodControl = 0;
    this->periodIndex = 0;
    this->lengthIndex = 0;
    this->timer = 0;
    this->length = 0;
    this->shift = 1;
    this->envelope = 0;
    this->envelopeDividerValue = 0;
    this->envelopeStart = false;
    this->enabled = false;
}

void NoiseChannel::clockCpuCycle() {
    if (this->timer == 0) {
        this->timer = this->timerPeriod();
        const quint16 tap = (this->periodControl & 0x80) != 0 ? 6 : 1;
        const quint16 feedback = static_cast<quint16>(
            (this->shift & 0x01) ^ ((this->shift >> tap) & 0x01));
        this->shift = static_cast<quint16>((this->shift >> 1) | (feedback << 14));
    } else {
        --this->timer;
    }
}

void NoiseChannel::onQuarterFrame() {
    this->clockEnvelope();
}

void NoiseChannel::onHalfFrame() {
    this->clockLengthCounter();
}

bool NoiseChannel::writeRegister(quint16 offset, quint8 value) {
    switch (offset) {
    case 0:
        this->control = value;
        return true;
    case 2:
        this->periodControl = value;
        this->periodIndex = static_cast<quint8>(value & 0x0F);
        return true;
    case 3:
        this->lengthIndex = static_cast<quint8>(value >> 3);
        this->envelopeStart = true;
        this->reloadLengthCounter();
        return true;
    default:
        return false;
    }
}

float NoiseChannel::sample() const {
    if (!this->isActive() || (this->shift & 0x01) != 0) {
        return 0.0F;
    }
    const quint8 volume = (this->control & 0x10) != 0
        ? static_cast<quint8>(this->control & 0x0F)
        : this->envelope;
    return static_cast<float>(volume) / 15.0F;
}

void NoiseChannel::setEnabled(bool value) {
    this->enabled = value;
    if (!value) {
        this->length = 0;
    }
}

bool NoiseChannel::isEnabled() const {
    return this->enabled;
}

bool NoiseChannel::isActive() const {
    return this->enabled && this->length != 0;
}

quint16 NoiseChannel::timerPeriod() const {
    return PeriodTable[this->periodIndex];
}

quint8 NoiseChannel::lengthCounter() const {
    return this->length;
}

quint8 NoiseChannel::envelopeVolume() const {
    return this->envelope;
}

quint8 NoiseChannel::envelopeDivider() const {
    return this->envelopeDividerValue;
}

quint16 NoiseChannel::shiftRegister() const {
    return this->shift;
}

bool NoiseChannel::mode() const {
    return (this->periodControl & 0x80) != 0;
}

void NoiseChannel::reloadLengthCounter() {
    if (this->enabled) {
        this->length = LengthTable[this->lengthIndex];
    }
}

void NoiseChannel::clockEnvelope() {
    const quint8 volume = this->control & 0x0F;
    if (this->envelopeStart) {
        this->envelopeStart = false;
        this->envelope = 15;
        this->envelopeDividerValue = volume;
        return;
    }
    if (this->envelopeDividerValue != 0) {
        --this->envelopeDividerValue;
    } else {
        this->envelopeDividerValue = volume;
        if (this->envelope != 0) {
            --this->envelope;
        } else if ((this->control & 0x20) != 0) {
            this->envelope = 15;
        }
    }
}

void NoiseChannel::clockLengthCounter() {
    if ((this->control & 0x20) == 0 && this->length > 0) {
        --this->length;
    }
}
