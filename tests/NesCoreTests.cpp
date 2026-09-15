#include "Cartridge.h"
#include "ChrTileDecoder.h"
#include "Cpu.h"
#include "Nes.h"
#include "NesClock.h"
#include "Ram.h"

#include <cstdlib>
#include <iostream>
#include <QTemporaryFile>

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

void testInesMapper000Load() {
    QByteArray rom(16 + 32 * 1024 + 8 * 1024, '\0');
    rom[0] = 'N';
    rom[1] = 'E';
    rom[2] = 'S';
    rom[3] = 0x1A;
    rom[4] = 2;
    rom[5] = 1;
    rom[6] = 0x01;
    rom[16] = static_cast<char>(0xA5);
    rom[16 + 16 * 1024] = static_cast<char>(0x5A);
    rom[16 + 32 * 1024] = static_cast<char>(0x3C);

    QTemporaryFile file;
    check(file.open(), "temporary iNES file should open");
    check(file.write(rom) == rom.size(), "temporary iNES file should be written");
    file.flush();

    Bus cpuBus(0x10000);
    Bus ppuBus(0x4000);
    Cartridge cartridge;
    QString error;
    check(cartridge.loadFromFile(file.fileName(), error),
        "Mapper 0 iNES image should load");
    check(error.isEmpty(), "successful iNES load should not report an error");
    check(cartridge.isLoaded(), "loaded cartridge should report its loaded state");
    check(cartridge.header().prgRomBanks == 2 && cartridge.header().chrRomBanks == 1,
        "iNES header should expose PRG and CHR bank counts");
    check(cartridge.header().mapperNumber == 0
          && cartridge.header().verticalMirroring,
        "iNES header should expose Mapper 0 vertical mirroring");

    cartridge.connect(cpuBus, ppuBus);
    check(read(cpuBus, 0x8000) == 0xA5,
        "32 KiB PRG-ROM should map its first bank at $8000");
    check(read(cpuBus, 0xC000) == 0x5A,
        "32 KiB PRG-ROM should map its second bank at $C000");
    check(read(ppuBus, 0x0000) == 0x3C,
        "CHR-ROM should map at PPU $0000");
    check(ppuBus.write(0x0000, 0xFF) == Bus::AccessResult::PermissionDenied,
        "CHR-ROM should remain read-only");
}

