#include "Nes.h"

Nes::Nes() : Nes(NesClock::TimingProfile()) {
}

Nes::Nes(NesClock::TimingProfile timing)
    : cpuComponent(),
            ppuComponent(),
            cartridgeComponent(),
            clockComponent(
                    [this]() { this->cpuComponent.clock(); },
                    [this]() { this->ppuComponent.clock(); },
                    timing) {
                this->ppuComponent.setNmiCallback(
                    [this]() { this->cpuComponent.nmi(); });
            const Bus::Mapping ppuRegisterMapping{
                .start = 0x2000,
                .end = 0x3FFF,
                .priority = 0,
                .flags = AccessFlags::Readable | AccessFlags::Writable,
                .name = QStringLiteral("PPU registers"),
                .device = &this->ppuComponent,
                .translate = [](quint16 address) {
                    return static_cast<quint16>(0x2000 | (address & 0x0007));
                },
            };
            this->cpuComponent.bus().registerMapping(ppuRegisterMapping);
            const Bus::Mapping controllerMapping{
                .start = 0x4016,
                .end = 0x4017,
                .priority = 0,
                .flags = AccessFlags::Readable | AccessFlags::Writable,
                .name = QStringLiteral("Controller ports"),
                .device = &this->controllerComponent,
            };
            this->cpuComponent.bus().registerMapping(controllerMapping);
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

Controller &Nes::controller() {
    return this->controllerComponent;
}

const Controller &Nes::controller() const {
    return this->controllerComponent;
}

NesClock &Nes::clock() {
    return this->clockComponent;
}

const NesClock &Nes::clock() const {
    return this->clockComponent;
}

void Nes::runMasterTicks(quint64 ticks) {
    this->clockComponent.runMasterTicks(ticks);
}

void Nes::runFrame() {
    this->clockComponent.runFrame();
}
