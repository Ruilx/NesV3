#include "PulseChannel.h"

constexpr quint8 PulseChannel::LengthTable[32];
constexpr quint8 PulseChannel::DutySequences[4][8];

void PulseChannel::reset() {
    this->control = 0;
    this->sweep = 0;
    this->timerLow = 0;
    this->timerHigh = 0;
    this->lengthIndex = 0;
    this->timer = 0;
    this->sequenceIndex = 0;
    this->length = 0;
    this->envelopeDivider = 0;
    this->envelopeDecay = 0;
    this->envelopeStart = false;
    this->timerPhase = false;
    this->enabled = false;
}

void PulseChannel::clockCpuCycle() {
    this->timerPhase = !this->timerPhase;
    if (!this->timerPhase) {
        return;
    }

    if (this->timer == 0) {
        this->timer = this->timerPeriod();
        this->sequenceIndex = static_cast<quint8>((this->sequenceIndex + 1) & 0x07);
    } else {
        --this->timer;
    }
}

void PulseChannel::onQuarterFrame() {
    this->clockEnvelope();
}

void PulseChannel::onHalfFrame() {
    this->clockLengthCounter();
}

bool PulseChannel::writeRegister(quint16 offset, quint8 value) {
    switch (offset) {
    case 0:
        this->control = value;
        this->envelopeStart = true;
        return true;
    case 1:
        this->sweep = value;
        return true;
    case 2:
        this->timerLow = value;
        return true;
    case 3:
        this->lengthIndex = static_cast<quint8>(value >> 3);
        this->timerHigh = value & 0x07;
        this->sequenceIndex = 0;
        this->envelopeStart = true;
        this->reloadLengthCounter();
        return true;
    default:
        return false;
    }
}

float PulseChannel::sample() const {
    if (!this->isActive() || this->timerPeriod() < 8
        || DutySequences[(this->control >> 6) & 0x03][this->sequenceIndex] == 0) {
        return 0.0F;
    }
    return static_cast<float>(this->envelopeVolume()) / 15.0F;
}

void PulseChannel::setEnabled(bool value) {
    this->enabled = value;
    if (!value) {
        this->length = 0;
    }
}

bool PulseChannel::isEnabled() const {
    return this->enabled;
}

bool PulseChannel::isActive() const {
    return this->enabled && this->length != 0;
}

quint8 PulseChannel::duty() const {
    return static_cast<quint8>((this->control >> 6) & 0x03);
}

quint16 PulseChannel::timerPeriod() const {
    return static_cast<quint16>(this->timerLow | (this->timerHigh << 8));
}

quint8 PulseChannel::lengthCounter() const {
    return this->length;
}

quint8 PulseChannel::envelopeVolume() const {
    return (this->control & 0x10) != 0
        ? static_cast<quint8>(this->control & 0x0F)
        : this->envelopeDecay;
}

void PulseChannel::reloadEnvelope() {
    this->envelopeStart = true;
}

void PulseChannel::reloadLengthCounter() {
    if (this->enabled) {
        this->length = LengthTable[this->lengthIndex];
    }
}

void PulseChannel::clockEnvelope() {
    if (this->envelopeStart) {
        this->envelopeStart = false;
        this->envelopeDecay = 15;
        this->envelopeDivider = static_cast<quint8>((this->control & 0x0F) + 1);
        return;
    }

    if (this->envelopeDivider > 0) {
        --this->envelopeDivider;
        return;
    }

    this->envelopeDivider = static_cast<quint8>((this->control & 0x0F) + 1);
    if (this->envelopeDecay > 0) {
        --this->envelopeDecay;
    } else if ((this->control & 0x20) != 0) {
        this->envelopeDecay = 15;
    }
}

void PulseChannel::clockLengthCounter() {
    if ((this->control & 0x20) == 0 && this->length > 0) {
        --this->length;
    }
}
