#include "Nes.h"

Nes::Nes()
    : cpuComponent(this),
            ppuComponent(),
            cartridgeComponent(),
            clockComponent(
                    [this]() { this->cpuComponent.clock(); },
                    [this]() { this->ppuComponent.clock(); }) {
    this->cartridgeComponent.connect(this->cpuComponent.bus(), this->ppuComponent.bus());
}

Cpu &Nes::cpu() {
    return this->cpuComponent;
}

const Cpu &Nes::cpu() const {
    return this->cpuComponent;
}

Ppu &Nes::ppu() {
    return this->ppuComponent;
}

const Ppu &Nes::ppu() const {
    return this->ppuComponent;
}

Cartridge &Nes::cartridge() {
    return this->cartridgeComponent;
}

const Cartridge &Nes::cartridge() const {
    return this->cartridgeComponent;
}

NesClock &Nes::clock() {
    return this->clockComponent;
}

const NesClock &Nes::clock() const {
    return this->clockComponent;
}
