#include "Cartridge.h"
#include "NesClock.h"
#include "Ram.h"

#include <cstdlib>
#include <iostream>

namespace {
void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

quint8 read(Bus &bus, quint16 address) {
    quint8 value = bus.openBusValue();
    const Bus::AccessResult result = bus.read(address, value);
    check(result == Bus::AccessResult::Handled, "bus read should be handled");
    return value;
}

void testCpuInternalRamMirrors() {
    Ram internalRam(0x0800);
    RamBusDevice internalRamDevice(internalRam);
    Bus cpuBus(0x10000);
    const Bus::Mapping mapping{
        .start = 0x0000,
        .end = 0x1FFF,
        .priority = 0,
        .flags = AccessFlags::Readable | AccessFlags::Writable,
        .name = QStringLiteral("Test Internal RAM"),
        .device = &internalRamDevice,
        .translate = [](quint16 address) {
            return static_cast<quint16>(address & 0x07FF);
        },
    };
    check(cpuBus.registerMapping(mapping) != 0, "CPU RAM test mapping should register");

    check(cpuBus.write(0x0000, 0x5A) == Bus::AccessResult::Handled,
          "CPU RAM base write should be handled");
    check(read(cpuBus, 0x0000) == 0x5A, "CPU RAM base address should retain its value");
    check(read(cpuBus, 0x0800) == 0x5A, "CPU RAM first mirror should retain its value");
    check(read(cpuBus, 0x1000) == 0x5A, "CPU RAM second mirror should retain its value");
    check(read(cpuBus, 0x1800) == 0x5A, "CPU RAM third mirror should retain its value");

    check(cpuBus.write(0x17FF, 0xA5) == Bus::AccessResult::Handled,
          "CPU RAM mirror write should be handled");
    check(read(cpuBus, 0x07FF) == 0xA5, "writing a mirror should update the base RAM");
}

void testCartridgePrgMirror() {
    Bus cpuBus(0x10000);
    Bus ppuBus(0x4000);
    Cartridge cartridge(16 * 1024, 8 * 1024);
    for (qsizetype index = 0; index < static_cast<qsizetype>(cartridge.prgRom().getSize()); ++index) {
        cartridge.prgRom().setU8(index, static_cast<quint8>(index));
    }

    cartridge.connect(cpuBus, ppuBus);

    check(read(cpuBus, 0x8000) == 0x00, "PRG-ROM should start at CPU $8000");
    check(read(cpuBus, 0x8123) == 0x23, "PRG-ROM should expose its CPU offset");
    check(read(cpuBus, 0xC000) == 0x00, "16 KiB PRG-ROM should mirror at CPU $C000");
    check(read(cpuBus, 0xD234) == 0x34, "PRG-ROM mirror should preserve its offset");

    const Bus::AccessResult writeResult = cpuBus.write(0x8000, 0xFF);
    check(writeResult == Bus::AccessResult::PermissionDenied, "PRG-ROM writes should be denied");
    check(read(cpuBus, 0x8000) == 0x00, "denied PRG-ROM writes should not change data");
}

void testCartridgeChrReadOnly() {
    Bus cpuBus(0x10000);
    Bus ppuBus(0x4000);
    Cartridge cartridge(16 * 1024, 8 * 1024);
    cartridge.chrRom().setU8(0x0000, 0x12);
    cartridge.chrRom().setU8(0x1FFF, 0xE7);

    cartridge.connect(cpuBus, ppuBus);

    check(read(ppuBus, 0x0000) == 0x12, "CHR-ROM should start at PPU $0000");
    check(read(ppuBus, 0x1FFF) == 0xE7, "CHR-ROM should end at PPU $1FFF");
    check(ppuBus.write(0x0000, 0xFF) == Bus::AccessResult::PermissionDenied,
          "CHR-ROM writes should be denied");
    check(read(ppuBus, 0x0000) == 0x12, "denied CHR-ROM writes should not change data");
}

void testCartridgeDisconnect() {
    Bus cpuBus(0x10000);
    Bus ppuBus(0x4000);
    Cartridge cartridge;
    cartridge.connect(cpuBus, ppuBus);

        quint8 value = 0;
        check(cpuBus.read(0x8000, value) == Bus::AccessResult::Handled,
            "connected cartridge should handle CPU reads");
    cartridge.disconnect();

    check(cpuBus.read(0x8000, value) == Bus::AccessResult::Unmapped,
          "disconnected cartridge should leave CPU window unmapped");
    check(ppuBus.read(0x0000, value) == Bus::AccessResult::Unmapped,
          "disconnected cartridge should leave PPU window unmapped");
}

void testCpuPpuBusIsolation() {
    Ram cpuRam(0x0800);
    RamBusDevice cpuRamDevice(cpuRam);
    Bus cpuBus(0x10000);
    const Bus::Mapping mapping{
        .start = 0x0000,
        .end = 0x07FF,
        .priority = 0,
        .flags = AccessFlags::Readable | AccessFlags::Writable,
        .name = QStringLiteral("CPU RAM"),
        .device = &cpuRamDevice,
    };
    check(cpuBus.registerMapping(mapping) != 0, "CPU isolation test mapping should register");
    Bus ppuBus(0x4000);

    check(cpuBus.write(0x0000, 0x33) == Bus::AccessResult::Handled,
          "CPU isolation test write should be handled");
    quint8 value = 0;
    check(ppuBus.read(0x0000, value) == Bus::AccessResult::Unmapped,
          "PPU bus should not see CPU RAM mappings");
    check(cpuBus.read(0x0000, value) == Bus::AccessResult::Handled && value == 0x33,
          "CPU bus should retain its own RAM mapping");
}

void testBusRejectsOverlappingMappings() {
    Ram ram(0x1000);
    RamBusDevice device(ram);
    Bus bus(0x10000);

    const Bus::Mapping firstMapping{
        .start = 0x1000,
        .end = 0x17FF,
        .priority = 0,
        .flags = AccessFlags::Readable | AccessFlags::Writable,
        .name = QStringLiteral("First mapping"),
        .device = &device,
    };
    const Bus::Mapping adjacentMapping{
        .start = 0x1800,
        .end = 0x1FFF,
        .priority = 10,
        .flags = AccessFlags::Readable | AccessFlags::Writable,
        .name = QStringLiteral("Adjacent mapping"),
        .device = &device,
    };
    const Bus::Mapping overlappingMapping{
        .start = 0x1700,
        .end = 0x18FF,
        .priority = 10,
        .flags = AccessFlags::Readable | AccessFlags::Writable,
        .name = QStringLiteral("Overlapping mapping"),
        .device = &device,
    };

    check(bus.registerMapping(firstMapping) != 0, "first mapping should register");
    check(bus.registerMapping(adjacentMapping) != 0, "adjacent mapping should register");
    check(bus.registerMapping(overlappingMapping) == 0,
          "overlapping mapping should be rejected regardless of priority");
}

void testNesClockRatio() {
    quint64 cpuCycles = 0;
    quint64 ppuTicks = 0;
    NesClock clock(
        [&cpuCycles]() { ++cpuCycles; },
        [&ppuTicks]() { ++ppuTicks; });

    clock.runPpuTicks(2);
    check(clock.ppuTicks() == 2, "clock should count PPU ticks");
    check(clock.cpuCycles() == 0, "CPU should wait for three PPU ticks");
    check(ppuTicks == 2, "PPU should receive every clock tick");
    check(cpuCycles == 0, "CPU should not receive an early cycle");

    clock.tick();
    check(clock.cpuCycles() == 1, "three PPU ticks should produce one CPU cycle");
    check(cpuCycles == 1, "CPU should receive one cycle after three PPU ticks");

    clock.runPpuTicks(6);
    check(clock.ppuTicks() == 9, "clock should retain total PPU tick count");
    check(clock.cpuCycles() == 3, "nine PPU ticks should produce three CPU cycles");
}
}

int main() {
    testCpuInternalRamMirrors();
    testCartridgePrgMirror();
    testCartridgeChrReadOnly();
    testCartridgeDisconnect();
    testCpuPpuBusIsolation();
    testBusRejectsOverlappingMappings();
    testNesClockRatio();
    std::cout << "NesCoreTests passed\n";
    return EXIT_SUCCESS;
}