#include "ApuChannelRegistry.h"

ApuChannelRegistry::ChannelHandle ApuChannelRegistry::registerChannel(
    std::unique_ptr<ApuChannel> channel,
    const ChannelDescriptor &descriptor) {
    if (!channel || descriptor.registerStart > descriptor.registerEnd
        || descriptor.statusMask == 0) {
        return 0;
    }

    const ChannelHandle handle = this->nextHandle++;
    this->entries.push_back(Entry{
        .handle = handle,
        .descriptor = descriptor,
        .channel = std::move(channel),
    });
    return handle;
}

bool ApuChannelRegistry::unregisterChannel(ChannelHandle handle) {
    for (auto iterator = this->entries.begin(); iterator != this->entries.end();
         ++iterator) {
        if (iterator->handle == handle) {
            this->entries.erase(iterator);
            return true;
        }
    }
    return false;
}

bool ApuChannelRegistry::writeRegister(quint16 address, quint8 value) {
    for (Entry &entry : this->entries) {
        if (address < entry.descriptor.registerStart
            || address > entry.descriptor.registerEnd) {
            continue;
        }
        return entry.channel->writeRegister(
            static_cast<quint16>(address - entry.descriptor.registerStart), value);
    }
    return false;
}

void ApuChannelRegistry::setEnabledMask(quint8 mask) {
    for (Entry &entry : this->entries) {
        entry.channel->setEnabled((mask & entry.descriptor.statusMask) != 0);
    }
}

quint8 ApuChannelRegistry::enabledMask() const {
    quint8 mask = 0;
    for (const Entry &entry : this->entries) {
        if (entry.channel->isEnabled()) {
            mask = static_cast<quint8>(mask | entry.descriptor.statusMask);
        }
    }
    return mask;
}

quint8 ApuChannelRegistry::activeMask() const {
    quint8 mask = 0;
    for (const Entry &entry : this->entries) {
        if (entry.channel->isActive()) {
            mask = static_cast<quint8>(mask | entry.descriptor.statusMask);
        }
    }
    return mask;
}

quint8 ApuChannelRegistry::registeredMask() const {
    quint8 mask = 0;
    for (const Entry &entry : this->entries) {
        mask = static_cast<quint8>(mask | entry.descriptor.statusMask);
    }
    return mask;
}

float ApuChannelRegistry::sample(quint8 statusMask) const {
    float result = 0.0F;
    for (const Entry &entry : this->entries) {
        if ((entry.descriptor.statusMask & statusMask) != 0) {
            result += entry.channel->sample();
        }
    }
    return result;
}

void ApuChannelRegistry::clockCpuCycle() {
    for (Entry &entry : this->entries) {
        entry.channel->clockCpuCycle();
    }
}

void ApuChannelRegistry::dispatchQuarterFrame() {
    for (Entry &entry : this->entries) {
        entry.channel->onQuarterFrame();
    }
}

void ApuChannelRegistry::dispatchHalfFrame() {
    for (Entry &entry : this->entries) {
        entry.channel->onHalfFrame();
    }
}

void ApuChannelRegistry::reset() {
    for (Entry &entry : this->entries) {
        entry.channel->reset();
    }
}
