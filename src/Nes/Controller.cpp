#include "Controller.h"

bool Controller::read(quint16 address, quint8 &value) {
    if ((address & 0x0001) != 0) {
        ++this->readStats.port2Reads;
        value = 1;
        return true;
    }

    ++this->readStats.port1Reads;
    if (this->strobe) {
        value = static_cast<quint8>(this->buttons & 0x01);
    } else {
        value = static_cast<quint8>(this->shiftRegister & 0x01);
        this->shiftRegister = static_cast<quint8>(
            (this->shiftRegister >> 1) | 0x80);
    }
    this->readStats.lastPort1Bit = value;
    return true;
}

bool Controller::write(quint16, quint8 value) {
    ++this->readStats.strobeWrites;
    const bool nextStrobe = (value & 0x01) != 0;
    if (nextStrobe || (this->strobe && !nextStrobe)) {
        this->latch();
    }
    this->strobe = nextStrobe;
    return true;
}

void Controller::setButton(Button button, bool pressed) {
    const quint8 mask = static_cast<quint8>(1U << static_cast<quint8>(button));
    if (pressed) {
        this->buttons = static_cast<quint8>(this->buttons | mask);
    } else {
        this->buttons = static_cast<quint8>(this->buttons & ~mask);
    }
}

void Controller::reset() {
    this->buttons = 0;
    this->shiftRegister = 0;
    this->strobe = false;
    this->readStats = {};
}

Controller::ReadStats Controller::takeReadStats() {
    const ReadStats result = this->readStats;
    this->readStats = {};
    return result;
}

void Controller::latch() {
    this->shiftRegister = this->buttons;
}