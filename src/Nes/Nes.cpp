#include "Nes.h"

#include <utility>

Nes::Nes() : Nes(NesClock::TimingProfile()) {
}

Nes::Nes(NesClock::TimingProfile timing)
    : cpuComponent(),
            ppuComponent(),
            controllerComponent(),
            cartridgeComponent(),
            apuComponent(),
            audioOutputShaper(),
            audioSampleScheduler(AudioSampleScheduler::Configuration{
                .inputClockNumerator = timing.cpuClockNumerator,
                .inputClockDenominator = timing.cpuClockDenominator,
            }),
            clockComponent(
                    [this]() {
                        this->cpuComponent.clock();
                        this->apuComponent.clock();
                        const quint32 sampleCount =
                            this->audioSampleScheduler.advance();
                        if (sampleCount == 0) {
                            return;
                        }
                        const float sample = this->apuComponent.sample();
                        for (quint32 index = 0; index < sampleCount; ++index) {
                            this->audioSampleScheduler.submitSample(sample);
                        }
                    },
                    [this]() { this->ppuComponent.clock(); },
                    timing) {
                this->audioSampleScheduler.setSampleCallback(
                    [this](float sample) {
                        const float shapedSample =
                            this->audioOutputShaper.process(sample);
                        if (this->audioSampleCallback) {
                            this->audioSampleCallback(shapedSample);
                        }
                    });
                this->apuComponent.setIrqCallback(
                    [this](bool pending) {
                        if (pending) {
                            this->cpuComponent.setIrq(Cpu::IrqFrameIrq);
                        } else {
                            this->cpuComponent.clearIrq(Cpu::IrqFrameIrq);
                        }
                    });
                this->apuComponent.setDpcmMemoryReader(
                    [this](quint16 address) {
                        return this->cpuComponent.readRam8(address);
                    });
                this->cartridgeComponent.setMirroringCallback(
                    [this](Mapper::Mirroring mirroring) {
                        switch (mirroring) {
                        case Mapper::Mirroring::SingleScreenA:
                            this->ppuComponent.setNametableMirroring(
                                Ppu::NametableMirroring::SingleScreenA);
                            break;
                        case Mapper::Mirroring::SingleScreenB:
                            this->ppuComponent.setNametableMirroring(
                                Ppu::NametableMirroring::SingleScreenB);
                            break;
                        case Mapper::Mirroring::Vertical:
                            this->ppuComponent.setNametableMirroring(
                                Ppu::NametableMirroring::Vertical);
                            break;
                        case Mapper::Mirroring::FourScreen:
                            this->ppuComponent.setNametableMirroring(
                                Ppu::NametableMirroring::FourScreen);
                            break;
                        case Mapper::Mirroring::Horizontal:
                            this->ppuComponent.setNametableMirroring(
                                Ppu::NametableMirroring::Horizontal);
                            break;
                        }
                    });
                this->ppuComponent.setNmiCallback(
                    [this]() {
                        this->cpuComponent.nmi();
                        this->cpuComponent.deferNmi();
                    });
                    this->ppuComponent.setOamDmaCallback(
                        [this](quint8 page) {
                        const quint16 base = static_cast<quint16>(page) << 8;
                        for (quint16 offset = 0; offset < 0x0100; ++offset) {
                            quint8 value = this->cpuComponent.bus().openBusValue();
                            this->cpuComponent.bus().read(
                                static_cast<quint16>(base + offset), value);
                            this->ppuComponent.writeOamDmaByte(value);
                        }
                        this->cpuComponent.dma(
                            513 + (this->cpuComponent.getTotalCycles() & 1));
                        });
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
            const Bus::Mapping oamDmaMapping{
                .start = 0x4014,
                .end = 0x4014,
                .priority = 0,
                .flags = AccessFlags::Writable,
                .name = QStringLiteral("OAM DMA"),
                .device = &this->ppuComponent,
                .translate = [](quint16) {
                    return static_cast<quint16>(0x4014);
                },
            };
            this->cpuComponent.bus().registerMapping(oamDmaMapping);
            const Bus::Mapping controllerMapping{
                .start = 0x4016,
                .end = 0x4017,
                .priority = 0,
                .flags = AccessFlags::Readable,
                .name = QStringLiteral("Controller port reads"),
                .device = &this->controllerComponent,
            };
            this->cpuComponent.bus().registerMapping(controllerMapping);
            const Bus::Mapping controllerStrobeMapping{
                .start = 0x4016,
                .end = 0x4016,
                .priority = 0,
                .flags = AccessFlags::Writable,
                .name = QStringLiteral("Controller strobe"),
                .device = &this->controllerComponent,
            };
            this->cpuComponent.bus().registerMapping(controllerStrobeMapping);
            const Bus::Mapping apuMapping{
                .start = 0x4000,
                .end = 0x4013,
                .priority = 0,
                .flags = AccessFlags::Writable,
                .name = QStringLiteral("APU registers"),
                .device = &this->apuComponent,
            };
            this->cpuComponent.bus().registerMapping(apuMapping);
            const Bus::Mapping apuStatusMapping{
                .start = 0x4015,
                .end = 0x4015,
                .priority = 0,
                .flags = AccessFlags::Readable | AccessFlags::Writable,
                .name = QStringLiteral("APU status"),
                .device = &this->apuComponent,
                .translate = [](quint16) {
                    return static_cast<quint16>(0x15);
                },
            };
            this->cpuComponent.bus().registerMapping(apuStatusMapping);
            const Bus::Mapping apuFrameCounterMapping{
                .start = 0x4017,
                .end = 0x4017,
                .priority = 0,
                .flags = AccessFlags::Writable,
                .name = QStringLiteral("APU frame counter"),
                .device = &this->apuComponent,
                .translate = [](quint16) {
                    return static_cast<quint16>(0x17);
                },
            };
            this->cpuComponent.bus().registerMapping(apuFrameCounterMapping);
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

Apu &Nes::apu() {
    return this->apuComponent;
}

const Apu &Nes::apu() const {
    return this->apuComponent;
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

void Nes::setAudioSampleCallback(
        AudioSampleScheduler::SampleCallback callback) {
    this->audioSampleCallback = std::move(callback);
}

void Nes::resetAudioSampling() {
    this->audioSampleScheduler.reset();
    this->audioOutputShaper.reset();
}

quint64 Nes::generatedAudioSampleCount() const {
    return this->audioSampleScheduler.generatedSampleCount();
}
