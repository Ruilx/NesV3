#pragma once

#include <QtGlobal>

class ApuChannel {
public:
    virtual ~ApuChannel() = default;

    virtual void reset() = 0;
    virtual void clockCpuCycle() = 0;
    virtual void onQuarterFrame() = 0;
    virtual void onHalfFrame() = 0;
    virtual bool writeRegister(quint16 offset, quint8 value) = 0;
    virtual float sample() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    [[nodiscard]] virtual bool isEnabled() const = 0;
    [[nodiscard]] virtual bool isActive() const = 0;
};
