#pragma once

#include "Bus.h"

class Controller final : public BusDevice {
public:
    enum class Button : quint8 {
        A = 0,
        B,
        Select,
        Start,
        Up,
        Down,
        Left,
        Right,
    };

    bool read(quint16 address, quint8 &value) override;
    bool write(quint16 address, quint8 value) override;

    void setButton(Button button, bool pressed);
    void reset();

private:
    quint8 buttons = 0;
    quint8 shiftRegister = 0;
    bool strobe = false;

    void latch();
};