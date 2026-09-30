#include "Nes.h"

#include <cstdlib>
#include <iostream>

namespace {
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
}

int main() {
    testSharedControllerAndApuAddress();
    testFourStepFrameCounterEvents();
    testFiveStepFrameCounterEvents();
    std::cout << "ApuTests passed\n";
    return EXIT_SUCCESS;
}
