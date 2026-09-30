#include "TriangleChannel.h"

constexpr quint8 TriangleChannel::LengthTable[32];
constexpr quint8 TriangleChannel::Sequence[32];

void TriangleChannel::reset() {
    this->control = 0;
    this->timerLow = 0;
    this->timerHigh = 0;
    this->lengthIndex = 0;
    this->timer = 0;
    this->sequencePosition = 0;
    this->length = 0;
    this->linear = 0;
    this->linearReloadValue = 0;
    this->linearReload = false;
    this->enabled = false;
}

void TriangleChannel::clockCpuCycle() {
    if (this->timer == 0) {
        this->timer = this->timerPeriod();
        if (this->isActive()) {
            this->sequencePosition = static_cast<quint8>(
                (this->sequencePosition + 1) & 0x1F);
        }
    } else {
        --this->timer;
    }
}

void TriangleChannel::onQuarterFrame() {
    this->clockLinearCounter();
}

void TriangleChannel::onHalfFrame() {
    this->clockLengthCounter();
}

bool TriangleChannel::writeRegister(quint16 offset, quint8 value) {
    switch (offset) {
    case 0:
        this->control = value;
        this->linearReloadValue = static_cast<quint8>(value & 0x7F);
        return true;
    case 2:
        this->timerLow = value;
        return true;
    case 3:
        this->lengthIndex = static_cast<quint8>(value >> 3);
        this->timerHigh = value & 0x07;
        this->sequencePosition = 0;
        this->linearReload = true;
        this->reloadLengthCounter();
        return true;
    default:
        return false;
    }
}

void TriangleChannel::contributeAudio(AudioMixContext &context) const {
    context.add(AudioMixBus::NativeTriangle, this->sample());
}

float TriangleChannel::sample() const {
    if (!this->isActive() || this->timerPeriod() < 2) {
        return 0.0F;
    }
    return static_cast<float>(Sequence[this->sequencePosition]) / 15.0F;
}

void TriangleChannel::setEnabled(bool value) {
    this->enabled = value;
    if (!value) {
        this->length = 0;
    }
}

bool TriangleChannel::isEnabled() const {
    return this->enabled;
}

bool TriangleChannel::isActive() const {
    return this->enabled && this->length != 0 && this->linear != 0;
}

quint16 TriangleChannel::timerPeriod() const {
    return static_cast<quint16>(this->timerLow | (this->timerHigh << 8));
}

quint8 TriangleChannel::lengthCounter() const {
    return this->length;
}

quint8 TriangleChannel::linearCounter() const {
    return this->linear;
}

quint8 TriangleChannel::sequenceIndex() const {
    return this->sequencePosition;
}

void TriangleChannel::reloadLengthCounter() {
    if (this->enabled) {
        this->length = LengthTable[this->lengthIndex];
    }
}

void TriangleChannel::clockLinearCounter() {
    if (this->linearReload) {
        this->linear = this->linearReloadValue;
    } else if (this->linear > 0) {
        --this->linear;
    }
    if ((this->control & 0x80) == 0) {
        this->linearReload = false;
    }
}

void TriangleChannel::clockLengthCounter() {
    if ((this->control & 0x80) == 0 && this->length > 0) {
        --this->length;
    }
}