void testChrRamInesLoad() {
    QByteArray rom(16 + 16 * 1024, '\0');
    rom[0] = 'N';
    rom[1] = 'E';
    rom[2] = 'S';
    rom[3] = 0x1A;
    rom[4] = 1;

    QTemporaryFile file;
    check(file.open(), "temporary CHR-RAM iNES file should open");
    check(file.write(rom) == rom.size(), "temporary CHR-RAM file should be written");
    file.flush();

    Bus cpuBus(0x10000);
    Bus ppuBus(0x4000);
    Cartridge cartridge;
    QString error;
    check(cartridge.loadFromFile(file.fileName(), error),
        "CHR-RAM iNES image should load");
    cartridge.connect(cpuBus, ppuBus);
    check(ppuBus.write(0x0000, 0xC3) == Bus::AccessResult::Handled,
        "CHR-RAM should accept PPU writes");
    check(read(ppuBus, 0x0000) == 0xC3,
        "CHR-RAM should return its written value");
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

void testNtscMasterFrame() {
    quint64 cpuCycles = 0;
    quint64 ppuTicks = 0;
    NesClock clock(
        [&cpuCycles]() { ++cpuCycles; },
        [&ppuTicks]() { ++ppuTicks; });

    check(clock.timing().region == NesClock::Region::Ntsc,
        "the default timing profile should be NTSC");
    check(clock.timing().ppuTicksPerCpuCycle == 3,
        "NTSC should use three PPU ticks per CPU cycle");
    check(clock.timing().ppuTicksPerFrame() == 341ULL * 262ULL,
        "NTSC frame length should be 341 times 262 master ticks");

    clock.runFrame();
    check(ppuTicks == 341ULL * 262ULL,
        "one NTSC frame should run the complete PPU tick budget");
    check(cpuCycles == (341ULL * 262ULL) / 3,
        "one NTSC frame should preserve the three to one clock ratio");
}

void testCpuBasicExecution() {
    Cpu cpu;
    Ram programRam(0x8000);
    RamBusDevice programDevice(programRam);
    const Bus::Mapping programMapping{
        .start = 0x8000,
        .end = 0xFFFF,
        .priority = 0,
        .flags = AccessFlags::Readable | AccessFlags::Writable | AccessFlags::Executable,
        .name = QStringLiteral("Test program"),
        .device = &programDevice,
        .translate = [](quint16 address) {
            return static_cast<quint16>(address - 0x8000);
        },
    };
    check(cpu.bus().registerMapping(programMapping) != 0, "CPU test program mapping should register");

    programRam.setU8(0x7FFC, 0x00);
    programRam.setU8(0x7FFD, 0x80);
    programRam.setU8(0x0000, 0xA9); // LDA #$42
    programRam.setU8(0x0001, 0x42);
    programRam.setU8(0x0002, 0xAA); // TAX
    programRam.setU8(0x0003, 0xE8); // INX
    programRam.setU8(0x0004, 0x8D); // STA $0200
    programRam.setU8(0x0005, 0x00);
    programRam.setU8(0x0006, 0x02);

    cpu.reset();
    Cpu::CpuReg registers{};
    cpu.getContent(registers);
    check(registers.pc == 0x8000, "CPU reset should load the reset vector");

    check(cpu.exec(10) == 10, "CPU should report the executed instruction cycles");
    cpu.getContent(registers);
    check(registers.a == 0x42, "LDA should load the accumulator");
    check(registers.x == 0x43, "TAX and INX should update X");
    check((registers.p & (Cpu::ZFlag | Cpu::NFlag)) == 0,
          "positive non-zero loads should clear zero and negative flags");
    check(cpu.readRam8(0x0200) == 0x42, "STA should write through the CPU bus");
    check(registers.pc == 0x8007, "CPU should advance the program counter");

    programRam.setU8(0x0000, 0x20); // JSR $8006
    programRam.setU8(0x0001, 0x06);
    programRam.setU8(0x0002, 0x80);
    programRam.setU8(0x0003, 0xA9); // LDA #$55 after RTS
    programRam.setU8(0x0004, 0x55);
    programRam.setU8(0x0006, 0xA9); // subroutine: LDA #$AA
    programRam.setU8(0x0007, 0xAA);
    programRam.setU8(0x0008, 0x60); // RTS

    cpu.reset();
    check(cpu.exec(14) == 14, "JSR, subroutine LDA and RTS should consume 14 cycles");
    cpu.getContent(registers);
    check(registers.a == 0xAA, "subroutine should execute before returning");
    check(registers.pc == 0x8003, "RTS should return to the instruction after JSR");
    check(registers.s == 0xFF, "RTS should restore the stack pointer");

    check(cpu.exec(2) == 2, "returned code should continue executing");
    cpu.getContent(registers);
    check(registers.a == 0x55, "execution should continue after RTS");
}

void testCpuCycleAndBranchTiming() {
    Cpu cpu;
    Ram programRam(0x8000);
    RamBusDevice programDevice(programRam);
    const Bus::Mapping programMapping{
        .start = 0x8000,
        .end = 0xFFFF,
        .priority = 0,
        .flags = AccessFlags::Readable | AccessFlags::Writable | AccessFlags::Executable,
        .name = QStringLiteral("CPU timing test program"),
        .device = &programDevice,
        .translate = [](quint16 address) {
            return static_cast<quint16>(address - 0x8000);
        },
    };
    check(cpu.bus().registerMapping(programMapping) != 0, "CPU timing test mapping should register");

    programRam.setU8(0x7FFC, 0x00);
    programRam.setU8(0x7FFD, 0x80);
    programRam.setU8(0x0000, 0xA9); // LDA #$01
    programRam.setU8(0x0001, 0x01);
    programRam.setU8(0x0002, 0xEA); // NOP

    cpu.reset();
    cpu.tickCpuCycle();
    Cpu::CpuReg registers{};
    cpu.getContent(registers);
    check(cpu.getTotalCycles() == 1, "one CPU tick should consume one cycle");
    check(registers.a == 0x01, "instruction effects should begin on the first CPU cycle");
    cpu.tickCpuCycle();
    check(cpu.getTotalCycles() == 2, "two CPU ticks should consume two cycles");
    cpu.getContent(registers);
    check(registers.pc == 0x8002, "two-cycle instruction should finish after two CPU ticks");
    cpu.tickCpuCycle();
    check(cpu.getTotalCycles() == 3, "the next instruction should consume the next CPU tick");
    cpu.getContent(registers);
    check(registers.pc == 0x8003, "the next instruction should start at the next cycle boundary");

    programRam.setU8(0x0000, 0xD0); // BNE +2
    programRam.setU8(0x0001, 0x02);
    registers = Cpu::CpuReg{
        .pc = 0x8000, .a = 0, .p = Cpu::RFlag | Cpu::ZFlag,
        .x = 0, .y = 0, .s = 0xFF, .intPending = Cpu::CpuInterrupt::None};
    cpu.reset();
    cpu.setContent(registers);
    cpu.setTotalCycles(0);
    check(cpu.exec(2) == 2, "not-taken branch should consume two cycles");
    cpu.getContent(registers);
    check(registers.pc == 0x8002, "not-taken branch should continue sequentially");

    registers.p = Cpu::RFlag;
    registers.pc = 0x8000;
    cpu.reset();
    cpu.setContent(registers);
    cpu.setTotalCycles(0);
    check(cpu.exec(3) == 3, "taken same-page branch should consume three cycles");
    cpu.getContent(registers);
    check(registers.pc == 0x8004, "taken same-page branch should apply its offset");

    programRam.setU8(0x00FD, 0xD0); // BNE +2 from $80FF to $8101
    programRam.setU8(0x00FE, 0x02);
    registers.pc = 0x80FD;
    cpu.reset();
    cpu.setContent(registers);
    cpu.setTotalCycles(0);
    check(cpu.exec(4) == 4, "taken cross-page branch should consume four cycles");
    cpu.getContent(registers);
    check(registers.pc == 0x8101, "taken cross-page branch should cross the page");

    programRam.setU8(0x7FFA, 0x00);
    programRam.setU8(0x7FFB, 0x90);
    registers = Cpu::CpuReg{
        .pc = 0x8000, .a = 0, .p = Cpu::RFlag, .x = 0, .y = 0,
        .s = 0xFF, .intPending = Cpu::CpuInterrupt::None};
    cpu.reset();
    cpu.setContent(registers);
    cpu.nmi();
    cpu.tickCpuCycle();
    cpu.getContent(registers);
    check(registers.pc == 0x9000, "NMI should load the NMI vector at the instruction boundary");
    check(registers.s == 0xFC, "NMI should push PC and status onto the stack");
    check(cpu.getTotalCycles() == 1, "NMI should begin with one CPU cycle");
    for (int cycle = 0; cycle < 6; ++cycle) {
        cpu.tickCpuCycle();
    }
    check(cpu.getTotalCycles() == 7, "NMI should consume seven CPU cycles");

    registers.pc = 0x8000;
    registers.s = 0xFF;
    registers.intPending = Cpu::CpuInterrupt::None;
    cpu.setContent(registers);
    cpu.setTotalCycles(0);
    cpu.dma(2);
    cpu.tickCpuCycle();
    cpu.tickCpuCycle();
    cpu.getContent(registers);
    check(cpu.getTotalCycles() == 2, "DMA should consume CPU cycles one at a time");
    check(registers.pc == 0x8000, "DMA should pause instruction fetch");
}

void testPpuRegistersAndBusMirror() {
    Nes nes;
    Cpu &cpu = nes.cpu();

    cpu.writeRam(0x2000, 0x04);
    cpu.writeRam(0x2008, 0x00);

    cpu.writeRam(0x2006, 0x20);
    cpu.writeRam(0x2006, 0x00);
    cpu.writeRam(0x2007, 0x5A);
    cpu.writeRam(0x2006, 0x20);
    cpu.writeRam(0x2006, 0x00);
    check(cpu.readRam8(0x2007) == 0x00, "nametable reads should use the initial read buffer");
    check(cpu.readRam8(0x2007) == 0x5A, "PPUDATA should return the buffered nametable value");

    cpu.writeRam(0x2003, 0xFE);
    cpu.writeRam(0x2004, 0xA1);
    cpu.writeRam(0x2004, 0xB2);
    cpu.writeRam(0x2003, 0xFE);
    check(cpu.readRam8(0x2004) == 0xA1, "OAMDATA should read the first byte");
}

    void testPpuVblankAndNmiTiming() {
        Nes nes;
        Cpu::CpuReg registers{};

        nes.ppu().write(0x2000, 0x80);
        nes.clock().runPpuTicks(241 * 341 + 1);
        check(nes.ppu().scanline() == 241, "PPU should enter VBlank on scanline 241");
        check(nes.ppu().dot() == 1, "PPU should set VBlank at dot 1");
        check((nes.ppu().read(0x2002, registers.a), (registers.a & 0x80) != 0),
            "PPUSTATUS should expose VBlank");
        nes.cpu().getContent(registers);
        check((registers.intPending & Cpu::NmiFlag) != 0,
            "VBlank should request a CPU NMI when enabled");

        nes.clock().runPpuTicks((261 - 241) * 341);
        check(nes.ppu().scanline() == 261, "PPU should reach the pre-render scanline");
        check(nes.ppu().dot() == 1, "PPU should reach pre-render dot 1");
        check((nes.ppu().read(0x2002, registers.a), (registers.a & 0x80) == 0),
            "pre-render should clear VBlank");

        nes.clock().runPpuTicks(340);
        check(nes.ppu().scanline() == 0 && nes.ppu().dot() == 0,
            "PPU should wrap to the next frame after pre-render");
        check(nes.ppu().frame() == 1, "PPU should increment the frame counter");
    }

        void testPpuMemoryMirrors() {
            Ppu ppu;

            check(ppu.bus().write(0x2000, 0x12) == Bus::AccessResult::Handled,
                "nametable base write should be handled");
            check(read(ppu.bus(), 0x3000) == 0x12,
                "the first nametable page should mirror at $3000");
            check(ppu.bus().write(0x3EFF, 0x34) == Bus::AccessResult::Handled,
                "nametable mirror write should be handled");
            check(read(ppu.bus(), 0x2EFF) == 0x34,
                "nametable mirror writes should reach the base RAM");

            check(ppu.bus().write(0x3F00, 0x56) == Bus::AccessResult::Handled,
                "universal background color write should be handled");
            check(read(ppu.bus(), 0x3F10) == 0x56,
                "sprite palette entry $10 should mirror background entry $00");
            check(ppu.bus().write(0x3F14, 0x78) == Bus::AccessResult::Handled,
                "sprite palette mirror write should be handled");
            check(read(ppu.bus(), 0x3F04) == 0x78,
                "sprite palette mirror writes should reach the background slot");
        }

        void testPpuNametableRendering() {
            Ppu ppu;
            ppu.setNametableMirroring(Ppu::NametableMirroring::Vertical);

            Ram chrRam(0x2000);
            RamBusDevice chrDevice(chrRam);
            const Bus::Mapping chrMapping{
              .start = 0x0000,
              .end = 0x1FFF,
              .priority = 0,
              .flags = AccessFlags::Readable,
              .name = QStringLiteral("Rendering CHR"),
              .device = &chrDevice,
            };
            check(ppu.bus().registerMapping(chrMapping) != 0,
                "rendering CHR mapping should register");

            chrRam.setU8(0x0000, 0x80);
            chrRam.setU8(0x0008, 0x80);
            check(ppu.bus().write(0x2000, 0x00) == Bus::AccessResult::Handled,
                "nametable tile write should be handled");
            check(ppu.bus().write(0x23C0, 0x01) == Bus::AccessResult::Handled,
                "attribute write should be handled");
            check(ppu.bus().write(0x3F00, 0x0F) == Bus::AccessResult::Handled,
                "background palette write should be handled");
            check(ppu.bus().write(0x3F05, 0x21) == Bus::AccessResult::Handled,
                "palette entry write should be handled");
            check(ppu.bus().write(0x3F06, 0x22) == Bus::AccessResult::Handled,
                "palette entry write should be handled");
            check(ppu.bus().write(0x3F07, 0x23) == Bus::AccessResult::Handled,
                "palette entry write should be handled");

            QVector<quint8> pixels;
            QVector<quint8> subpalette;
            check(ppu.renderNametableTile(0, 0, 0, pixels, subpalette),
                "PPU should render a nametable tile");
            check(pixels.size() == 64 && pixels[0] == 3,
                "rendered tile should contain decoded CHR pixels");
            check(subpalette == QVector<quint8>({0x0F, 0x21, 0x22, 0x23}),
                "attribute quadrant should select the correct subpalette");

            check(ppu.bus().write(0x2000, 0x44) == Bus::AccessResult::Handled,
                "vertical mirror base write should be handled");
            check(read(ppu.bus(), 0x2800) == 0x44,
                "vertical mirroring should share nametables 0 and 2");
        }

        void testChrTileDecoder() {
            Bus chrBus(0x4000);
            Ram chrRam(0x2000);
            RamBusDevice chrDevice(chrRam);
            const Bus::Mapping chrMapping{
              .start = 0x0000,
              .end = 0x1FFF,
              .priority = 0,
              .flags = AccessFlags::Readable,
              .name = QStringLiteral("Test CHR"),
              .device = &chrDevice,
            };
            check(chrBus.registerMapping(chrMapping) != 0, "CHR test mapping should register");

            chrRam.setU8(0x0000, 0b10000001);
            chrRam.setU8(0x0001, 0b01000010);
            chrRam.setU8(0x0008, 0b01000000);
            chrRam.setU8(0x0009, 0b10000000);

            QVector<quint8> pixels;
            check(ChrTileDecoder::decodeTile(
                    chrBus, 0, ChrTileDecoder::PatternTable::Lower, pixels),
                "CHR tile should decode from the lower pattern table");
            check(pixels.size() == 64, "decoded CHR tile should contain 64 pixels");
                check(pixels[0] == 1 && pixels[1] == 2 && pixels[7] == 1,
                "decoded pixels should combine the two CHR bit planes");
                check(pixels[8] == 2 && pixels[9] == 1,
                "decoded pixels should preserve each row's bit planes");

            QVector<quint8> flipped;
            check(ChrTileDecoder::decodeTile(
                    chrBus, 0, ChrTileDecoder::PatternTable::Lower, flipped,
                    ChrTileDecoder::Flip::HorizontalAndVertical),
                "CHR tile should support horizontal and vertical flips");
            check(flipped[0] == pixels[63] && flipped[63] == pixels[0],
                "flipped pixels should reverse both axes");

            check(!ChrTileDecoder::decodeTile(
                    chrBus, ChrTileDecoder::TileCount,
                    ChrTileDecoder::PatternTable::Lower, pixels),
                "tile indices outside a pattern table should be rejected");
            check(pixels.isEmpty(), "rejected tile decoding should clear the output");
        }
}

int main() {
    testCpuInternalRamMirrors();
    testCartridgePrgMirror();
    testCartridgeChrReadOnly();
    testCartridgeDisconnect();
    testInesMapper000Load();
    testChrRamInesLoad();
    testCpuPpuBusIsolation();
    testBusRejectsOverlappingMappings();
    testNesClockRatio();
    testNtscMasterFrame();
    testCpuBasicExecution();
    testCpuCycleAndBranchTiming();
    testPpuRegistersAndBusMirror();
    testPpuVblankAndNmiTiming();
    testPpuMemoryMirrors();
    testPpuNametableRendering();
    testChrTileDecoder();
    std::cout << "NesCoreTests passed\n";
    return EXIT_SUCCESS;
}