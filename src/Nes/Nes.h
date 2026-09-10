#pragma once

#include "Cartridge.h"
#include "Cpu.h"
#include "Ppu.h"
#include "NesClock.h"

class Nes {
public:
    Nes();

    Nes(const Nes &) = delete;
    Nes &operator=(const Nes &) = delete;

    [[nodiscard]] Cpu &cpu();
    [[nodiscard]] const Cpu &cpu() const;
    [[nodiscard]] Ppu &ppu();
    [[nodiscard]] const Ppu &ppu() const;
    [[nodiscard]] Cartridge &cartridge();
    [[nodiscard]] const Cartridge &cartridge() const;
    [[nodiscard]] NesClock &clock();
    [[nodiscard]] const NesClock &clock() const;

private:
    Cpu cpuComponent;
    Ppu ppuComponent;
    Cartridge cartridgeComponent;
    NesClock clockComponent;
};
