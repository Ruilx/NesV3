#include "Nes.h"
#include "DpcmChannel.h"
#include "NoiseChannel.h"
#include "PulseChannel.h"
#include "TriangleChannel.h"

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
            .registerStart = 0x10,
            .registerEnd = 0x13,
            .statusMask = 0x10,
        });

    check(handle != 0, "registered channel should receive a handle");
    check(apu.write(0x12, 0xA5), "registered channel write should be handled");
    check(channelPointer->lastOffset == 2 && channelPointer->lastValue == 0xA5,
        "registered channel should receive translated register writes");

    apu.write(0x15, 0x10);
    check(channelPointer->isEnabled(), "$4015 should enable registered channels");
    check((apu.status().channelEnable & 0x10) != 0,
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

void testPulseChannelWaveform() {
    PulseChannel pulse;
    pulse.setEnabled(true);
    check(pulse.writeRegister(0, 0x9F), "pulse control register should be accepted");
    check(pulse.writeRegister(2, 0x08), "pulse timer low register should be accepted");
    check(pulse.writeRegister(3, 0x08), "pulse timer high register should be accepted");
    check(pulse.duty() == 2 && pulse.timerPeriod() == 8,
        "pulse should decode duty and 11-bit timer period");
    check(pulse.lengthCounter() == 254,
        "pulse high register should load the length counter");
    check(pulse.sample() == 0.0F,
        "pulse duty sequence should start at its first low phase");

    pulse.clockCpuCycle();
    check(pulse.sample() > 0.9F,
        "pulse timer should advance to the next duty phase");

    pulse.onHalfFrame();
    check(pulse.lengthCounter() == 253,
        "pulse length counter should decrement on half frame");
    pulse.setEnabled(false);
    check(!pulse.isActive() && pulse.lengthCounter() == 0,
        "disabling pulse should clear its length counter");
}

void testApuBuiltInPulseChannels() {
    Apu apu;
    apu.write(0x00, 0xDF);
    apu.write(0x02, 0x08);
    apu.write(0x03, 0x08);
    apu.write(0x15, 0x01);
    check((apu.status().channelEnable & 0x01) != 0,
        "APU should expose the built-in first pulse status bit");
    check(apu.write(0x04, 0xDF), "APU should route the second pulse registers");
}

void testPulseSweep() {
    PulseChannel pulse1(true);
    pulse1.setEnabled(true);
    pulse1.writeRegister(0, 0x90);
    pulse1.writeRegister(2, 0x00);
    pulse1.writeRegister(3, 0x01);
    pulse1.writeRegister(1, 0x91);
    check(pulse1.sweepTargetPeriod() == 0x0180,
        "positive sweep should add the shifted timer period");
    check(pulse1.sweepDivider() == 0,
        "writing sweep should wait for the first half frame");

    pulse1.onHalfFrame();
    check(pulse1.sweepDivider() == 2 && pulse1.timerPeriod() == 0x0100,
        "first half frame should reload the sweep divider without updating");
    pulse1.onHalfFrame();
    pulse1.onHalfFrame();
    pulse1.onHalfFrame();
    check(pulse1.timerPeriod() == 0x0180,
        "sweep should update the timer when its divider expires");

    PulseChannel pulse2(false);
    pulse2.setEnabled(true);
    pulse2.writeRegister(0, 0x90);
    pulse2.writeRegister(2, 0x00);
    pulse2.writeRegister(3, 0x01);
    pulse2.writeRegister(1, 0x99);
    pulse1.writeRegister(2, 0x00);
    pulse1.writeRegister(3, 0x01);
    pulse1.writeRegister(1, 0x99);
    pulse1.onHalfFrame();
    pulse2.onHalfFrame();
    pulse1.onHalfFrame();
    pulse2.onHalfFrame();
    pulse1.onHalfFrame();
    pulse2.onHalfFrame();
    pulse1.onHalfFrame();
    pulse2.onHalfFrame();
    check(pulse1.timerPeriod() == 0x007F && pulse2.timerPeriod() == 0x0080,
        "pulse 1 negative sweep should subtract one extra from pulse 2");

    PulseChannel muted;
    muted.setEnabled(true);
    muted.writeRegister(0, 0x90);
    muted.writeRegister(2, 0x07);
    muted.writeRegister(3, 0x00);
    check(muted.sweepMutes(), "pulse periods below eight should be muted");
    muted.writeRegister(2, 0xFF);
    muted.writeRegister(3, 0x07);
    muted.writeRegister(1, 0x80);
    check(muted.sweepMutes(), "positive sweep targets above 0x7ff should mute");
}

void testTriangleChannel() {
    TriangleChannel triangle;
    triangle.setEnabled(true);
    check(triangle.writeRegister(0, 0x85),
        "triangle control register should be accepted");
    check(triangle.writeRegister(2, 0x08),
        "triangle timer low register should be accepted");
    check(triangle.writeRegister(3, 0x08),
        "triangle length register should be accepted");
    check(triangle.timerPeriod() == 8 && triangle.lengthCounter() == 254,
        "triangle should decode timer and length fields");
    check(triangle.linearCounter() == 0 && !triangle.isActive(),
        "triangle should wait for its quarter frame linear reload");

    triangle.onQuarterFrame();
    check(triangle.linearCounter() == 5 && triangle.isActive(),
        "triangle quarter frame should reload the linear counter");
    check(triangle.sample() == 1.0F,
        "triangle should start at the top of its waveform");
    triangle.clockCpuCycle();
    check(triangle.sequenceIndex() == 1,
        "triangle timer should advance its 32-step sequence");

    triangle.onHalfFrame();
    check(triangle.lengthCounter() == 254,
        "triangle control halt should preserve its length counter");
    triangle.writeRegister(0, 0x05);
    triangle.onQuarterFrame();
    triangle.onQuarterFrame();
    check(triangle.linearCounter() == 4,
        "triangle linear counter should decrement after reload is cleared");
    triangle.setEnabled(false);
    check(!triangle.isActive() && triangle.lengthCounter() == 0,
        "disabling triangle should clear its length counter");
}

void testApuBuiltInTriangleChannel() {
    Apu apu;
    apu.write(0x08, 0x87);
    apu.write(0x0A, 0x08);
    apu.write(0x0B, 0x08);
    apu.write(0x15, 0x04);
    check((apu.status().channelEnable & 0x04) != 0,
        "APU should expose the built-in triangle status bit");
}

void testNoiseChannel() {
    NoiseChannel noise;
    noise.setEnabled(true);
    noise.writeRegister(0, 0x03);
    noise.writeRegister(2, 0x8F);
    noise.writeRegister(3, 0x08);
    check(noise.timerPeriod() == 4068 && noise.mode(),
        "noise should decode mode and period fields");
    check(noise.lengthCounter() == 254,
        "noise should decode its length counter index");
    check(noise.shiftRegister() == 1,
        "noise should reset its LFSR to one");

    noise.onQuarterFrame();
    check(noise.envelopeVolume() == 15 && noise.envelopeDivider() == 3,
        "noise quarter frame should start its envelope");
    for (int cycle = 0; cycle < 4069; ++cycle) {
        noise.clockCpuCycle();
    }
    check(noise.shiftRegister() != 1,
        "noise timer should advance its LFSR");
    check(noise.sample() >= 0.0F && noise.sample() <= 1.0F,
        "noise sample should remain normalized");
    noise.writeRegister(0, 0x1F);
    check(noise.sample() == 1.0F || noise.sample() == 0.0F,
        "noise constant volume should use the register volume when audible");

    noise.writeRegister(0, 0x23);
    noise.onHalfFrame();
    check(noise.lengthCounter() == 254,
        "noise loop flag should halt the length counter");
    noise.writeRegister(0, 0x03);
    noise.onHalfFrame();
    check(noise.lengthCounter() == 253,
        "noise length counter should decrement when loop is clear");
    noise.setEnabled(false);
    check(!noise.isActive() && noise.lengthCounter() == 0,
        "disabling noise should clear its length counter");
}

void testApuBuiltInNoiseChannel() {
    Apu apu;
    apu.write(0x0C, 0x03);
    apu.write(0x0E, 0x00);
    apu.write(0x0F, 0x08);
    apu.write(0x15, 0x08);
    check((apu.status().channelEnable & 0x08) != 0,
        "APU should expose the built-in noise status bit");
}

void testDpcmChannel() {
    DpcmChannel dpcm;
    quint16 readAddress = 0;
    int irqChanges = 0;
    bool irqPending = false;
    dpcm.setMemoryReader([&readAddress](quint16 address) {
        readAddress = address;
        return static_cast<quint8>(0xFF);
    });
    dpcm.setIrqCallback([&irqChanges, &irqPending](bool pending) {
        ++irqChanges;
        irqPending = pending;
    });
    dpcm.writeRegister(0, 0x8F);
    dpcm.writeRegister(1, 0x40);
    dpcm.writeRegister(2, 0x02);
    dpcm.writeRegister(3, 0x00);
    dpcm.setEnabled(true);
    check(dpcm.currentAddress() == 0xC080 && dpcm.sampleLength() == 1,
        "DPCM should decode sample address and length registers");
    check(dpcm.timerPeriod() == 54 && dpcm.outputLevel() == 0x40,
        "DPCM should decode rate and direct output registers");

    dpcm.clockCpuCycle();
    check(readAddress == 0xC080 && dpcm.bytesRemaining() == 0,
        "DPCM should fetch the sample byte at its decoded address");
    for (int bit = 1; bit < 8; ++bit) {
        for (int cycle = 0; cycle < 55; ++cycle) {
            dpcm.clockCpuCycle();
        }
    }
    check(dpcm.irqPending() && irqPending && irqChanges == 2,
        "DPCM should raise IRQ after consuming the final sample bit");
    check(dpcm.outputLevel() == 0x50,
        "DPCM one bits should increase the output by two with saturation");
    dpcm.clearIrq();
    check(!dpcm.irqPending() && !irqPending,
        "DPCM IRQ should be clearable");
}

void testApuBuiltInDpcmChannel() {
    Apu apu;
    bool irqPending = false;
    apu.setDpcmMemoryReader([](quint16) { return static_cast<quint8>(0); });
    apu.setIrqCallback([&irqPending](bool pending) {
        irqPending = pending;
    });
    apu.write(0x10, 0x8F);
    apu.write(0x11, 0x40);
    apu.write(0x12, 0x00);
    apu.write(0x13, 0x00);
    apu.write(0x15, 0x10);
    check((apu.status().channelEnable & 0x10) != 0,
        "APU should expose the built-in DPCM status bit");
    runCycles(apu, 1 + 7 * 55);
    check(apu.status().dpcmIrq && irqPending,
        "APU should expose and route DPCM IRQ state");
    quint8 status = 0;
    check(apu.read(0x15, status) && (status & 0x80) != 0,
        "APU status read should expose DPCM IRQ");
    check(apu.status().dpcmIrq && irqPending,
        "APU status read should preserve DPCM IRQ");
    apu.write(0x15, 0x00);
    check(!apu.status().dpcmIrq && !irqPending,
        "APU status write should clear DPCM IRQ");
}
}

int main() {
    testSharedControllerAndApuAddress();
    testFourStepFrameCounterEvents();
    testFiveStepFrameCounterEvents();
    testRegisteredChannelLifecycle();
    testPulseChannelWaveform();
    testApuBuiltInPulseChannels();
    testPulseSweep();
    testTriangleChannel();
    testApuBuiltInTriangleChannel();
    testNoiseChannel();
    testApuBuiltInNoiseChannel();
    testDpcmChannel();
    testApuBuiltInDpcmChannel();
    std::cout << "ApuTests passed\n";
    return EXIT_SUCCESS;
}
