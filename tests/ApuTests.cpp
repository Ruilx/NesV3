#include "Nes.h"

#include <cstdlib>
#include <iostream>

namespace {
void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
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
}

int main() {
    testSharedControllerAndApuAddress();
    std::cout << "ApuTests passed\n";
    return EXIT_SUCCESS;
}
