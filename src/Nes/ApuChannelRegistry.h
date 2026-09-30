#pragma once

#include "ApuChannel.h"

#include <QString>

#include <memory>
#include <vector>

class ApuChannelRegistry final {
public:
    using ChannelHandle = quint64;

    struct ChannelDescriptor {
        QString id;
        quint16 registerStart = 0;
        quint16 registerEnd = 0;
        quint8 statusMask = 0;
    };

    ApuChannelRegistry() = default;
    ~ApuChannelRegistry() = default;

    ApuChannelRegistry(const ApuChannelRegistry &) = delete;
    ApuChannelRegistry &operator=(const ApuChannelRegistry &) = delete;

    ChannelHandle registerChannel(
        std::unique_ptr<ApuChannel> channel,
        const ChannelDescriptor &descriptor);
    bool unregisterChannel(ChannelHandle handle);

    bool writeRegister(quint16 address, quint8 value);
    void setEnabledMask(quint8 mask);
    [[nodiscard]] quint8 enabledMask() const;
    [[nodiscard]] quint8 activeMask() const;
    [[nodiscard]] quint8 registeredMask() const;
    [[nodiscard]] float sample(quint8 statusMask) const;
    void contributeAudio(AudioMixContext &context) const;

    void clockCpuCycle();
    void dispatchQuarterFrame();
    void dispatchHalfFrame();
    void reset();

private:
    struct Entry {
        ChannelHandle handle = 0;
        ChannelDescriptor descriptor;
        std::unique_ptr<ApuChannel> channel;
    };

    std::vector<Entry> entries;
    ChannelHandle nextHandle = 1;
};
