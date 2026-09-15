#pragma once

#include "Bus.h"
#include "Mapper/Mapper.h"
#include "Ram.h"
#include "Mapper/Mapper000.h"
#include "InesHeader.h"

class Cartridge {
public:
    explicit Cartridge(size_t prgRomSize = 16 * 1024, size_t chrRomSize = 8 * 1024);

    Cartridge(const Cartridge &) = delete;
    Cartridge &operator=(const Cartridge &) = delete;

    ~Cartridge();

    void connect(Bus &cpuBus, Bus &ppuBus);
    void disconnect();

    [[nodiscard]] bool loadFromFile(const QString &path, QString &error);
    void unload();
    [[nodiscard]] bool isLoaded() const;
    [[nodiscard]] const InesHeader &header() const;

    [[nodiscard]] Ram &prgRom();
    [[nodiscard]] Ram &chrRom();
    [[nodiscard]] const Ram &prgRom() const;
    [[nodiscard]] const Ram &chrRom() const;

private:
    Ram prgRomStorage;
    Ram chrRomStorage;
    Mapper000 mapper;
    MapperCpuDevice cpuDevice;
    MapperPpuDevice ppuDevice;
    InesHeader inesHeader;
    bool loaded = false;
    Bus *connectedCpuBus = nullptr;
    Bus *connectedPpuBus = nullptr;
    Bus::MappingId cpuMappingId = 0;
    Bus::MappingId ppuMappingId = 0;
};
