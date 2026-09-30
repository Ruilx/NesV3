#include "Nes.h"

#include <cstdlib>
#include <iostream>

namespace {
class DummyChannel final : public ApuChannel {
public:
    void reset() override {
        this->resetCalls++;
        this->enabled = false;
    }

    void clockCpuCycle() override {
        this->clockCalls++;
    }

    void onQuarterFrame() override {
        this->quarterFrames++;
    }

    void onHalfFrame() override {
        this->halfFrames++;
    }

    bool writeRegister(quint16 offset, quint8 value) override {
        this->lastOffset = offset;
        this->lastValue = value;
        return true;
    }

    float sample() const override {
        return 0.0F;
    }

    void setEnabled(bool value) override {
        this->enabled = value;
    }

    [[nodiscard]] bool isEnabled() const override {
        return this->enabled;
    }

    [[nodiscard]] bool isActive() const override {
        return this->enabled;
    }

    int resetCalls = 0;
    int clockCalls = 0;
    int quarterFrames = 0;
    int halfFrames = 0;
    quint16 lastOffset = 0;
    quint8 lastValue = 0;

private:
    bool enabled = false;
};

struct FrameEvents {
    int quarter = 0;
    int half = 0;
    int irq = 0;
};

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void runCycles(Apu &apu, int cycles) {
    for (int cycle = 0; cycle < cycles; ++cycle) {
        apu.clock();
    }
}

void testSharedControllerAndApuAddress() {
    Nes nes;

    nes.controller().setButton(Controller::Button::A, true);
    nes.cpu().writeRam(0x4016, 0x01);
    nes.cpu().writeRam(0x4016, 0x00);
    check(nes.cpu().readRam8(0x4016) == 1,
        "CPU $4016 read should reach Controller port one");
    check(nes.cpu().readRam8(0x4017) == 0,
        "CPU $4017 read should reach Controller port two");

    nes.cpu().writeRam(0x4017, 0xC0);
    check(nes.apu().registerValue(0x15) == 0xC0,
        "CPU $4017 write should reach the APU frame counter");
}

void testFourStepFrameCounterEvents() {
    Apu apu;
    FrameEvents events;
    apu.setFrameCounterCallback([&events](Apu::FrameCounterEvent event) {
        switch (event) {
        case Apu::FrameCounterEvent::QuarterFrame:
            ++events.quarter;
            break;
        case Apu::FrameCounterEvent::HalfFrame:
            ++events.half;
            break;
        case Apu::FrameCounterEvent::FrameIrq:
            ++events.irq;
            break;
        }
    });

    runCycles(apu, 3728);
    check(events.quarter == 0, "4-step quarter event should not fire early");
    apu.clock();
    check(events.quarter == 1, "4-step quarter event should fire at 3729 cycles");

    runCycles(apu, 3727);
    check(events.half == 0, "4-step half event should not fire early");
    apu.clock();
    check(events.quarter == 2 && events.half == 1,
        "4-step quarter and half events should fire at 7457 cycles");

    runCycles(apu, 7458);
    check(events.quarter == 4 && events.half == 2 && events.irq == 1,
        "4-step events should complete at 14915 cycles");
    check(apu.status().frameIrq, "4-step frame IRQ should be pending");
}

void testFiveStepFrameCounterEvents() {
    Apu apu;
    FrameEvents events;
    apu.setFrameCounterCallback([&events](Apu::FrameCounterEvent event) {
        switch (event) {
        case Apu::FrameCounterEvent::QuarterFrame:
            ++events.quarter;
            break;
        case Apu::FrameCounterEvent::HalfFrame:
            ++events.half;
            break;
        case Apu::FrameCounterEvent::FrameIrq:
            ++events.irq;
            break;
        }
    });
    apu.write(0x17, 0x80);

    runCycles(apu, 14915);
    check(events.quarter == 4 && events.half == 2 && events.irq == 0,
        "5-step mode should omit frame IRQ at the 4-step boundary");
    runCycles(apu, 3725);
    check(events.quarter == 4, "5-step final quarter event should not fire early");
    apu.clock();
    check(events.quarter == 5 && events.half == 2 && events.irq == 0,
        "5-step mode should emit its fifth quarter event at 18641 cycles");
}

void testRegisteredChannelLifecycle() {
    Apu apu;
    auto channel = std::make_unique<DummyChannel>();
    DummyChannel *channelPointer = channel.get();
    const ApuChannelRegistry::ChannelHandle handle = apu.registerChannel(
        std::move(channel),
        ApuChannelRegistry::ChannelDescriptor{
            .id = QStringLiteral("dummy"),
            .registerStart = 0x00,
            .registerEnd = 0x03,
            .statusMask = 0x01,
        });

    check(handle != 0, "registered channel should receive a handle");
    check(apu.write(0x02, 0xA5), "registered channel write should be handled");
    check(channelPointer->lastOffset == 2 && channelPointer->lastValue == 0xA5,
        "registered channel should receive translated register writes");

    apu.write(0x15, 0x01);
    check(channelPointer->isEnabled(), "$4015 should enable registered channels");
    check((apu.status().channelEnable & 0x01) != 0,
        "$4015 status should include registered channel state");

    apu.clock();
    check(channelPointer->clockCalls == 1,
        "registered channel should receive CPU cycle clocks");
    runCycles(apu, 7456);
    check(channelPointer->quarterFrames == 2 && channelPointer->halfFrames == 1,
        "registered channel should receive Frame Counter events");

    apu.reset();
    check(channelPointer->resetCalls == 1 && !channelPointer->isEnabled(),
        "reset should reset registered channels");
    check(apu.unregisterChannel(handle), "registered channel should be removable");
    check(!apu.unregisterChannel(handle), "removing a channel twice should fail");
}
}

int main() {
    testSharedControllerAndApuAddress();
    testFourStepFrameCounterEvents();
    testFiveStepFrameCounterEvents();
    testRegisteredChannelLifecycle();
    std::cout << "ApuTests passed\n";
    return EXIT_SUCCESS;
}
