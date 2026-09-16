#pragma once

#include "Cartridge.h"
#include "Controller.h"
#include "Cpu.h"
#include "Ppu.h"
#include "NesClock.h"

class Nes {
public:
    Nes();
    explicit Nes(NesClock::TimingProfile timing);

    Nes(const Nes &) = delete;
    Nes &operator=(const Nes &) = delete;

    [[nodiscard]] Cpu &cpu();
    [[nodiscard]] const Cpu &cpu() const;
    [[nodiscard]] Ppu &ppu();
    [[nodiscard]] const Ppu &ppu() const;
    [[nodiscard]] Cartridge &cartridge();
    [[nodiscard]] const Cartridge &cartridge() const;
    [[nodiscard]] Controller &controller();
    [[nodiscard]] const Controller &controller() const;
    [[nodiscard]] NesClock &clock();
    [[nodiscard]] const NesClock &clock() const;
    void runMasterTicks(quint64 ticks);
    void runFrame();

private:
    Cpu cpuComponent;
    Ppu ppuComponent;
    Controller controllerComponent;
    Cartridge cartridgeComponent;
    NesClock clockComponent;
};
