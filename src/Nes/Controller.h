#pragma once

#include "Bus.h"

class Controller final : public BusDevice {
public:
    struct ReadStats {
        quint64 port1Reads = 0;
        quint64 port2Reads = 0;
        quint64 strobeWrites = 0;
        quint8 lastPort1Bit = 0;
    };

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
    [[nodiscard]] ReadStats takeReadStats();

private:
    quint8 buttons = 0;
    quint8 shiftRegister = 0;
    bool strobe = false;
    ReadStats readStats;

    void latch();
};