#pragma once

#include "Bus.h"
#include "Mapper/Mapper.h"
#include "Ram.h"
#include "InesHeader.h"

#include <memory>
#include <functional>

class Mapper001;
#include <functional>

class Cartridge {
public:
    explicit Cartridge(size_t prgRomSize = 16 * 1024, size_t chrRomSize = 8 * 1024);

    Cartridge(const Cartridge &) = delete;
    Cartridge &operator=(const Cartridge &) = delete;

    ~Cartridge();

    void connect(Bus &cpuBus, Bus &ppuBus);
    void disconnect();
    void setMirroringCallback(Mapper::MirroringCallback callback);
    void setMappingChangedCallback(Mapper::MappingChangedCallback callback);
    [[nodiscard]] Mapper001 *mapper001();

    [[nodiscard]] bool loadFromFile(const QString &path, QString &error);
    void unload();
    [[nodiscard]] bool isLoaded() const;
    [[nodiscard]] const InesHeader &header() const;

    [[nodiscard]] Ram &prgRom();
    [[nodiscard]] Ram &prgRam();
    [[nodiscard]] Ram &chrRom();
    [[nodiscard]] const Ram &prgRom() const;
    [[nodiscard]] const Ram &prgRam() const;
    [[nodiscard]] const Ram &chrRom() const;

private:
    [[nodiscard]] std::unique_ptr<Mapper> createMapper(
        quint8 mapperNumber,
        bool chrRam);

    Ram prgRomStorage;
    Ram prgRamStorage;
    Ram chrRomStorage;
    std::unique_ptr<Mapper> mapper;
    MapperCpuDevice cpuDevice;
    MapperPpuDevice ppuDevice;
    RamBusDevice prgRamDevice;
    InesHeader inesHeader;
    bool loaded = false;
    Bus *connectedCpuBus = nullptr;
    Bus *connectedPpuBus = nullptr;
    Bus::MappingId cpuMappingId = 0;
    Bus::MappingId prgRamMappingId = 0;
    Bus::MappingId ppuMappingId = 0;
    Mapper::MirroringCallback mirroringCallback;
    Mapper::MappingChangedCallback mappingChangedCallback;
};
