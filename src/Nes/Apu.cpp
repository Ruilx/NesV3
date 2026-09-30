#include "Apu.h"
#include "NoiseChannel.h"
#include "PulseChannel.h"
#include "TriangleChannel.h"

Apu::Apu() {
    this->dpcmChannel.setIrqCallback([this](bool pending) {
        this->updateDpcmIrq(pending);
    });
    this->reset();
    this->registerChannel(
        std::make_unique<PulseChannel>(true),
        ApuChannelRegistry::ChannelDescriptor{
            .id = QStringLiteral("pulse1"),
            .registerStart = 0x00,
            .registerEnd = 0x03,
            .statusMask = 0x01,
        });
    this->registerChannel(
        std::make_unique<PulseChannel>(false),
        ApuChannelRegistry::ChannelDescriptor{
            .id = QStringLiteral("pulse2"),
            .registerStart = 0x04,
            .registerEnd = 0x07,
            .statusMask = 0x02,
        });
    this->registerChannel(
        std::make_unique<TriangleChannel>(),
        ApuChannelRegistry::ChannelDescriptor{
            .id = QStringLiteral("triangle"),
            .registerStart = 0x08,
            .registerEnd = 0x0B,
            .statusMask = 0x04,
        });
    this->registerChannel(
        std::make_unique<NoiseChannel>(),
        ApuChannelRegistry::ChannelDescriptor{
            .id = QStringLiteral("noise"),
            .registerStart = 0x0C,
            .registerEnd = 0x0F,
            .statusMask = 0x08,
        });
}

bool Apu::read(quint16 address, quint8 &value) {
    ++this->statistics.reads;
    if (address == 0x15) {
        value = static_cast<quint8>(this->channelEnable & 0x1F);
        if (this->frameIrq) {
            value |= 0x40;
        }
        if (this->dpcmIrq) {
            value |= 0x80;
        }
        this->frameIrq = false;
        this->updateIrqLine();
        return true;
    }

    return false;
}

bool Apu::write(quint16 address, quint8 value) {
    ++this->statistics.writes;
    if (address <= 0x13) {
        this->channelRegistry.writeRegister(address, value);
        if (address >= 0x10) {
            this->dpcmChannel.writeRegister(
                static_cast<quint16>(address - 0x10), value);
        }
        this->registers[address] = value;
        return true;
    }
    if (address == 0x15) {
        this->registers[address] = value;
        this->channelEnable = static_cast<quint8>(value & 0x1F);
        this->channelRegistry.setEnabledMask(this->channelEnable);
        this->dpcmChannel.setEnabled((this->channelEnable & 0x10) != 0);
        this->dpcmChannel.clearIrq();
        this->dpcmIrq = false;
        this->updateIrqLine();
        return true;
    }
    if (address == 0x17) {
        this->writeFrameCounter(value);
        return true;
    }

    return false;
}

void Apu::clock() {
    ++this->statistics.clockCalls;
    this->channelRegistry.clockCpuCycle();
    this->dpcmChannel.clockCpuCycle();
    ++this->frameCounterCycles;
    switch (this->frameCounterCycles) {
    case 3729:
        this->emitFrameCounterEvent(FrameCounterEvent::QuarterFrame);
        break;
    case 7457:
        this->emitFrameCounterEvent(FrameCounterEvent::QuarterFrame);
        this->emitFrameCounterEvent(FrameCounterEvent::HalfFrame);
        break;
    case 11186:
        this->emitFrameCounterEvent(FrameCounterEvent::QuarterFrame);
        break;
    case 14915:
        this->emitFrameCounterEvent(FrameCounterEvent::QuarterFrame);
        this->emitFrameCounterEvent(FrameCounterEvent::HalfFrame);
        if (!this->frameCounterMode5 && !this->frameIrqInhibit) {
            this->frameIrq = true;
            this->emitFrameCounterEvent(FrameCounterEvent::FrameIrq);
            this->updateIrqLine();
        }
        if (!this->frameCounterMode5) {
            this->frameCounterCycles = 0;
        }
        break;
    case 18641:
        if (this->frameCounterMode5) {
            this->emitFrameCounterEvent(FrameCounterEvent::QuarterFrame);
            this->frameCounterCycles = 0;
        }
        break;
    default:
        break;
    }
}

void Apu::reset() {
    for (quint8 &value : this->registers) {
        value = 0;
    }
    this->channelEnable = 0;
    this->frameIrq = false;
    this->dpcmIrq = false;
    this->frameCounterMode5 = false;
    this->frameIrqInhibit = false;
    this->frameCounterCycles = 0;
    this->channelRegistry.reset();
    this->dpcmChannel.reset();
    this->statistics = Stats();
    this->irqLinePending = false;
    this->updateIrqLine();
}

void Apu::setIrqCallback(IrqCallback callback) {
    this->irqCallback = std::move(callback);
    this->updateIrqLine();
}

void Apu::setDpcmMemoryReader(DpcmChannel::MemoryReader reader) {
    this->dpcmChannel.setMemoryReader(std::move(reader));
}

void Apu::setFrameCounterCallback(FrameCounterCallback callback) {
    this->frameCounterCallback = std::move(callback);
}

ApuChannelRegistry::ChannelHandle Apu::registerChannel(
    std::unique_ptr<ApuChannel> channel,
    const ApuChannelRegistry::ChannelDescriptor &descriptor) {
    const ApuChannelRegistry::ChannelHandle handle =
        this->channelRegistry.registerChannel(std::move(channel), descriptor);
    this->channelRegistry.setEnabledMask(this->channelEnable);
    return handle;
}

bool Apu::unregisterChannel(ApuChannelRegistry::ChannelHandle handle) {
    return this->channelRegistry.unregisterChannel(handle);
}

Apu::Status Apu::status() const {
    return Status{
        .channelEnable = static_cast<quint8>(
            this->channelRegistry.enabledMask()
            | (this->dpcmChannel.isEnabled() ? 0x10 : 0)),
        .frameIrq = this->frameIrq,
        .dpcmIrq = this->dpcmIrq,
    };
}

quint8 Apu::registerValue(quint8 index) const {
    return index < 0x16 ? this->registers[index] : 0;
}

Apu::Stats Apu::takeStats() {
    const Stats result = this->statistics;
    this->statistics = Stats();
    return result;
}

void Apu::writeFrameCounter(quint8 value) {
    this->registers[0x15] = value;
    this->frameCounterMode5 = (value & 0x80) != 0;
    this->frameIrqInhibit = (value & 0x40) != 0;
    if (this->frameIrqInhibit) {
        this->frameIrq = false;
    }
    this->frameCounterCycles = 0;
    this->updateIrqLine();
}

void Apu::emitFrameCounterEvent(FrameCounterEvent event) {
    if (event == FrameCounterEvent::QuarterFrame) {
        this->channelRegistry.dispatchQuarterFrame();
    } else if (event == FrameCounterEvent::HalfFrame) {
        this->channelRegistry.dispatchHalfFrame();
    }
    if (this->frameCounterCallback) {
        this->frameCounterCallback(event);
    }
}

void Apu::updateIrqLine() {
    const bool pending = this->frameIrq || this->dpcmIrq;
    if (pending != this->irqLinePending) {
        this->irqLinePending = pending;
        ++this->statistics.irqLineChanges;
    }
    if (this->irqCallback) {
        this->irqCallback(pending);
    }
}

void Apu::updateDpcmIrq(bool pending) {
    this->dpcmIrq = pending;
    this->updateIrqLine();
}
